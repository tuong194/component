#ifndef GCOOL_H__
#define GCOOL_H__

#include "rd_uart.h"
#include "esp_err.h"

typedef void (*dev_rsp_cloud)(uint8_t func_code, uint8_t value);

esp_err_t gcool_init_product_cmd(uint8_t DP_IP, uint8_t data_type, uint16_t data_length, uint8_t* func_cmd);
esp_err_t gcool_init_basic_cmd(uint8_t ver_module, uint8_t cmd_word, uint8_t length, uint8_t* data);

esp_err_t gcool_func_lamp(uint8_t on_off);
esp_err_t gcool_func_countdown_timer(uint8_t time_h);
esp_err_t gcool_func_fan_switch(uint8_t on_off);
esp_err_t gcool_func_fan_mode(uint8_t fan_mode);
esp_err_t gcool_func_wind_speed(uint8_t level);
esp_err_t gcool_func_direction_fan(uint8_t direc_val);
esp_err_t gcool_func_fan_countdown_left(uint32_t time_m);
esp_err_t gcool_func_work_mode(uint8_t mode);
esp_err_t gcool_func_colour_data(uint8_t* data, uint8_t length);
esp_err_t gcool_func_dreamlight_scene(uint8_t* data, uint8_t length);

uint8_t gcool_get_status_wifi(void);
void gcool_set_status_wifi(uint8_t stt);
void gcool_init(const gpio_num_t tx_pin, const gpio_num_t rx_pin, const uart_port_t port_num);
void gcool_register_func_rsp_cloud(dev_rsp_cloud cb);


#endif
