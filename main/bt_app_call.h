#ifndef BT_APP_CALL_H
#define BT_APP_CALL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "bt_app_core.h"
#include "bt_app_av.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

#define BT_APP_EVT_STACK_UP 0
#define DEVICE_NAME "INZYNIERKA 2024"

void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

void bt_av_hdl_stack_evt(uint16_t event, void *p_param);

#endif /* BT_APP_CALL_H */