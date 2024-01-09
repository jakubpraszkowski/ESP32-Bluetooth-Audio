#include "core.h"

/**
 * @brief Queue handle for the Bluetooth application task.
 *
 * This queue is used to send events to the Bluetooth application task.
 */
static QueueHandle_t s_bt_app_task_queue = NULL;

/**
 * @brief Task handle for the Bluetooth application task.
 *
 * This handle is used to manage the Bluetooth application task.
 */
static TaskHandle_t s_bt_app_task_handle = NULL;

/**
 * @brief Task handle for the Bluetooth I2S task.
 *
 * This handle is used to manage the Bluetooth I2S task.
 */
static TaskHandle_t s_bt_i2s_task_handle = NULL;

/**
 * @brief Ring buffer handle for the I2S data.
 *
 * This ring buffer is used to store the I2S data.
 */
static RingbufHandle_t s_ringbuf_i2s = NULL;

/**
 * @brief Semaphore handle for I2S write operations.
 *
 * This semaphore is used to synchronize I2S write operations.
 */
static SemaphoreHandle_t s_i2s_write_semaphore = NULL;

/**
 * @brief Mode for the ring buffer.
 *
 * This variable determines the processing mode for the ring buffer.
 */
static uint16_t ringbuffer_mode = PROCESSING;

/**
 * @brief External reference to the DAC continuous handle.
 *
 * This handle is used to manage the DAC continuous mode.
 */
extern dac_continuous_handle_t tx_chan;

bool send_bluetooth_app_message(bt_app_message_t *message)
{
    if (message == NULL)
    {
        return false;
    }

    if (xQueueSend(s_bt_app_task_queue, message, 10 / portTICK_PERIOD_MS) != pdTRUE)
    {
        ESP_LOGE(BT_APP_CORE_TAG, "%s xQueue send failed", __func__);
        return false;
    }
    return true;
}

void dispatch_bluetooth_app_work(bt_app_message_t *msg)
{
    if (msg->callback)
    {
        msg->callback(msg->event, msg->param);
    }
}

void handle_bluetooth_app_task(void *taskArgument)
{
    bt_app_message_t msg;

    while (true)
    {
        if (pdTRUE == xQueueReceive(s_bt_app_task_queue, &msg, (TickType_t)portMAX_DELAY))
        {
            ESP_LOGD(BT_APP_CORE_TAG, "%s, signal: 0x%x, event: 0x%x", __func__, msg.signal, msg.event);

            switch (msg.signal)
            {
            case BT_APP_SIG_WORK_DISPATCH:
                dispatch_bluetooth_app_work(&msg);
                break;
            default:
                ESP_LOGW(BT_APP_CORE_TAG, "%s, unhandled signal: %d", __func__, msg.signal);
                break;
            }

            if (msg.param)
            {
                free(msg.param);
            }
        }
    }
}

void handle_i2s_task(void *taskArgument)
{
    uint8_t *data = NULL;
    size_t item_size = 0;
    const size_t item_size_upto = 240 * 6;
    size_t bytes_written = 0;

    while (true)
    {
        if (pdTRUE == xSemaphoreTake(s_i2s_write_semaphore, portMAX_DELAY))
        {
            while (true)
            {
                item_size = 0;
                data = (uint8_t *)xRingbufferReceiveUpTo(s_ringbuf_i2s, &item_size, (TickType_t)pdMS_TO_TICKS(20), item_size_upto);
                if (item_size == 0)
                {
                    ringbuffer_mode = PREFETCHING;
                    break;
                }

                dac_continuous_write(tx_chan, data, item_size, &bytes_written, -1);
                vRingbufferReturnItem(s_ringbuf_i2s, (void *)data);
            }
        }
    }
}

