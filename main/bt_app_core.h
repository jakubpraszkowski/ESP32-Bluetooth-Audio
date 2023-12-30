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
#include "freertos/ringbuf.h"

#include "esp_log.h"

#include "driver/dac_continuous.h"

#define RINGBUF_MAX_BYTES_BUFFER (32 * 1024)
#define RINGBUF_PREFETCH_START (20 * 1024)

#define BT_APP_CORE_TAG "BT_APP_CORE"

/* signal for `bt_app_work_dispatch` */
#define BT_APP_SIG_WORK_DISPATCH (0x01)

/**
 * @brief Typedef for a callback function used in the Bluetooth application.
 *
 * This is a function pointer type that points to a function taking a uint16_t event and a void pointer parameter,
 * and returning void. This type is used to define the callback functions that handle various Bluetooth events in
 * the application.
 *
 * @param event The event type.
 * @param param The event parameter.
 */
typedef void (*bt_app_cb_t)(uint16_t event, void *param);

/**
 * @brief Typedef for a callback function used to copy data in the Bluetooth application.
 *
 * This is a function pointer type that points to a function taking a destination pointer, a source pointer,
 * and a length, and returning void. This type is used to define the callback functions that handle copying
 * data in the application.
 *
 * @param p_dest Pointer to the destination where the data should be copied.
 * @param p_src Pointer to the source from where the data should be copied.
 * @param len The length of the data to be copied.
 */
typedef void (*bt_app_copy_cb_t)(void *p_dest, void *p_src, int len);

/* modes of ringbuffer */
typedef enum
{
    PROCESSING,  /*!< ringbuffer is buffering incoming audio data, I2S is working */
    PREFETCHING, /*!< ringbuffer is buffering incoming audio data, I2S is waiting */
    DROPPING     /*!< ringbuffer is not buffering (dropping) incoming audio data, I2S is working */
} Ringbuffer_mode;

/* message to be sent */
typedef struct
{
    uint16_t sig;   /*!< signal to bt_app_task */
    uint16_t event; /*!< message event id */
    bt_app_cb_t cb; /*!< context switch callback */
    void *param;    /*!< parameter area needs to be last */
} bt_app_msg_t;

/**
 * @brief Sends a message to the Bluetooth application task queue.
 *
 * This function sends a message to the task queue of the Bluetooth application.
 * If the message is NULL or the queue is full, it logs an error and returns false.
 *
 * @param msg Pointer to the message to be sent. This should point to a variable of type bt_app_msg_t.
 * @return Returns true if the message was successfully sent to the queue, false otherwise.
 */
bool bt_app_send_msg(bt_app_msg_t *msg);

/**
 * @brief Dispatches a message to its associated callback function.
 *
 * This function takes a message and, if the message has an associated callback function,
 * it calls that function with the event and parameter stored in the message.
 *
 * @param msg Pointer to the message to be dispatched. This should point to a variable of type bt_app_msg_t.
 */
void bt_app_work_dispatched(bt_app_msg_t *msg);

/**
 * @brief Handles the Bluetooth application task.
 *
 * This function runs an infinite loop that continuously receives messages from the task queue
 * and dispatches them to their associated callback functions. If a message has an associated
 * parameter, it is freed after the message is handled.
 *
 * @param arg Pointer to the argument for the task. This is not used in the function and can be NULL.
 */
void bt_app_task_handler(void *arg);

/**
 * @brief Handles the I2S task.
 *
 * This function runs an infinite loop that continuously checks if a semaphore is available.
 * If the semaphore is available, it receives data from a ring buffer and writes it to the I2S DMA transmit buffer.
 * If the ring buffer underflows, it changes the ring buffer mode to PREFETCHING and breaks the loop.
 * After writing the data, it returns the item back to the ring buffer.
 *
 * @param arg Pointer to the argument for the task. This is not used in the function and can be NULL.
 */
void bt_i2s_task_handler(void *arg);

