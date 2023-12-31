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

/* log tags */
#define BT_AV_TAG "BT_AV"
#define BT_RC_TG_TAG "RC_TG"
#define BT_RC_CT_TAG "RC_CT"

/* installation for i2s */
void bt_i2s_driver_install(void);
/* uninstallation for i2s */
void bt_i2s_driver_uninstall(void);
/* set volume by remote controller */
void volume_set_by_controller(uint8_t volume);
/* set volume by local host */
void volume_set_by_local_host(uint8_t volume);
/* a2dp event handler */
void bt_av_hdl_a2d_evt(uint16_t event, void *p_param);
/* avrc controller event handler */
void bt_av_hdl_avrc_ct_evt(uint16_t event, void *p_param);
/* avrc target event handler */
void bt_av_hdl_avrc_tg_evt(uint16_t event, void *p_param);

/**
 * @brief  callback function for A2DP sink
 *
 * @param [in] event  event id
 * @param [in] param  callback parameter
 */
void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

/**
 * @brief  callback function for A2DP sink audio data stream
 *
 * @param [out] data  data stream writteen by application task
 * @param [in]  len   length of data stream in byte
 */
void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len);

/**
 * @brief  callback function for AVRCP controller
 *
 * @param [in] event  event id
 * @param [in] param  callback parameter
 */
void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);

/**
 * @brief  callback function for AVRCP target
 *
 * @param [in] event  event id
 * @param [in] param  callback parameter
 */
void bt_app_rc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);

#endif /* __BT_APP_AV_H__*/