bool dispatch_bluetooth_app_work_with_callback(bt_app_event_callback_t p_cback, uint16_t event, void *p_params, int param_len, copy_bluetooth_app_callback_t p_copy_cback)
{
    ESP_LOGD(BT_APP_CORE_TAG, "%s event: 0x%x, param len: %d", __func__, event, param_len);

    bt_app_message_t msg;
    memset(&msg, 0, sizeof(bt_app_message_t));

    msg.signal = BT_APP_SIG_WORK_DISPATCH;
    msg.event = event;
    msg.callback = p_cback;

    if (param_len == 0)
    {
        return send_bluetooth_app_message(&msg);
    }
    else if (p_params && param_len > 0)
    {
        if ((msg.param = malloc(param_len)) != NULL)
        {
            memcpy(msg.param, p_params, param_len);

            if (p_copy_cback)
            {
                p_copy_cback(msg.param, p_params, param_len);
            }
            return send_bluetooth_app_message(&msg);
        }
    }

    return false;
}

void start_bluetooth_app_task(void)
{
    s_bt_app_task_queue = xQueueCreate(10, sizeof(bt_app_message_t));
    xTaskCreate(handle_bluetooth_app_task, "BtAppTask", 3072, NULL, 10, &s_bt_app_task_handle);
}

void shut_down_bluetooth_app_task(void)
{
    if (s_bt_app_task_handle)
    {
        vTaskDelete(s_bt_app_task_handle);
        s_bt_app_task_handle = NULL;
    }
    if (s_bt_app_task_queue)
    {
        vQueueDelete(s_bt_app_task_queue);
        s_bt_app_task_queue = NULL;
    }
}

void start_i2s_task(void)
{
    ringbuffer_mode = PREFETCHING;
    if ((s_i2s_write_semaphore = xSemaphoreCreateBinary()) == NULL)
    {
        ESP_LOGE(BT_APP_CORE_TAG, "%s, Semaphore create failed", __func__);
        return;
    }
    if ((s_ringbuf_i2s = xRingbufferCreate(RINGBUF_MAX_WATER_LEVEL, RINGBUF_TYPE_BYTEBUF)) == NULL)
    {
        ESP_LOGE(BT_APP_CORE_TAG, "%s, ringbuffer create failed", __func__);
        return;
    }
    xTaskCreate(handle_i2s_task, "BtI2STask", 2048, NULL, configMAX_PRIORITIES - 3, &s_bt_i2s_task_handle);
}

void shut_down_i2s_task(void)
{
    if (s_bt_i2s_task_handle)
    {
        vTaskDelete(s_bt_i2s_task_handle);
        s_bt_i2s_task_handle = NULL;
    }
    if (s_ringbuf_i2s)
    {
        vRingbufferDelete(s_ringbuf_i2s);
        s_ringbuf_i2s = NULL;
    }
    if (s_i2s_write_semaphore)
    {
        vSemaphoreDelete(s_i2s_write_semaphore);
        s_i2s_write_semaphore = NULL;
    }
}

size_t write_to_ringbuffer(const uint8_t *data, size_t size)
{
    size_t item_size = 0;
    BaseType_t done = pdFALSE;

    if (ringbuffer_mode == DROPPING)
    {
        vRingbufferGetInfo(s_ringbuf_i2s, NULL, NULL, NULL, NULL, &item_size);
        if (item_size <= RINGBUF_PREFETCH_WATER_LEVEL)
        {
            ringbuffer_mode = PROCESSING;
        }
        return 0;
    }

    done = xRingbufferSend(s_ringbuf_i2s, (void *)data, size, (TickType_t)0);

    if (!done)
    {
        ringbuffer_mode = DROPPING;
    }

    if (ringbuffer_mode == PREFETCHING)
    {
        vRingbufferGetInfo(s_ringbuf_i2s, NULL, NULL, NULL, NULL, &item_size);
        if (item_size >= RINGBUF_PREFETCH_WATER_LEVEL)
        {
            ringbuffer_mode = PROCESSING;
            if (pdFALSE == xSemaphoreGive(s_i2s_write_semaphore))
            {
                ESP_LOGE(BT_APP_CORE_TAG, "semphore give failed");
            }
        }
    }

    return done ? size : 0;
}