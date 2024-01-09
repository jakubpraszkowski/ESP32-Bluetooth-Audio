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
#include "core.h"
#include "av.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

/**
 * @brief Event code for Bluetooth stack up.
 *
 * This constant is used as an event code to indicate that the Bluetooth stack is up and ready.
 */
#define BT_APP_EVT_STACK_UP 0

/**
 * @brief Name of the device.
 *
 * This constant is used as the name of the Bluetooth device.
 */
#define DEVICE_NAME "INZYNIERKA 2024"

/**
 * @brief Callback function for Bluetooth GAP (Generic Access Profile) events.
 *
 * This function is called when a GAP event occurs. It logs the event and performs
 * actions based on the event type.
 *
 * @param event The GAP event type.
 * @param param The parameters associated with the event.
 */
void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

/**
 * @brief Handles Bluetooth stack events.
 *
 * This function is called when a Bluetooth stack event occurs. It logs the event and performs
 * actions based on the event type. For the BT_APP_EVT_STACK_UP event, it sets up various Bluetooth
 * profiles and registers their callbacks.
 *
 * @param event The Bluetooth stack event type.
 * @param p_param Pointer to the parameters associated with the event.
 */
void bt_av_hdl_stack_evt(uint16_t event, void *p_param);

#endif /* BT_APP_CALL_H */