#include "bt_app_core.h"
#include "bt_app_av.h"

/* AVRCP used transaction labels */
#define APP_RC_CT_TL_GET_CAPS (0)
#define APP_RC_CT_TL_GET_META_DATA (1)
#define APP_RC_CT_TL_RN_TRACK_CHANGE (2)
#define APP_RC_CT_TL_RN_PLAYBACK_CHANGE (3)
#define APP_RC_CT_TL_RN_PLAY_POS_CHANGE (4)

/* Application layer causes delay value */
#define APP_DELAY_VALUE 50 // 5ms

static uint32_t s_pkt_cnt = 0; /* count for audio packet */
static esp_a2d_audio_state_t s_audio_state = ESP_A2D_AUDIO_STATE_STOPPED;
/* audio stream datapath state in string */
static esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;
/* AVRC target notification capability bit mask */
static _lock_t s_volume_lock;
static TaskHandle_t s_vcs_task_hdl = NULL; /* handle for volume change simulation task */
static uint8_t s_volume = 0;               /* local volume value */
dac_continuous_handle_t tx_chan;

void bt_i2s_driver_install(void)
{
    dac_continuous_config_t cont_cfg = {
        .chan_mask = DAC_CHANNEL_MASK_ALL,
        .desc_num = 8,
        .buf_size = 2048,
        .freq_hz = 44100,
        .offset = 127,
        .clk_src = DAC_DIGI_CLK_SRC_DEFAULT, // Using APLL as clock source to get a wider frequency range
        .chan_mode = DAC_CHANNEL_MODE_ALTER,
    };
    /* Allocate continuous channels */
    ESP_ERROR_CHECK(dac_continuous_new_channels(&cont_cfg, &tx_chan));
    /* Enable the continuous channels */
    ESP_ERROR_CHECK(dac_continuous_enable(tx_chan));
}

void bt_i2s_driver_uninstall(void)
{
    ESP_ERROR_CHECK(dac_continuous_disable(tx_chan));
    ESP_ERROR_CHECK(dac_continuous_del_channels(tx_chan));
}

void volume_set_by_controller(uint8_t volume)
{
    ESP_LOGI(BT_RC_TG_TAG, "Volume is set by remote controller to: %" PRIu32 "%%", (uint32_t)volume * 100 / 0x7f);
    /* set the volume in protection of lock */
    _lock_acquire(&s_volume_lock);
    s_volume = volume;
    _lock_release(&s_volume_lock);
}

void bt_av_hdl_a2d_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_AV_TAG, "%s event: %d", __func__, event);

    esp_a2d_cb_param_t *a2d = NULL;

    switch (event)
    {
    /* when connection state changed, this event comes */
    case ESP_A2D_CONNECTION_STATE_EVT:
    {
        a2d = (esp_a2d_cb_param_t *)(p_param);
        if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
        {
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            bt_i2s_driver_uninstall();
            bt_i2s_task_shut_down();
        }
        else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED)
        {
            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
            bt_i2s_task_start_up();
        }
        else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTING)
        {
            bt_i2s_driver_install();
        }
        break;
    }
    /* when audio stream transmission state changed, this event comes */
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
    /* when audio codec is configured, this event comes */
    case ESP_A2D_AUDIO_CFG_EVT:
    {
        a2d = (esp_a2d_cb_param_t *)(p_param);
        if (a2d->audio_cfg.mcc.type == ESP_A2D_MCT_SBC)
        {
            int sample_rate = 44100;

            dac_continuous_disable(tx_chan);
            dac_continuous_del_channels(tx_chan);
            dac_continuous_config_t cont_cfg = {
                .chan_mask = DAC_CHANNEL_MASK_ALL,
                .desc_num = 8,
                .buf_size = 2048,
                .freq_hz = sample_rate,
                .offset = 127,
                .clk_src = DAC_DIGI_CLK_SRC_DEFAULT, // Using APLL as clock source to get a wider frequency range
                .chan_mode = DAC_CHANNEL_MODE_ALTER,
            };
            /* Allocate continuous channels */
            dac_continuous_new_channels(&cont_cfg, &tx_chan);
            /* Enable the continuous channels */
            dac_continuous_enable(tx_chan);
        }
        break;
    }
    /* when a2dp init or deinit completed, this event comes */
    case ESP_A2D_PROF_STATE_EVT:
    {
        a2d = (esp_a2d_cb_param_t *)(p_param);
        if (ESP_A2D_INIT_SUCCESS == a2d->a2d_prof_stat.init_state)
        {
            ESP_LOGI(BT_AV_TAG, "A2DP PROF STATE: Init Complete");
        }
        else
        {
            ESP_LOGI(BT_AV_TAG, "A2DP PROF STATE: Deinit Complete");
        }
        break;
    }
    /* When protocol service capabilities configured, this event comes */
    case ESP_A2D_SNK_PSC_CFG_EVT:
    {
        a2d = (esp_a2d_cb_param_t *)(p_param);
        ESP_LOGI(BT_AV_TAG, "protocol service capabilities configured: 0x%x ", a2d->a2d_psc_cfg_stat.psc_mask);
        if (a2d->a2d_psc_cfg_stat.psc_mask & ESP_A2D_PSC_DELAY_RPT)
        {
            ESP_LOGI(BT_AV_TAG, "Peer device support delay reporting");
        }
        else
        {
            ESP_LOGI(BT_AV_TAG, "Peer device unsupport delay reporting");
        }
        break;
    }
    /* when set delay value completed, this event comes */
    case ESP_A2D_SNK_SET_DELAY_VALUE_EVT:
    {
        a2d = (esp_a2d_cb_param_t *)(p_param);
        if (ESP_A2D_SET_INVALID_PARAMS == a2d->a2d_set_delay_value_stat.set_state)
        {
            ESP_LOGI(BT_AV_TAG, "Set delay report value: fail");
        }
        else
        {
            ESP_LOGI(BT_AV_TAG, "Set delay report value: success, delay_value: %u * 1/10 ms", a2d->a2d_set_delay_value_stat.delay_value);
        }
        break;
    }
    /* when get delay value completed, this event comes */
    case ESP_A2D_SNK_GET_DELAY_VALUE_EVT:
    {
        a2d = (esp_a2d_cb_param_t *)(p_param);
        ESP_LOGI(BT_AV_TAG, "Get delay report value: delay_value: %u * 1/10 ms", a2d->a2d_get_delay_value_stat.delay_value);
        /* Default delay value plus delay caused by application layer */
        esp_a2d_sink_set_delay_value(a2d->a2d_get_delay_value_stat.delay_value + APP_DELAY_VALUE);
        break;
    }
    /* others */
    default:
        ESP_LOGE(BT_AV_TAG, "%s unhandled event: %d", __func__, event);
        break;
    }
}

