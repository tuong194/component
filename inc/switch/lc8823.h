#ifndef _LC_8823_H__
#define _LC_8823_H__

#include "stdint.h"
#include "esp_err.h"
#include "driver/gpio.h"

#define LIGHT_MAX    0xEF  //50%
#define LIGHT_MIN    0xE2  //2/31 = 6%



typedef struct{
    gpio_num_t clk_pin;
    gpio_num_t data_pin;
    uint16_t freq; // max 27kHz
}lc8823_config_t;

typedef struct {
    uint8_t lum;
    uint8_t blue;
    uint8_t green;
    uint8_t red;
}led_data_t;

typedef struct{
    led_data_t ON;
    led_data_t OFF;
}led_value_onoff_t;

/*======== LED Object=======*/
typedef struct{
    led_value_onoff_t value;
    uint8_t state;
    uint8_t index; // vi tri led trong frame
    uint8_t element; // dinh danh led (ele1, ele2..., led_wifi)
}led_t;

void lc8823_init(lc8823_config_t *config);

esp_err_t lc8823_init_frame_buffer(const uint8_t num_led);
esp_err_t lc8823_update_frame_buffer(uint8_t idx, led_data_t *led_data);
esp_err_t lc8823_send_frame_buffer(void);

void      lc8823_load_value(led_t *led, uint8_t onoff, uint8_t dim_value, uint8_t red, uint8_t green, uint8_t blue);
esp_err_t lc8823_set_state(led_t *led, uint8_t state);
esp_err_t lc8823_set_color(led_t *led, uint8_t onoff, uint8_t red, uint8_t green, uint8_t blue);
esp_err_t lc8823_set_dim(led_t *led, uint8_t onoff, uint8_t dim_value);

uint8_t get_dim_onoff(led_t *led, uint8_t onoff);
uint8_t get_red_color(led_t *led, uint8_t onoff);
uint8_t get_green_color(led_t *led, uint8_t onoff);
uint8_t get_blue_color(led_t *led, uint8_t onoff);

#endif /*  */

