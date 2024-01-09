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

/**
 * @brief Maximum water level for the ring buffer.
 *
 * This constant defines the maximum amount of data (in bytes) that can be stored in the ring buffer before
 * it is considered full.
 */
#define RINGBUF_MAX_WATER_LEVEL (32 * 1024)

/**
 * @brief Prefetch water level for the ring buffer.
 *
 * This constant defines the amount of data (in bytes) that should be prefetched into the ring buffer.
 */
#define RINGBUF_PREFETCH_WATER_LEVEL (20 * 1024)

/**
 * @brief Tag for the Bluetooth application core.
 *
 * This constant is used as a tag for logging messages related to the Bluetooth application core.
 */
#define BT_APP_CORE_TAG "BT_APP_CORE"

/**
 * @brief Signal for dispatching work to the Bluetooth application task.
 *
 * This constant is used as a signal to dispatch work to the Bluetooth application task.
 */
#define BT_APP_SIG_WORK_DISPATCH (0x01)

/**
 * @brief Enumeration for the ring buffer modes.
 *
 * This enum defines the possible modes for the ring buffer. The modes include PROCESSING, where the ring buffer
 * is buffering incoming audio data and the I2S is working, PREFETCHING, where the ring buffer is buffering
 * incoming audio data and the I2S is waiting, and DROPPING, where the ring buffer is not buffering (dropping)
 * incoming audio data and the I2S is working.
 */
typedef enum
{
    PROCESSING,  /* ringbuffer is buffering incoming audio data, I2S is working */
    PREFETCHING, /* ringbuffer is buffering incoming audio data, I2S is waiting */
    DROPPING     /* ringbuffer is not buffering (dropping) incoming audio data, I2S is working */
} ringbuffer_mode_t;

/**
 * @brief Type definition for a Bluetooth application event callback function.
 *
 * This type defines a function pointer for a callback function that is called when a Bluetooth application
 * event occurs. The function takes an event type and a pointer to the event parameters.
 */
typedef void (*bt_app_event_callback_t)(uint16_t event, void *param);

/**
 * @brief Structure for a Bluetooth application message.
 *
 * This struct defines a message that can be sent to the Bluetooth application task. It includes a signal to
 * dispatch work to the task, an event id, a callback function for context switching, and a parameter area.
 */
typedef struct
{
    uint16_t signal;                  /*!< signal to bt_app_task */
    uint16_t event;                   /*!< message event id */
    bt_app_event_callback_t callback; /*!< context switch callback */
    void *param;                      /*!< parameter area needs to be last */
} bt_app_message_t;

/**
 * @brief Type definition for a Bluetooth application copy callback function.
 *
 * This type defines a function pointer for a callback function that is called to copy data in the Bluetooth application.
 * The function takes a destination pointer, a source pointer, and a length indicating the number of bytes to copy.
 *
 * @param p_dest Pointer to the destination where the data should be copied.
 * @param p_src Pointer to the source from where the data should be copied.
 * @param len The number of bytes to copy from the source to the destination.
 */
typedef void (*copy_bluetooth_app_callback_t)(void *p_dest, void *p_src, int len);

/**
 * @brief Sends a message to the Bluetooth application task.
 *
 * This function sends a message to the Bluetooth application task via a queue. If the message is NULL, 
 * or if the queue send operation fails, it returns false. Otherwise, it returns true.
 *
 * @param message Pointer to the message to be sent. This should be a pointer to a bt_app_message_t structure.
 * @return true if the message was successfully sent, false otherwise.
 */
bool send_bluetooth_app_message(bt_app_message_t *msg);

/**
 * @brief Dispatches work to the Bluetooth application.
 *
 * This function dispatches work to the Bluetooth application by calling the callback function specified in the message.
 * If the callback function is NULL, it does nothing.
 *
 * @param msg Pointer to the message containing the work to be dispatched. This should be a pointer to a bt_app_message_t structure.
 */
void dispatch_bluetooth_app_work(bt_app_message_t *msg);

/**
 * @brief Handles the Bluetooth application task.
 *
 * This function runs in an infinite loop, receiving messages from the Bluetooth application task queue and 
 * dispatching work based on the received messages. If a message with an unrecognized signal is received, 
 * it logs a warning. After handling a message, it frees the memory allocated for the message parameters.
 *
 * @param taskArgument Pointer to the task argument. This is not used in the function and can be NULL.
 */
