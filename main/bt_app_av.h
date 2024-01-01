#ifndef __BT_APP_AV_H__
#define __BT_APP_AV_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "esp_log.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/dac_continuous.h"
#include "sys/lock.h"

#define BT_AV_TAG "BT_AV"
#define BT_RC_TG_TAG "RC_TG"
#define BT_RC_CT_TAG "RC_CT"

void install_i2s_driver(void);

void uninstall_i2s_driver(void);

void set_volume_by_bluetooth_client(uint8_t volume);

void handle_bt_audio_distribution_event(uint16_t event, void *p_param);

void handle_bt_avrc_controller_event(uint16_t event, void *p_param);

void handle_bt_avrc_target_event(uint16_t event, void *p_param);

void bluetooth_app_audio_distribution_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

void bluetooth_app_audio_distribution_data_callback(const uint8_t *data, uint32_t len);

void bluetooth_app_avrc_controller_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);

void bluetooth_app_avrc_target_callback(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);

#endif /* __BT_APP_AV_H__*/
