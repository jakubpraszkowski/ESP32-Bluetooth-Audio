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

/* AVRCP used transaction labels */
#define APP_RC_CT_TL_GET_CAPS (0)
#define APP_RC_CT_TL_GET_META_DATA (1)
#define APP_RC_CT_TL_RN_TRACK_CHANGE (2)
#define APP_RC_CT_TL_RN_PLAYBACK_CHANGE (3)
#define APP_RC_CT_TL_RN_PLAY_POS_CHANGE (4)

/* Application layer causes delay value */
#define APP_DELAY_VALUE 50 // 5ms

/**
 * @brief Allocates a buffer for metadata and copies the metadata into it.
 *
 * This function allocates a buffer of size equal to the length of the metadata plus one,
 * then copies the metadata into this buffer. The last byte of the buffer is set to 0.
 * The metadata pointer is then updated to point to this new buffer.
 *
 * @param param Pointer to the metadata parameter. This should point to a variable of type esp_avrc_ct_cb_param_t.
 */
void bt_app_alloc_meta_buffer(esp_avrc_ct_cb_param_t *param);

/**
 * @brief Handles a new track event in the Bluetooth Audio Video Remote Control (AVRC) profile.
 *
 * This function requests metadata for the new track, including title, artist, album, and genre.
 * If the peer device supports track change notifications, it also registers for such notifications.
 */
void bt_av_new_track(void);

/**
 * @brief Handles a playback status change event in the Bluetooth Audio Video Remote Control (AVRC) profile.
 *
 * This function checks if the peer device supports playback status change notifications.
 * If it does, the function registers for such notifications.
 */
void bt_av_playback_changed(void);

/**
 * @brief Handles a playback position change event in the Bluetooth Audio Video Remote Control (AVRC) profile.
 *
 * This function checks if the peer device supports playback position change notifications.
 * If it does, the function registers for such notifications.
 */
void bt_av_play_pos_changed(void);

/**
 * @brief Handles notifications for the Bluetooth Audio Video Remote Control (AVRC) profile.
 *
 * This function handles various events related to the AVRC profile. When a new track is loaded,
 * it calls the bt_av_new_track function. When the playback status changes, it logs the new status
 * and calls the bt_av_playback_changed function. When the playback position changes, it logs the
 * new position and calls the bt_av_play_pos_changed function. If the event is not one of these,
 * it logs that the event is unhandled.
 *
 * @param event_id The ID of the event.
 * @param event_parameter Pointer to the parameter of the event. This can be NULL if no parameter is needed.
 */
void bt_av_notify_evt_handler(uint8_t event_id, esp_avrc_rn_param_t *event_parameter);

/**
 * @brief Installs the I2S driver for the Bluetooth application.
 *
 * This function configures the DAC for continuous operation with a specific configuration.
 * It allocates continuous channels with the given configuration and enables them. The configuration includes parameters
 * such as the channel mask, descriptor number, buffer size, frequency, offset, clock source, and channel mode.
 */
void bt_i2s_driver_install(void);

/**
 * @brief Uninstalls the I2S driver for the Bluetooth application.
 *
 * This function disables the continuous operation of the DAC and deletes the continuous channels.
 */
void bt_i2s_driver_uninstall(void);

/**
 * @brief Sets the volume level as specified by a remote controller.
 *
 * This function logs the new volume level and sets the volume to the specified level.
 * The volume level is expected to be a value between 0 and 127 (0x7f), inclusive.
 * The function acquires a lock before changing the volume and releases it afterwards
 * to prevent other tasks from changing the volume at the same time.
 *
 * @param volume The new volume level, as a value between 0 and 127 (0x7f), inclusive.
 */
void volume_set_by_controller(uint8_t volume);

/**
 * @brief Sets the volume level as specified by the local host.
 *
 * This function logs the new volume level and sets the volume to the specified level.
 * The volume level is expected to be a value between 0 and 127 (0x7f), inclusive.
 * The function acquires a lock before changing the volume and releases it afterwards
 * to prevent other tasks from changing the volume at the same time.
 * If a volume change notification was previously requested by a remote AVRCP controller,
 * it sends a notification response to the controller with the new volume level.
 *
 * @param volume The new volume level, as a value between 0 and 127 (0x7f), inclusive.
 */
void volume_set_by_local_host(uint8_t volume);

/**
 * @brief Handles the A2DP events.
 *
 * This function is called when an A2DP event occurs. It handles various events such as connection state changes,
 * audio stream transmission state changes, audio codec configuration, A2DP initialization and deinitialization,
 * protocol service capabilities configuration, delay value setting and getting.
 *
 * @param event The A2DP event type.
 * @param p_param The event parameter, which is cast to the appropriate type based on the event type.
 */
void bt_av_hdl_a2d_evt(uint16_t event, void *p_param);

/**
 * @brief Handles the AVRCP Controller events.
 *
 * This function is called when an AVRCP Controller event occurs. It handles various events such as connection state changes,
 * passthrough responses, metadata responses, change notifications, remote features indication, and notification capabilities
 * of the peer device. For each event, it logs the event details and performs the appropriate action.
 *
 * @param event The AVRCP Controller event type.
 * @param p_param The event parameter, which is cast to the appropriate type based on the event type.
 */
void bt_av_hdl_avrc_ct_evt(uint16_t event, void *p_param);

/**
 * @brief Handles the AVRCP Target events.
 *
 * This function is called when an AVRCP Target event occurs. It handles various events such as passthrough commands,
 * absolute volume setting commands, notification registration, remote features indication, and others.
 * For each event, it logs the event details and performs the appropriate action.
 *
 * @param event The AVRCP Target event type.
 * @param p_param The event parameter, which is cast to the appropriate type based on the event type.
 */
void bt_av_hdl_avrc_tg_evt(uint16_t event, void *p_param);

/**
 * @brief Callback function for A2DP events.
 *
 * This function is called when an A2DP event occurs. It handles various events such as connection state changes,
 * audio stream transmission state changes, audio codec configuration, A2DP initialization and deinitialization,
 * protocol service capabilities configuration, delay value setting and getting. For each event, it dispatches the
 * event to the bt_av_hdl_a2d_evt function for handling. If an invalid event is received, it logs an error message.
 *
 * @param event The A2DP event type.
 * @param param The event parameter, which is cast to the appropriate type based on the event type.
 */
void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

/**
 * @brief Callback function for AVRCP Controller events.
 *
 * This function is called when an AVRCP Controller event occurs. It handles various events such as metadata responses,
 * connection state changes, passthrough responses, change notifications, remote features indication, and notification
 * capabilities of the peer device. For each event, it dispatches the event to the bt_av_hdl_avrc_ct_evt function for handling.
 * If an invalid event is received, it logs an error message.
 *
 * @param event The AVRCP Controller event type.
 * @param param The event parameter, which is cast to the appropriate type based on the event type.
 */
void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);

/**
 * @brief Callback function for AVRCP Target events.
 *
 * This function is called when an AVRCP Target event occurs. It handles various events such as connection state changes,
 * passthrough commands, absolute volume setting commands, notification registration, and player application setting commands.
 * For each event, it dispatches the event to the bt_av_hdl_avrc_tg_evt function for handling. If an invalid event is received,
 * it logs an error message.
 *
 * @param event The AVRCP Target event type.
 * @param param The event parameter, which is cast to the appropriate type based on the event type.
 */
void bt_app_rc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);

#endif /* __BT_APP_AV_H__*/
