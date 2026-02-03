#include "dev_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#if (CONFIG_SWITCH_DEVICE)
    #include "button_mgmt.h"
    #include "led_mgmt.h"
    #include "relay_mgmt.h"
    #include "util.h"
#endif

#if (CONFIG_GCOOL_DEVICE)
    #include "gcool.h"
#endif

struct on_off_t{
    uint8_t present;
    uint8_t target;
};
static struct on_off_t dev_onoff[MAX_NUM_ELEMENT];


void dev_control_onoff(uint8_t element_index, uint8_t on_off)
{
    dev_onoff[element_index].target = on_off;
}

static void dev_on_off_scan(void)
{
    for (uint8_t i = 0; i < MAX_NUM_ELEMENT; i++)
    {
        if (dev_onoff[i].present != dev_onoff[i].target)
        {
            dev_onoff[i].present = dev_onoff[i].target;
#if CONFIG_SWITCH_DEVICE
            led_mgmt_set_state(i, dev_onoff[i].present);
            relay_set_state(i, dev_onoff[i].present);
#elif CONFIG_GCOOL_DEVICE
            if(i == FAN_INDEX){
                gcool_func_fan_switch(dev_onoff[FAN_INDEX].present);
            }else{
                gcool_func_lamp(dev_onoff[LAMP_INDEX].present);
            }
#endif
        }
    }
}

uint8_t dev_get_state_present(uint8_t ele)
{
    return dev_onoff[ele].present;
}

#if CONFIG_SWITCH_DEVICE

/*example*/
static void btn_event_handle(void *arg, esp_event_base_t event_base, int32_t event_id, void *data)
{
    if (event_base == BUTTON_EVENT_BASE)
    {
        switch (event_id)
        {
        case EVENT_BUTTON_PRESS:
        {
            ESP_LOGI("COMMON", "button press");
            uint8_t element = *((uint8_t *)data);
            uint8_t state = dev_get_state_present(element);
            dev_control_onoff(element, !state);
            break;
        }
        case EVENT_BUTTON_PAIR_K9B:
        {
            ESP_LOGI("COMMON", "event pair k9b");
            led_mgmt_set_blink(*(uint8_t *)data, 7, 200);
            break;
        }
        case EVENT_BUTTON_DELETE_ALL_K9B:
        {

            break;
        }
        case EVENT_BUTTON_CONFIG_WIFI:
        {
            ESP_LOGI("COMMON", "event config wwifi");


            break;
        }
        case EVENT_BUTTON_KICK_OUT:
        {
            ESP_LOGI("BTN_MANAGER", "KICK OUT !!!");



            break;
        }
        case EVENT_BUTTON_INC_COUNT_KICK_OUT:
            ESP_LOGI("COMMON", "event increase kick out");
 
            break;
        default:
            ESP_LOGE("COMMON", "UNKNOW EVENT_ID BUTTON_EVENT_BASE");
            break;
        }
    }
}

static void control_task(void *arg)
{
    led_mgmt_init();
    led_mgmt_register_get_state_cb(dev_get_state_present);
    button_gpio_config();
    relay_gpio_config();
    
    while (1)
    {
        led_mgmt_scan_blink();
        dev_on_off_scan();
        #if DETECT_ZERO
        relay_control_scan_all();
        #endif
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void init_control_task(void)
{
    xTaskCreate(control_task, "control_task", 2048, NULL, configMAX_PRIORITIES - 3, NULL);
}
#endif

void dev_common_init(void)
{
#if CONFIG_SWITCH_DEVICE
    btn_event_init(btn_event_handle);
    init_control_task();
#endif
}

