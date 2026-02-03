#include "gcool.h"
#include "gcool_def.h"
#include "esp_log.h"
#include "esp_err.h"
#include <string.h>

static const char* GCOOL_TAG = "GCOOL";

static dev_rsp_cloud dev_rsp_cloud_control = NULL;

#define min(a,b)            ((a) <= (b) ? (a) : (b))
#define max(a,b)            ((a) >= (b) ? (a) : (b))

static esp_err_t rsp_func_lamp_handle(uint8_t* data, uint8_t length);
static esp_err_t rsp_func_countdown_timer_handle(uint8_t* data, uint8_t length);
static esp_err_t rsp_func_fan_switch_handle(uint8_t* data, uint8_t length);
static esp_err_t rsp_func_fan_mode_handle(uint8_t* data, uint8_t length);
static esp_err_t rsp_func_wind_speed_handle(uint8_t* data, uint8_t length);
static esp_err_t rsp_func_direction_fan_handle(uint8_t* data, uint8_t length);

void gcool_register_func_rsp_cloud(dev_rsp_cloud cb) {
    if (cb) dev_rsp_cloud_control = cb;
}

/*==============================PRODUCT FUNCTION CMD=================================*/
esp_err_t gcool_func_lamp(uint8_t on_off)
{
    if (on_off > 0) on_off = 1;
    ESP_LOGI(GCOOL_TAG, "control LAMP: %02x", on_off);
    return gcool_init_product_cmd(LAMP, 0x01, 1, &on_off);
}

esp_err_t gcool_func_countdown_timer(uint8_t time_h) // max 1 day
{
    uint32_t time_s = time_h * 3600;
    time_s = min(time_s, 86400);
    ESP_LOGI(GCOOL_TAG, "set countdown: %lu s", time_s);
    uint8_t data[4] = { 0 };
    data[0] = (time_s >> 24) & 0xff;
    data[1] = (time_s >> 16) & 0xff;
    data[2] = (time_s >> 8) & 0xff;
    data[3] = (time_s) & 0xff;
    printf("GCOOL: time %02X-%02X-%02X-%02X\n", data[0], data[1], data[2], data[3]);
    return gcool_init_product_cmd(COUNTDOWN, 0x02, 4, data);
}

esp_err_t gcool_func_fan_switch(uint8_t on_off)
{
    ESP_LOGI(GCOOL_TAG, "control onoff fan: %02x", on_off);
    return gcool_init_product_cmd(FAN_SWITCH, 0x01, 1, &on_off);
}

esp_err_t gcool_func_fan_mode(uint8_t fan_mode)
{
    if (fan_mode > NATURE) fan_mode = NATURE;
    ESP_LOGI(GCOOL_TAG, "control mode fan: %02x", fan_mode);
    return gcool_init_product_cmd(FAN_MODE, 0x04, 1, &fan_mode);
}

esp_err_t gcool_func_wind_speed(uint8_t level)
{ // 0x01->0x05
    level = max(level, 0x01);
    level = min(level, 0x05);
    uint8_t data_send[4] = { 0x00, 0x00, 0x00, level };
    ESP_LOGI(GCOOL_TAG, "control wind speed: %02x", level);
    return gcool_init_product_cmd(WIND_SPEED, 0x02, 4, data_send);
}

esp_err_t gcool_func_direction_fan(uint8_t direc_val)
{
    if (direc_val > CCW) direc_val = CCW;
    ESP_LOGI(GCOOL_TAG, "control direction fan: %02x", direc_val);
    return gcool_init_product_cmd(DIRECTION_FAN, 0x04, 1, &direc_val);
}

esp_err_t gcool_func_fan_countdown_left(uint32_t time_m)
{ // max 540 minute
    time_m = min(time_m, 540);
    ESP_LOGI(GCOOL_TAG, "set countdown left: %lu minute", time_m);
    uint8_t data[4] = { 0 };
    data[0] = (time_m >> 24) & 0xff;
    data[1] = (time_m >> 16) & 0xff;
    data[2] = (time_m >> 8) & 0xff;
    data[3] = (time_m) & 0xff;
    printf("GCOOL: time minute %02X-%02X-%02X-%02X \n", data[0], data[1], data[2], data[3]);
    return gcool_init_product_cmd(FAN_COUNTDOWN_LEFT, 0x02, 4, data);
}

esp_err_t gcool_func_work_mode(uint8_t mode)
{
    mode = 0x00; // default
    ESP_LOGI(GCOOL_TAG, "work mode: %02x", mode);
    return gcool_init_product_cmd(WORK_MODE, 0x04, 1, &mode);
}