/**
 * @brief Dispatches a work event to the Bluetooth application task.
 *
 * This function creates a message with the given event and callback function, and optionally a parameter.
 * If a parameter is provided and a copy callback function is also provided, the copy callback function
 * is used to copy the parameter data. The message is then sent to the Bluetooth application task queue.
 *
 * @param p_cback The callback function to be associated with the event.
 * @param event The event to be dispatched.
 * @param p_params Pointer to the parameter data to be associated with the event. This can be NULL if no parameter data is needed.
 * @param param_len The length of the parameter data in bytes. This should be 0 if no parameter data is provided.
 * @param p_copy_cback The copy callback function to be used to copy the parameter data. This can be NULL if no deep copy is needed.
 * @return Returns true if the message was successfully sent to the queue, false otherwise.
 */
bool bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback);

/**
 * @brief Starts up the Bluetooth application task.
 *
 * This function creates a queue for Bluetooth application messages and starts the Bluetooth application task.
 * The task runs the bt_app_task_handler function in a new task named "BtAppTask".
 */
void bt_app_task_start_up(void);

/**
 * @brief Shuts down the Bluetooth application task.
 *
 * This function deletes the Bluetooth application task and its associated queue if they exist.
 * After deletion, the task handle and queue handle are set to NULL.
 */
void bt_app_task_shut_down(void);

/**
 * @brief Starts up the I2S task.
 *
 * This function sets the ring buffer mode to PREFETCHING, creates a binary semaphore for I2S writing,
 * creates a ring buffer for I2S data, and starts the I2S task. The task runs the bt_i2s_task_handler
 * function in a new task named "BtI2STask". If the semaphore or ring buffer creation fails, it logs an error and returns.
 */
void bt_i2s_task_start_up(void);

/**
 * @brief Shuts down the task.
 *
 * This function deletes the I2S task, its associated ring buffer, and semaphore if they exist.
 * After deletion, the task handle, ring buffer, and semaphore are set to NULL.
 */
void bt_i2s_task_shut_down(void);

/**
 * @brief Writes data to the ring buffer.
 *
 * This function writes a given amount of data to the ring buffer. If the ring buffer is in DROPPING mode,
 * it drops the packet and checks if the ring buffer data has decreased enough to switch back to PROCESSING mode.
 * If the ring buffer is in PREFETCHING mode, it checks if the ring buffer data has increased enough to switch to PROCESSING mode.
 * If the ring buffer overflows, it switches to DROPPING mode.
 *
 * @param data Pointer to the data to be written to the ring buffer.
 * @param size The size of the data in bytes.
 * @return Returns the size of the data written if the write was successful, 0 otherwise.
 */
size_t write_ringbuf(const uint8_t *data, size_t size);

/**
 * @brief Callback function for GAP events.
 *
 * This function is called when a GAP event occurs. It handles various events such as authentication completion,
 * GAP mode changes, ACL connection completion, and ACL disconnection completion. For each event, it logs the
 * appropriate information. If an invalid event is received, it logs the event number.
 *
 * @param event The GAP event type.
 * @param param The event parameter, which is cast to the appropriate type based on the event type.
 */
void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

/**
 * @brief Handles the Bluetooth stack events.
 *
 * This function is called when a Bluetooth stack event occurs. It handles various events such as stack initialization.
 * For the stack initialization event, it sets the device name, registers callbacks for GAP, AVRCP Controller, AVRCP Target,
 * and A2DP, initializes AVRCP Controller and Target, sets the event capabilities for AVRCP Target, initializes A2DP sink,
 * gets the default delay value for A2DP sink, and sets the device to be discoverable and connectable. If an invalid event
 * is received, it logs an error message.
 *
 * @param event The Bluetooth stack event type.
 * @param p_param The event parameter, which is not used in this function.
 */
void bt_av_hdl_stack_evt(uint16_t event, void *p_param);

#endif /* __BT_APP_CORE_H__ */