void handle_bluetooth_app_task(void *arg);

/**
 * @brief Handles the I2S task.
 *
 * This function runs in an infinite loop, waiting for the semaphore to be given. When the semaphore is given, 
 * it enters another infinite loop where it receives data from the ring buffer and writes it to the DAC. 
 * If no data is received from the ring buffer, it changes the ring buffer mode to PREFETCHING and breaks 
 * the inner loop. After writing data to the DAC, it returns the data item to the ring buffer.
 *
 * @param taskArgument Pointer to the task argument. This is not used in the function and can be NULL.
 */
void handle_i2s_task(void *arg);

/**
 * @brief Dispatches work to the Bluetooth application with a callback.
 *
 * This function dispatches work to the Bluetooth application by creating a message and sending it to the 
 * Bluetooth application task. The message includes a signal to dispatch work, an event id, and a callback 
 * function for context switching. If parameters are provided, they are copied into the message. If a copy 
 * callback is provided, it is used to copy the parameters. If the message is successfully sent, the function 
 * returns true. If the parameters cannot be copied, or the message cannot be sent, it returns false.
 *
 * @param p_cback Pointer to the callback function to be used for context switching.
 * @param event The event id for the message.
 * @param p_params Pointer to the parameters to be copied into the message.
 * @param param_len The length of the parameters in bytes.
 * @param p_copy_cback Pointer to the callback function to be used for copying the parameters.
 * @return true if the message was successfully sent, false otherwise.
 */
bool dispatch_bluetooth_app_work_with_callback(bt_app_event_callback_t p_cback, uint16_t event, void *p_params, int param_len, copy_bluetooth_app_callback_t p_copy_cback);

/**
 * @brief Starts the Bluetooth application task.
 *
 * This function creates a queue for the Bluetooth application task and then creates the task itself. 
 * The task is created with a stack size of 3072 bytes, a priority of 10, and the handle is stored in 
 * the global variable s_bt_app_task_handle.
 */
void start_bluetooth_app_task(void);

/**
 * @brief Shuts down the Bluetooth application task.
 *
 * This function deletes the Bluetooth application task and the associated queue. If the task handle 
 * or the queue handle is not NULL, it deletes the task or the queue and sets the handle to NULL.
 */
void shut_down_bluetooth_app_task(void);

/**
 * @brief Starts the I2S task.
 *
 * This function sets the ringbuffer mode to PREFETCHING, creates a binary semaphore for I2S write operations, 
 * creates a ring buffer for I2S data, and then creates the I2S task itself. The task is created with a stack 
 * size of 2048 bytes, a priority of (configMAX_PRIORITIES - 3), and the handle is stored in the global 
 * variable s_bt_i2s_task_handle. If the semaphore or the ring buffer cannot be created, it logs an error 
 * and returns without creating the task.
 */
void start_i2s_task(void);

/**
 * @brief Shuts down the I2S task.
 *
 * This function deletes the I2S task, the associated ring buffer, and the semaphore used for I2S write operations. 
 * If the task handle, the ring buffer handle, or the semaphore handle is not NULL, it deletes the task, the ring buffer, 
 * or the semaphore and sets the handle to NULL.
 */
void shut_down_i2s_task(void);

/**
 * @brief Writes data to the ring buffer.
 *
 * This function writes the specified data to the ring buffer. If the ring buffer mode is DROPPING, it checks 
 * the item size in the ring buffer and switches the mode to PROCESSING if the item size is less than or equal 
 * to RINGBUF_PREFETCH_WATER_LEVEL. If the ring buffer mode is not DROPPING, it tries to send the data to the 
 * ring buffer. If the send operation fails, it switches the mode to DROPPING. If the ring buffer mode is 
 * PREFETCHING, it checks the item size in the ring buffer and switches the mode to PROCESSING and gives the 
 * semaphore if the item size is greater than or equal to RINGBUF_PREFETCH_WATER_LEVEL.
 *
 * @param data Pointer to the data to be written to the ring buffer.
 * @param size The size of the data in bytes.
 * @return The number of bytes written to the ring buffer, or 0 if the write operation failed.
 */
size_t write_to_ringbuffer(const uint8_t *data, size_t size);

#endif /* __BT_APP_CORE_H__ */