// cần xem lai
esp_err_t gcool_func_colour_data(uint8_t* data, uint8_t length)
{ // 25 16 37 ==> 32 35 20 31 36 20 33 37
    ESP_LOGI(GCOOL_TAG, "set colour data");
    uint8_t length_send = length * 2 + (length - 1);
    uint8_t* data_send = malloc(length_send);
    for (size_t i = 0; i < length; i++)
    {
        data_send[i * 3] = 0x30 + (data[i] >> 4 & 0x0f);
        data_send[i * 3 + 1] = 0x30 + (data[i] & 0x0f);
        if (i == (length - 1))
            break;
        data_send[i * 3 + 2] = 0x20;
    }
    esp_err_t err = gcool_init_product_cmd(COLOUR, 0x03, length_send, data_send);
    free(data_send);
    return err;
}

esp_err_t gcool_func_dreamlight_scene(uint8_t* data, uint8_t length)
{
    ESP_LOGI(GCOOL_TAG, "dreamlight scene");
    return gcool_init_product_cmd(DREAMLIGHT_SCENE, 0x00, length, data);
}

//HANDLE DATA 
esp_err_t Handle_Product_Function(uint8_t* data)
{
    esp_err_t err = ESP_OK;
    uint8_t DP_ID = data[0];
    uint16_t data_length = (data[2] << 8) | data[3];
    uint8_t* data_rec = malloc(data_length);
    memcpy(data_rec, &data[4], data_length);
    switch (DP_ID)
    {
    case LAMP:
        rsp_func_lamp_handle(data_rec, data_length);
        break;
    case COUNTDOWN:
        rsp_func_countdown_timer_handle(data_rec, data_length);
        break;
    case FAN_SWITCH:
        rsp_func_fan_switch_handle(data_rec, data_length);
        break;
    case FAN_MODE:
        rsp_func_fan_mode_handle(data_rec, data_length);
        break;
    case WIND_SPEED:
        rsp_func_wind_speed_handle(data_rec, data_length);
        break;
    case DIRECTION_FAN:
        rsp_func_direction_fan_handle(data_rec, data_length);
        break;

    case FAN_COUNTDOWN_LEFT:
        break;
    case WORK_MODE:
        break;
    case COLOUR:
        break;
    case DREAMLIGHT_SCENE:
        break;
    default:
        ESP_LOGE(GCOOL_TAG, "Unknown DP ID");
        err = ESP_FAIL;
        break;
    }
    free(data_rec);
    return err;
}

static esp_err_t rsp_func_lamp_handle(uint8_t* data, uint8_t length) {
    esp_err_t err = ESP_OK;
    printf("LAMP data rsp: %2X\n", data[0]);
    uint8_t rsp = data[0];
    if (dev_rsp_cloud_control) dev_rsp_cloud_control(LAMP, rsp);
    return err;
}

static esp_err_t rsp_func_countdown_timer_handle(uint8_t* data, uint8_t length) {
    esp_err_t err = ESP_OK;
    uint32_t time_s = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    uint8_t time_h = time_s / 3600;
    printf("COUNTDOWN data rsp: %2X hour\n", time_h);
    return err;
}

static esp_err_t rsp_func_fan_switch_handle(uint8_t* data, uint8_t length) {
    esp_err_t err = ESP_OK;
    printf("FAN_SWITCH data rsp: %2X\n", data[0]);
    uint8_t rsp = data[0];
    if (dev_rsp_cloud_control) dev_rsp_cloud_control(FAN_SWITCH, rsp);
    return err;
}

static esp_err_t rsp_func_fan_mode_handle(uint8_t* data, uint8_t length) {
    esp_err_t err = ESP_OK;
    printf("FAN_MODE data rsp: %2X\n", data[0]);
    uint8_t rsp = data[0];
    if (dev_rsp_cloud_control) dev_rsp_cloud_control(FAN_MODE, rsp);
    return err;
}

static esp_err_t rsp_func_wind_speed_handle(uint8_t* data, uint8_t length) {
    esp_err_t err = ESP_OK;
    printf("WIND_SPEED data rsp: %2X\n", data[3]);
    uint8_t rsp = data[3];
    if (dev_rsp_cloud_control) dev_rsp_cloud_control(WIND_SPEED, rsp);
    return err;
}

static esp_err_t rsp_func_direction_fan_handle(uint8_t* data, uint8_t length) {
    esp_err_t err = ESP_OK;
    printf("DIRECTION_FAN data rsp: %2X\n", data[0]);
    uint8_t rsp = data[0];
    if (dev_rsp_cloud_control) dev_rsp_cloud_control(DIRECTION_FAN, rsp);
    return err;
}