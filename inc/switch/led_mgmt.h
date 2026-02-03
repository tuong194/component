#ifndef _LED_MANAGER_H__
#define _LED_MANAGER_H__

#include "esp_err.h"
#include "sdk_def.h"

#define FREQ_LED 25000
#define ALL_ELE 0xff


#if NUM_ELEMENT == 1
#define MAX_NUM_LED_OBJECT (NUM_ELEMENT * 4 + 1)
    enum rd_led
    {
        LED_1 = ELE_1,  
        LED_WIFI = 0xff
    }; 
#elif NUM_ELEMENT == 2
#define MAX_NUM_LED_OBJECT (NUM_ELEMENT * 2 + 2)
    enum rd_led
    {
        LED_1 = ELE_1,  
        LED_2 = ELE_2,
        LED_WIFI = 0xff      
    };
#elif NUM_ELEMENT == 3
#define MAX_NUM_LED_OBJECT (NUM_ELEMENT * 2 + 1)
    enum rd_led
    {
        LED_1 = ELE_1,  
        LED_2 = ELE_2,
        LED_3 = ELE_3,
        LED_WIFI = 0xff      
    };

#elif NUM_ELEMENT == 4
#define MAX_NUM_LED_OBJECT (NUM_ELEMENT * 2 + 1)
    enum rd_led
    {
        LED_1 = ELE_1, 
        LED_2 = ELE_3,
        LED_3 = ELE_4,
        LED_4 = ELE_2,      
        LED_WIFI = 0xff
    };
#endif

typedef uint8_t (*get_state_ele_cb_t)(uint8_t element);

void led_mgmt_init(void);
void led_mgmt_register_get_state_cb(get_state_ele_cb_t cb);

esp_err_t led_mgmt_set_state(uint8_t element, uint8_t state);
esp_err_t led_mgmt_set_dim(uint8_t element, uint8_t onoff, uint8_t dim_value);
esp_err_t led_mgmt_set_color(uint8_t element, uint8_t onoff, uint8_t red, uint8_t green, uint8_t blue);
esp_err_t led_mgmt_set_wifi_color(uint8_t stt);

uint8_t led_mgmt_get_dim_onoff(uint8_t element, uint8_t onoff);
uint8_t led_mgmt_get_red_color(uint8_t element, uint8_t onoff);
uint8_t led_mgmt_get_green_color(uint8_t element, uint8_t onoff);
uint8_t led_mgmt_get_blue_color(uint8_t element, uint8_t onoff);

void led_mgmt_blink_delay(uint8_t ele, uint8_t num_cycle, uint16_t time_delay_ms);
void led_mgmt_set_blink(uint8_t ele, uint8_t num_cycle, uint32_t time_tongle_ms);
void led_mgmt_scan_blink(void);

#endif /* */
