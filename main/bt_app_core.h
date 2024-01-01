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

#define RINGBUF_HIGHEST_WATER_LEVEL (32 * 1024)
#define RINGBUF_PREFETCH_WATER_LEVEL (20 * 1024)

#define BT_APP_CORE_TAG "BT_APP_CORE"

#define BT_APP_SIG_WORK_DISPATCH (0x01)

/* handler for application task */
typedef enum
{
    PROCESSING,  /* ringbuffer is buffering incoming audio data, I2S is working */
    PREFETCHING, /* ringbuffer is buffering incoming audio data, I2S is waiting */
    DROPPING     /* ringbuffer is not buffering (dropping) incoming audio data, I2S is working */
} Ringbuffer_mode;

typedef void (*bt_app_cb_t)(uint16_t event, void *param);

typedef struct
{
    uint16_t sig;   /*!< signal to bt_app_task */
    uint16_t event; /*!< message event id */
    bt_app_cb_t cb; /*!< context switch callback */
    void *param;    /*!< parameter area needs to be last */
} bt_app_msg_t;

void bt_app_task_handler(void *arg);

void i2s_task_handler(void *arg);

bool bt_app_send_msg(bt_app_msg_t *msg);

void bt_app_work_dispatched(bt_app_msg_t *msg);

typedef void (*bt_app_copy_cb_t)(void *p_dest, void *p_src, int len);

bool bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback);

void bt_app_task_start_up(void);

void bt_app_task_shut_down(void);

void i2s_task_start_up(void);

void i2s_task_shut_down(void);

size_t write_ringbuf(const uint8_t *data, size_t size);

#endif /* __BT_APP_CORE_H__ */
