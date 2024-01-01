#include "core.h"
#include "av.h"

#define APP_DELAY_VALUE 50
#define APP_RC_CT_GET_CAPABILITIES 0

static uint32_t s_pkt_cnt = 0;
static esp_a2d_audio_state_t s_audio_state = ESP_A2D_AUDIO_STATE_STOPPED;
static esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;
static _lock_t s_volume_lock;
static TaskHandle_t s_vcs_task_hdl = NULL;
static uint8_t s_volume = 0;
dac_continuous_handle_t tx_chan;

void install_i2s_driver(void)
{
    dac_continuous_config_t cont_cfg = {
        .chan_mask = DAC_CHANNEL_MASK_ALL,
        .desc_num = 8,
        .buf_size = 2048,
        .freq_hz = 44100,
        .offset = 127,
        .clk_src = DAC_DIGI_CLK_SRC_DEFAULT,
        .chan_mode = DAC_CHANNEL_MODE_ALTER,
    };

    ESP_ERROR_CHECK(dac_continuous_new_channels(&cont_cfg, &tx_chan));

    ESP_ERROR_CHECK(dac_continuous_enable(tx_chan));
}

void uninstall_i2s_driver(void)
{
    ESP_ERROR_CHECK(dac_continuous_disable(tx_chan));
    ESP_ERROR_CHECK(dac_continuous_del_channels(tx_chan));
}

void set_volume_by_bluetooth_client(uint8_t volume)
{
    _lock_acquire(&s_volume_lock);
    s_volume = volume;
    _lock_release(&s_volume_lock);
}

void handle_bt_audio_distribution_event(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_AV_TAG, "%s event: %d", __func__, event);

    esp_a2d_cb_param_t *a2d = NULL;

    switch (event)
    {
    case ESP_A2D_AUDIO_CFG_EVT:
    case ESP_A2D_PROF_STATE_EVT:
    case ESP_A2D_SNK_PSC_CFG_EVT:
    case ESP_A2D_SNK_SET_DELAY_VALUE_EVT:
    case ESP_A2D_CONNECTION_STATE_EVT:
    {
        a2d = (esp_a2d_cb_param_t *)(p_param);
        if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
        {
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            uninstall_i2s_driver();
            shut_down_i2s_task();
        }
        else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED)
        {
            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
            start_i2s_task();
        }
        else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTING)
        {
            install_i2s_driver();
        }
        break;
    }

    case ESP_A2D_AUDIO_STATE_EVT:
    {
        a2d = (esp_a2d_cb_param_t *)(p_param);
        s_audio_state = a2d->audio_stat.state;
        if (ESP_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state)
        {
            s_pkt_cnt = 0;
        }
        break;
    }

    case ESP_A2D_SNK_GET_DELAY_VALUE_EVT:
    {
        a2d = (esp_a2d_cb_param_t *)(p_param);
        esp_a2d_sink_set_delay_value(a2d->a2d_get_delay_value_stat.delay_value + APP_DELAY_VALUE);
        break;
    }

    default:
        ESP_LOGE(BT_AV_TAG, "%s unhandled event: %d", __func__, event);
        break;
    }
}

void handle_bt_avrc_controller_event(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_RC_CT_TAG, "%s event: %d", __func__, event);

    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(p_param);

    switch (event)
    {
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
    {
        if (rc->conn_stat.connected)
        {
            esp_avrc_ct_send_get_rn_capabilities_cmd(APP_RC_CT_GET_CAPABILITIES);
        }
        else
        {
            s_avrc_peer_rn_cap.bits = 0;
        }
        break;
    }

    case ESP_AVRC_CT_METADATA_RSP_EVT:
    {
        free(rc->meta_rsp.attr_text);
        break;
    }

    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
    {
        s_avrc_peer_rn_cap.bits = rc->get_rn_caps_rsp.evt_set.bits;
        break;
    }

    default:
        ESP_LOGE(BT_RC_CT_TAG, "%s unhandled event: %d", __func__, event);
        break;
    }
}

void handle_bt_avrc_target_event(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_RC_TG_TAG, "%s event: %d", __func__, event);

    esp_avrc_tg_cb_param_t *rc = (esp_avrc_tg_cb_param_t *)(p_param);

    switch (event)
    {
    case ESP_AVRC_TG_REMOTE_FEATURES_EVT:
    case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT:
    case ESP_AVRC_TG_CONNECTION_STATE_EVT:
    {
        vTaskDelete(s_vcs_task_hdl);
        break;
    }

    case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
    {
        set_volume_by_bluetooth_client(rc->set_abs_vol.volume);
        break;
    }

    default:
        ESP_LOGE(BT_RC_TG_TAG, "%s unhandled event: %d", __func__, event);
        break;
    }
}

void bluetooth_app_audio_distribution_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch (event)
    {
    case ESP_A2D_CONNECTION_STATE_EVT:
    case ESP_A2D_AUDIO_STATE_EVT:
    case ESP_A2D_AUDIO_CFG_EVT:
    case ESP_A2D_PROF_STATE_EVT:
    case ESP_A2D_SNK_PSC_CFG_EVT:
    case ESP_A2D_SNK_SET_DELAY_VALUE_EVT:
    case ESP_A2D_SNK_GET_DELAY_VALUE_EVT:
    {
        dispatch_bluetooth_app_work_with_callback(handle_bt_audio_distribution_event, event, param, sizeof(esp_a2d_cb_param_t), NULL);
        break;
    }
    default:
        ESP_LOGE(BT_AV_TAG, "Invalid A2DP event: %d", event);
        break;
    }
}

void bluetooth_app_audio_distribution_data_callback(const uint8_t *data, uint32_t len)
{
    write_to_ringbuffer(data, len);
}

void bluetooth_app_avrc_controller_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    switch (event)
    {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
    {
        dispatch_bluetooth_app_work_with_callback(handle_bt_avrc_controller_event, event, param, sizeof(esp_avrc_ct_cb_param_t), NULL);
        break;
    }
    default:
        ESP_LOGE(BT_RC_CT_TAG, "Invalid AVRC event: %d", event);
        break;
    }
}

void bluetooth_app_avrc_target_callback(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param)
{
    switch (event)
    {
    case ESP_AVRC_TG_CONNECTION_STATE_EVT:
    case ESP_AVRC_TG_REMOTE_FEATURES_EVT:
    case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT:
    case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
    case ESP_AVRC_TG_SET_PLAYER_APP_VALUE_EVT:
        dispatch_bluetooth_app_work_with_callback(handle_bt_avrc_target_event, event, param, sizeof(esp_avrc_tg_cb_param_t), NULL);
        break;
    default:
        ESP_LOGE(BT_RC_TG_TAG, "Invalid AVRC event: %d", event);
        break;
    }
}
