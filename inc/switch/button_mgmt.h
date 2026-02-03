#ifndef BUTTON_MANAGER_H__
#define BUTTON_MANAGER_H__

#include "esp_event.h"

// extern BUTTON_EVENT_BASE
ESP_EVENT_DECLARE_BASE(BUTTON_EVENT_BASE);

#define CONFIG_PRESS_TIME_MS     100
#define CONFIG_KEEP_TIME_MS      1000
#define CONFIG_LONG_KEEP_TIME_MS 6000

#define CLOCK_TIME_SET_PAIR_K9B  (3*1000*1000 - CONFIG_KEEP_TIME_MS*1000)
#define CLOCK_TIME_KICK_OUT      (3*1000*1000 - CONFIG_KEEP_TIME_MS*1000)
#define CLOCK_TIME_OUT_KICK_OUT  (10*1000*1000 - CONFIG_KEEP_TIME_MS*1000)

/**
 * @brief enum button event
 * 
 */
typedef enum {
    EVENT_BUTTON_PRESS = 1,  // press key
    EVENT_BUTTON_PAIR_K9B,   // pair CT2C 
    EVENT_BUTTON_DELETE_ALL_K9B, // delete all CT2C with one button
    EVENT_BUTTON_CONFIG_WIFI, // config Wifi
    EVENT_BUTTON_KICK_OUT,  // hard reset
    EVENT_BUTTON_INC_COUNT_KICK_OUT, 
} btn_event_id_t;

/**
 * @brief config gpio button
 * 
 * @return ESP_OK configuration successful
 */
esp_err_t button_gpio_config(void);

/**
 * @brief this function is used to initialize events button
 * 
 * @param event_handler: callback event handling
 */
void btn_event_init(esp_event_handler_t event_handler);

#endif /*  */