void bt_av_hdl_avrc_ct_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_RC_CT_TAG, "%s event: %d", __func__, event);

    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(p_param);

    switch (event)
    {
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
    /* when connection state changed, this event comes */
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
    {
        if (rc->conn_stat.connected)
        {
            /* get remote supported event_ids of peer AVRCP Target */
            esp_avrc_ct_send_get_rn_capabilities_cmd(APP_RC_CT_TL_GET_CAPS);
        }
        else
        {
            /* clear peer notification capability record */
            s_avrc_peer_rn_cap.bits = 0;
        }
        break;
    }

    case ESP_AVRC_CT_METADATA_RSP_EVT:
    {
        free(rc->meta_rsp.attr_text);
        break;
    }

    /* when notification capability of peer device got, this event comes */
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

void bt_av_hdl_avrc_tg_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_RC_TG_TAG, "%s event: %d", __func__, event);

    esp_avrc_tg_cb_param_t *rc = (esp_avrc_tg_cb_param_t *)(p_param);

    switch (event)
    {
    /* when connection state changed, this event comes */
    case ESP_AVRC_TG_CONNECTION_STATE_EVT:
    {
        uint8_t *bda = rc->conn_stat.remote_bda;
        ESP_LOGI(BT_RC_TG_TAG, "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
        vTaskDelete(s_vcs_task_hdl);
        ESP_LOGI(BT_RC_TG_TAG, "Stop volume change simulation");

        break;
    }
    /* when passthrough commanded, this event comes */
    case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT:
    {
        ESP_LOGI(BT_RC_TG_TAG, "AVRC passthrough cmd: key_code 0x%x, key_state %d", rc->psth_cmd.key_code, rc->psth_cmd.key_state);
        break;
    }
    /* when absolute volume command from remote device set, this event comes */
    case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
    {
        ESP_LOGI(BT_RC_TG_TAG, "AVRC set absolute volume: %d%%", (int)rc->set_abs_vol.volume * 100 / 0x7f);
        volume_set_by_controller(rc->set_abs_vol.volume);
        break;
    }
    /* when feature of remote device indicated, this event comes */
    case ESP_AVRC_TG_REMOTE_FEATURES_EVT:
    {
        ESP_LOGI(BT_RC_TG_TAG, "AVRC remote features: %" PRIx32 ", CT features: %x", rc->rmt_feats.feat_mask, rc->rmt_feats.ct_feat_flag);
        break;
    }
    /* others */
    default:
        ESP_LOGE(BT_RC_TG_TAG, "%s unhandled event: %d", __func__, event);
        break;
    }
}

void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
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
        bt_app_work_dispatch(bt_av_hdl_a2d_evt, event, param, sizeof(esp_a2d_cb_param_t), NULL);
        break;
    }
    default:
        ESP_LOGE(BT_AV_TAG, "Invalid A2DP event: %d", event);
        break;
    }
}

void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len)
{
    write_ringbuf(data, len);
}

void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    switch (event)
    {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
    {
        bt_app_work_dispatch(bt_av_hdl_avrc_ct_evt, event, param, sizeof(esp_avrc_ct_cb_param_t), NULL);
        break;
    }
    default:
        ESP_LOGE(BT_RC_CT_TAG, "Invalid AVRC event: %d", event);
        break;
    }
}

void bt_app_rc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param)
{
    switch (event)
    {
    case ESP_AVRC_TG_CONNECTION_STATE_EVT:
    case ESP_AVRC_TG_REMOTE_FEATURES_EVT:
    case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT:
    case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
    case ESP_AVRC_TG_SET_PLAYER_APP_VALUE_EVT:
        bt_app_work_dispatch(bt_av_hdl_avrc_tg_evt, event, param, sizeof(esp_avrc_tg_cb_param_t), NULL);
        break;
    default:
        ESP_LOGE(BT_RC_TG_TAG, "Invalid AVRC event: %d", event);
        break;
    }
}
