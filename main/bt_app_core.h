#ifndef __BT_APP_CORE_H__
#define __BT_APP_CORE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/xtensa_api.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/dac_continuous.h"
#include "esp_gap_bt_api.h"
#include "freertos/ringbuf.h"

#define RINGBUF_MAX_WATER_LEVEL (32 * 1024)
#define RINGBUF_PREFETCH_WATER_LEVEL (20 * 1024)

#define BT_APP_CORE_TAG "BT_APP_CORE"

#define BT_APP_SIG_WORK_DISPATCH (0x01)

/* handler for application task */
typedef enum
{
    PROCESSING,  /* ringbuffer is buffering incoming audio data, I2S is working */
    PREFETCHING, /* ringbuffer is buffering incoming audio data, I2S is waiting */
    DROPPING     /* ringbuffer is not buffering (dropping) incoming audio data, I2S is working */
} ringbuffer_mode_t;

typedef void (*bt_app_event_callback_t)(uint16_t event, void *param);

typedef struct
{
    uint16_t signal;                  /*!< signal to bt_app_task */
    uint16_t event;                   /*!< message event id */
    bt_app_event_callback_t callback; /*!< context switch callback */
    void *param;                      /*!< parameter area needs to be last */
} bt_app_message_t;

void handle_bluetooth_app_task(void *arg);

void handle_i2s_task(void *arg);

bool send_bluetooth_app_message(bt_app_message_t *msg);

void dispatch_bluetooth_app_work(bt_app_message_t *msg);

typedef void (*copy_bluetooth_app_callback_t)(void *p_dest, void *p_src, int len);

bool dispatch_bluetooth_app_work_with_callback(bt_app_event_callback_t p_cback, uint16_t event, void *p_params, int param_len, copy_bluetooth_app_callback_t p_copy_cback);

void start_bluetooth_app_task(void);

void shut_down_bluetooth_app_task(void);

void start_i2s_task(void);

void shut_down_i2s_task(void);

size_t write_to_ringbuffer(const uint8_t *data, size_t size);

#endif /* __BT_APP_CORE_H__ */
