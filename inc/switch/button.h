#ifndef RD_BUTTON_H__
#define RD_BUTTON_H__

#include "driver/gpio.h"
#include "stdint.h"
#include "stdbool.h"
#include "esp_err.h"

typedef void *button_handle_t;
typedef void (* button_cb_t)(void *button_handle, void *usr_data);

typedef struct {
    gpio_num_t gpio_num;              /**< num of gpio */
    uint8_t active_level;          /**< gpio level when press down */
    bool disable_pull;            /**< disable internal pull or not */
} button_gpio_config_t;

typedef struct{
    uint16_t press_time; 
    uint16_t keep_time; 
    uint16_t long_keep_time;
    button_gpio_config_t button_gpio_config;
}button_config_t;

typedef enum{
    BUTTON_EVENT_PRESS,                 // Button press
    BUTTON_EVENT_KEEPING,               // Button is being held
    BUTTON_EVENT_RELEASE_KEEPING,       // Release after holding
    BUTTON_EVENT_LONG_KEEPING,          // Long press
    BUTTON_EVENT_RELEASE_LONG_KEEPING,  // Release after long press
    BUTTON_EVENT_MAX,                   // Maximum number of events
    BUTTON_EVENT_NONE_PRESS             // No button press
}button_event_t;

/**
 * @brief Configure GPIO pins to activate the button
 * 
 * @param RST_TOUCH_PIN 
 * @param active_level 
 */
void button_reset_touch_pin_config(gpio_num_t RST_TOUCH_PIN, uint8_t active_level);

/**
 * @brief Initialize button configuration
 * 
 * @param config Button configuration parameters
 * @return button_handle
 */
button_handle_t button_create_button_gpio(const button_config_t *config);

/**
 * @brief Register callback function for button event
 * 
 * @param btn_handle handle corresponding to button event
 * @param event event 
 * @param cb callback corresponding to event
 * @param usr_data custom data
 * @return ESP_OK    register callback successfully
 *         ESP_FAIL  register callback fail
 */
esp_err_t button_register_callback(button_handle_t btn_handle, button_event_t event, button_cb_t cb, void *usr_data);

/**
 * @brief Get event corresponding to handle
 * 
 * @param btn_handle handle 
 * @return button_event
 */
button_event_t button_get_event(button_handle_t btn_handle);

esp_err_t button_delete_handle(button_handle_t btn_handle);

#endif /*  */

