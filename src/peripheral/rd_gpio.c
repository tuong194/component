#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"

#include "rd_gpio.h"
#include "sdk_def.h"

#define TAG "RD_GPIO"

static inline uint32_t real_level_set_output(output_t *output, uint8_t state)
{
    if (output->active_level == ACTIVE_HIGH)
        return state == ON_STATE ? 1 : 0;
    else
        return state == ON_STATE ? 0 : 1;
}

static inline void rd_set_hw(output_t *output)
{
    printf("OUTPUT: set %s\n", output->state ? "ON" : "OFF");
    gpio_set_level(output->gpio_pin, real_level_set_output(output, output->state));
}

void rd_set_output_state(output_t *output, uint8_t state)
{
    if (output->state == state)
    {
        return;
    }
    output->state = state;
    rd_set_hw(output);
}

void rd_init_output(output_t *output)
{
    esp_err_t ret = ESP_OK;
    gpio_config_t gpio_config_pin = {
        .pin_bit_mask = 1ULL << (output->gpio_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&gpio_config_pin);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "config gpio %d FAIL", output->gpio_pin);
    }
}

void rd_init_input(input_t *config)
{
    esp_err_t ret = ESP_OK;
    gpio_config_t gpio_config_pin;
    gpio_config_pin.pin_bit_mask = 1ULL << (config->gpio_pin);
    gpio_config_pin.mode = GPIO_MODE_INPUT;
    gpio_config_pin.intr_type = GPIO_INTR_DISABLE;

    if (config->pull_enable)
    {
        if (config->active_level)
        {
            gpio_config_pin.pull_down_en = GPIO_PULLDOWN_ENABLE;
            gpio_config_pin.pull_up_en = GPIO_PULLUP_DISABLE;
        }
        else
        {
            gpio_config_pin.pull_down_en = GPIO_PULLDOWN_DISABLE;
            gpio_config_pin.pull_up_en = GPIO_PULLUP_ENABLE;
        }
    }
    else
    {
        gpio_config_pin.pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_config_pin.pull_up_en = GPIO_PULLUP_DISABLE;
    }
    ret = gpio_config(&gpio_config_pin);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "config gpio input %d FAIL", config->gpio_pin);
    }
}

