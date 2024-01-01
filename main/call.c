#include "call.h"

void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event)
    {

    case ESP_BT_GAP_MODE_CHG_EVT:
    case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
    case ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
    default:
    {
        ESP_LOGI(BT_AV_TAG, "event: %d", event);
        break;
    }
    }
}

void bt_av_hdl_stack_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_AV_TAG, "%s event: %d", __func__, event);

    switch (event)
    {
    case BT_APP_EVT_STACK_UP:
    {
        esp_bt_dev_set_device_name(DEVICE_NAME);
        esp_bt_gap_register_callback(bt_app_gap_cb);

        assert(esp_avrc_ct_init() == ESP_OK);
        esp_avrc_ct_register_callback(bluetooth_app_avrc_controller_callback);
        assert(esp_avrc_tg_init() == ESP_OK);
        esp_avrc_tg_register_callback(bluetooth_app_avrc_target_callback);

        esp_avrc_rn_evt_cap_mask_t evt_set = {0};
        esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
        assert(esp_avrc_tg_set_rn_evt_cap(&evt_set) == ESP_OK);

        assert(esp_a2d_sink_init() == ESP_OK);
        esp_a2d_register_callback(&bluetooth_app_audio_distribution_callback);
        esp_a2d_sink_register_data_callback(bluetooth_app_audio_distribution_data_callback);

        esp_a2d_sink_get_delay_value();

        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        break;
    }

    default:
        ESP_LOGE(BT_AV_TAG, "%s unhandled event: %d", __func__, event);
        break;
    }
}