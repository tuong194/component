#include "lc8823.h"
#include "util.h"

#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "driver/spi_master.h"

static const char* TAG = "lc8823";

#define LED_DATA_DEFAULT     GPIO_NUM_21   
#define LED_CLK_DEFAULT      GPIO_NUM_20 
#define FREQ_LED_DEFAULT     25000

#define EN_COLOR     0xff
#define DIS_COLOR    0x00
#define START_FRAME  0x00
#define END_FRAME    0xff

// macro chuyển đổi độ sáng LED theo format của led lc8823
#define LIGHT_SET(dim_val)  (0xE0 | (dim_val))
#define min2(a,b)	 ((a) < (b) ? (a) : (b))

#define HSPI_HOST    SPI2_HOST
static spi_device_handle_t spi;

static uint8_t *frame_buffer = NULL;
static uint8_t frame_buffer_size = 0;
static lc8823_config_t lc8823_config = {
    .clk_pin = LED_CLK_DEFAULT,
    .data_pin = LED_DATA_DEFAULT,
    .freq = FREQ_LED_DEFAULT
};

static void lc8823_spi_init(gpio_num_t clk_pin, gpio_num_t data_pin, uint32_t freq)
{
    // Cấu hình bus SPI
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,      // MISO
        .mosi_io_num = data_pin,      // MOSI
        .sclk_io_num = clk_pin,       // SCLK
        .quadwp_io_num = -1,             // not use
        .quadhd_io_num = -1,             // not use
        .max_transfer_sz = 0 //SPI_BUF_MAX       
    };

    // Cấu hình thiết bị SPI
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = freq,        
        .mode = 0,         
        .spics_io_num = -1,   
        .queue_size = 2,
        .pre_cb = NULL,      // Không cần callback trước khi truyền
        .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_NO_DUMMY,
    };

    spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(HSPI_HOST, &devcfg, &spi);


    ESP_LOGI(TAG, "lc8823 spi init done!!");
}

static void spi_send_data(uint8_t *data_send, uint8_t length){
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = length*8; 
    if(t.length<=32){
        t.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA; 
        for(size_t i = 0; i<length; i++){
            t.tx_data[i] = data_send[i];
        }
    }else{
        t.tx_buffer = data_send; 
    }
          
    esp_err_t ret = spi_device_transmit(spi, &t);
    if (ret != ESP_OK) {
        ESP_LOGE("SPI", "SPI transaction failed: %s\n", esp_err_to_name(ret));
        return;
    }
}

inline void lc8823_calib_dim(uint8_t *dim){
    *dim = (*dim) * 31 / 100;
}

esp_err_t lc8823_init_frame_buffer(const uint8_t num_led){
    if(frame_buffer != NULL){
        free(frame_buffer);
        frame_buffer = NULL;
        frame_buffer_size = 0;
    }
    frame_buffer_size = 4 + num_led * sizeof(led_data_t) + 4; // start frame + data led + end frame
    frame_buffer = (uint8_t *) calloc(frame_buffer_size, sizeof(uint8_t));
    if(frame_buffer == NULL){
        ESP_LOGE(TAG, "frame buffer calloc fail");
        frame_buffer_size = 0;
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t lc8823_send_frame_buffer(void){
    if(frame_buffer == NULL || frame_buffer_size == 0){
        ESP_LOGE(TAG, "frame buffer not init");
        return ESP_FAIL;
    }

    spi_send_data(frame_buffer, frame_buffer_size);
    return ESP_OK;
}

esp_err_t lc8823_update_frame_buffer(uint8_t idx, led_data_t *led_data){
    if(frame_buffer == NULL || frame_buffer_size == 0){
        ESP_LOGE(TAG, "frame buffer not init");
        return ESP_FAIL;
    }
    uint8_t *p = frame_buffer + 4 + idx * sizeof(led_data_t);
    p[0] = led_data->lum;
    p[1] = led_data->blue;
    p[2] = led_data->green;
    p[3] = led_data->red;
    return ESP_OK; // return lc8823_send_frame_buffer();
}


esp_err_t lc8823_set_state(led_t *led, uint8_t state){
    if(led == NULL) return ESP_FAIL;
    //printf("set state led_index %d to %d\n", led->index, state);
    led->state = state;
    return lc8823_update_frame_buffer(led->index, (state) ? &led->value.ON : &led->value.OFF);
}

esp_err_t lc8823_set_color(led_t *led, uint8_t onoff, uint8_t red, uint8_t green, uint8_t blue){
    if(led == NULL) return ESP_FAIL;
    if (onoff)
    {
        led->value.ON.blue = blue;
        led->value.ON.green = green;
        led->value.ON.red = red;
    }
    else
    {
        led->value.OFF.blue = blue;
        led->value.OFF.green = green;
        led->value.OFF.red = red;
    }
    uint8_t stt_onoff = led->state;
    return lc8823_update_frame_buffer(led->index, (stt_onoff) ? &led->value.ON : &led->value.OFF);
}

esp_err_t lc8823_set_dim(led_t *led, uint8_t onoff, uint8_t dim_value){
    if(led == NULL) return ESP_FAIL;
    lc8823_calib_dim(&dim_value);
    ESP_LOGW("LC8823", "set dim: %02X", dim_value);
    if (onoff)
    {
        led->value.ON.lum = LIGHT_SET(dim_value);
    }
    else
    {
        led->value.OFF.lum = LIGHT_SET(dim_value);
    }
    uint8_t stt_onoff = led->state;
    return lc8823_update_frame_buffer(led->index, (stt_onoff) ? &led->value.ON : &led->value.OFF);
}

// load data to led object
void lc8823_load_value(led_t *led, uint8_t onoff, uint8_t dim_value, uint8_t red, uint8_t green, uint8_t blue){
    if(led == NULL) return;
    if (onoff)
    {
        led->value.ON.lum = dim_value;
        led->value.ON.blue = blue;
        led->value.ON.green = green;
        led->value.ON.red = red;
    }
    else
    {
        led->value.OFF.lum = dim_value;
        led->value.OFF.blue = blue;
        led->value.OFF.green = green;
        led->value.OFF.red = red;
    }
}

uint8_t get_dim_onoff(led_t *led, uint8_t onoff){
    if(led == NULL) return 0;
    return onoff ? led->value.ON.lum : led->value.OFF.lum;
}

uint8_t get_red_color(led_t *led, uint8_t onoff){
    if(led == NULL) return 0;
    return onoff ? led->value.ON.red : led->value.OFF.red;
}

uint8_t get_green_color(led_t *led, uint8_t onoff){
    if(led == NULL) return 0;
    return onoff ? led->value.ON.green : led->value.OFF.green;
}

uint8_t get_blue_color(led_t *led, uint8_t onoff){
    if(led == NULL) return 0;
    return onoff ? led->value.ON.blue : led->value.OFF.blue;
}

void lc8823_init(lc8823_config_t *config){
    if (!config)
    {
        ESP_LOGE(TAG, "lc8823 config is NULL, use config default");
    }else{
        lc8823_config.freq = min2(config->freq, 27000);
        lc8823_config.clk_pin = config->clk_pin;
        lc8823_config.data_pin = config->data_pin;
    }
    lc8823_spi_init(lc8823_config.clk_pin, lc8823_config.data_pin, lc8823_config.freq);
}
