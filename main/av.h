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

/**
 * @brief Tag for Bluetooth Audio/Video (AV) related logs.
 *
 * This constant is used as a tag for logging messages related to Bluetooth Audio/Video operations.
 */
#define BT_AV_TAG "BT_AV"

/**
 * @brief Tag for Bluetooth Remote Control Target (RC_TG) related logs.
 *
 * This constant is used as a tag for logging messages related to Bluetooth Remote Control Target operations.
 */
#define BT_RC_TG_TAG "RC_TG"

/**
 * @brief Tag for Bluetooth Remote Control Controller (RC_CT) related logs.
 *
 * This constant is used as a tag for logging messages related to Bluetooth Remote Control Controller operations.
 */
#define BT_RC_CT_TAG "RC_CT"

/**
 * @brief Installs the I2S driver for DAC (Digital-to-Analog Converter).
 *
 * This function configures the DAC for continuous mode with the specified parameters,
 * creates new channels, and enables them. The DAC is configured to use all channels,
 * with 8 descriptors, a buffer size of 2048, a frequency of 44100 Hz, an offset of 127,
 * the default clock source, and alternating channel mode.
 *
 * @note This function will abort the program if any of the ESP_ERROR_CHECK calls fail.
 */
void install_i2s_driver(void);

/**
 * @brief Uninstalls the I2S driver for DAC (Digital-to-Analog Converter).
 *
 * This function disables the continuous mode for the DAC and deletes the channels.
 * It checks for errors at each step and will abort the program if any of the ESP_ERROR_CHECK calls fail.
 */
void uninstall_i2s_driver(void);

/**
 * @brief Sets the volume level as specified by the Bluetooth client.
 *
 * This function acquires a lock before setting the volume to prevent race conditions.
 * After setting the volume, it releases the lock.
 *
 * @param volume The volume level to set. Should be a value between 0 and 255.
 */
void set_volume_by_bluetooth_client(uint8_t volume);

/**
 * @brief Handles Bluetooth audio distribution events.
 *
 * This function is called when a Bluetooth audio distribution event occurs. It logs the event and performs
 * actions based on the event type. These actions include setting the Bluetooth scan mode, starting and stopping
 * the I2S task, installing and uninstalling the I2S driver, and setting the audio state and delay value.
 *
 * @param event The Bluetooth audio distribution event type.
 * @param p_param Pointer to the parameters associated with the event. This should be a pointer to an
 * esp_a2d_cb_param_t structure.
 */
void handle_bt_audio_distribution_event(uint16_t event, void *p_param);

/**
 * @brief Handles Bluetooth AVRCP (Audio/Video Remote Control Profile) controller events.
 *
 * This function is called when a Bluetooth AVRCP controller event occurs. It logs the event and performs
 * actions based on the event type. These actions include sending a get capabilities command when a connection
 * is established, freeing attribute text when metadata is received, and updating the remote notification
 * capabilities when they are received.
 *
 * @param event The Bluetooth AVRCP controller event type.
 * @param p_param Pointer to the parameters associated with the event. This should be a pointer to an
 * esp_avrc_ct_cb_param_t structure.
 */
void handle_bt_avrc_controller_event(uint16_t event, void *p_param);

/**
 * @brief Handles Bluetooth AVRCP (Audio/Video Remote Control Profile) target events.
 *
 * This function is called when a Bluetooth AVRCP target event occurs. It logs the event and performs
 * actions based on the event type. These actions include deleting the VCS task when certain events occur,
 * and setting the volume when a set absolute volume command is received.
 *
 * @param event The Bluetooth AVRCP target event type.
 * @param p_param Pointer to the parameters associated with the event. This should be a pointer to an
 * esp_avrc_tg_cb_param_t structure.
 */
void handle_bt_avrc_target_event(uint16_t event, void *p_param);

/**
 * @brief Callback function for Bluetooth Audio Distribution Profile (A2DP) events.
 *
 * This function is called when an A2DP event occurs. It dispatches the event to the 
 * `handle_bt_audio_distribution_event` function for handling, passing along the event type and parameters.
 * If an invalid event is received, it logs an error message.
 *
 * @param event The A2DP event type.
 * @param param Pointer to the parameters associated with the event. This should be a pointer to an
 * esp_a2d_cb_param_t structure.
 */
void bluetooth_app_audio_distribution_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

/**
 * @brief Callback function for Bluetooth Audio Distribution Profile (A2DP) data events.
 *
 * This function is called when an A2DP data event occurs. It writes the received data to a ring buffer.
 *
 * @param data Pointer to the received data. This should be a pointer to a buffer of uint8_t values.
 * @param len The length of the received data, in bytes.
 */
void bluetooth_app_audio_distribution_data_callback(const uint8_t *data, uint32_t len);

/**
 * @brief Callback function for Bluetooth AVRCP (Audio/Video Remote Control Profile) controller events.
 *
 * This function is called when an AVRCP controller event occurs. It dispatches the event to the 
 * `handle_bt_avrc_controller_event` function for handling, passing along the event type and parameters.
 * If an invalid event is received, it logs an error message.
 *
 * @param event The AVRCP controller event type.
 * @param param Pointer to the parameters associated with the event. This should be a pointer to an
 * esp_avrc_ct_cb_param_t structure.
 */
void bluetooth_app_avrc_controller_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);

/**
 * @brief Callback function for Bluetooth AVRCP (Audio/Video Remote Control Profile) target events.
 *
 * This function is called when an AVRCP target event occurs. It dispatches the event to the 
 * `handle_bt_avrc_target_event` function for handling, passing along the event type and parameters.
 * If an invalid event is received, it logs an error message.
 *
 * @param event The AVRCP target event type.
 * @param param Pointer to the parameters associated with the event. This should be a pointer to an
 * esp_avrc_tg_cb_param_t structure.
 */
void bluetooth_app_avrc_target_callback(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);

#endif /* __BT_APP_AV_H__*/
