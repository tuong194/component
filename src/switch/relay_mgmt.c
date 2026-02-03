#include "rd_gpio.h"
#include "relay_mgmt.h"
#include "esp_log.h"
#include "sdk_def.h"
#include "util.h"
#include "esp_timer.h"


typedef struct {
    uint8_t element;
    output_t gpio;
    uint8_t state;
#if DETECT_ZERO
    bool is_control;
#endif
}relay_t;

void relay_set_state(uint8_t element_index, uint8_t state);

relay_t relay[RELAY_NUM] = {
#if RELAY_NUM >= 1
    {
        .element = ELE_1,
        .gpio = {
            RELAY1_PIN, ACTIVE_HIGH, OFF_STATE
        },
        .state = false,
        #if DETECT_ZERO
        .is_control = false
        #endif
    }
#endif
#if RELAY_NUM >= 2
    ,{
        .element = ELE_2,
        .gpio = {
            RELAY2_PIN, ACTIVE_HIGH, OFF_STATE
        },
        .state = false,
        #if DETECT_ZERO
        .is_control = false
        #endif
    }
#endif
#if RELAY_NUM >= 3
    ,{
        .element = ELE_3,
        .gpio = {
            RELAY3_PIN, ACTIVE_HIGH, OFF_STATE
        },
        .state = false,
        #if DETECT_ZERO
        .is_control = false
        #endif
    }
#endif
#if RELAY_NUM >= 4
    ,{
        .element = ELE_4,
        .gpio = {
            RELAY4_PIN, ACTIVE_HIGH, OFF_STATE
        },
        .state = false,
        #if DETECT_ZERO
        .is_control = false
        #endif
    }    
#endif
};

#if DETECT_ZERO
static inline uint8_t get_detect_zero_pin(void){
    uint8_t stt = gpio_get_level(DETECT_ZERO_PIN);
    return stt;
}
esp_err_t rd_wait_detect_zero(void){
    uint8_t over_time_detect = 0;
    uint8_t stt_detect_past1, stt_detect_past2, stt_detect_cur1, stt_detect_cur2;
    stt_detect_past1 = get_detect_zero_pin();
    stt_detect_past2 = get_detect_zero_pin();
    stt_detect_cur1 = get_detect_zero_pin();
    stt_detect_cur2 = get_detect_zero_pin();
    do
    {
        over_time_detect++;
        stt_detect_past1 = get_detect_zero_pin();
        SLEEP_US(50);
        stt_detect_past2 = get_detect_zero_pin();
        SLEEP_US(500);
        stt_detect_cur1 = get_detect_zero_pin();
        SLEEP_US(50);
        stt_detect_cur2 = get_detect_zero_pin();

        if(over_time_detect >= NUM_CHECK_DETECH_MAX){
            printf("detect zero break\n");
            return ESP_FAIL;
            // break;
        }
    } while (!(stt_detect_past1 != 0 && stt_detect_past2 != 0 && stt_detect_cur1 == 0 && stt_detect_cur2 == 0)); //falling
    return ESP_OK;
}
#endif

void relay_gpio_config(void){
    #if RELAY_NUM >= 1
        rd_init_output(&relay[0].gpio);
    #endif
    #if RELAY_NUM >= 2
        rd_init_output(&relay[1].gpio);
    #endif
    #if RELAY_NUM >= 3    
        rd_init_output(&relay[2].gpio);
    #endif
    #if RELAY_NUM >= 4
        rd_init_output(&relay[3].gpio);
    #endif
    #if DETECT_ZERO
        gpio_config_t gpio_config_pin = {
            .pin_bit_mask = 1ULL << DETECT_ZERO_PIN,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE};
        gpio_config(&gpio_config_pin);
    #endif
}

void relay_set_state(uint8_t element_index, uint8_t state){
    if(element_index >= RELAY_NUM) return;
    relay[element_index].state = state;
    #if DETECT_ZERO
    relay[element_index].is_control = true;
    #else
    rd_set_output_state(&(relay[element_index].gpio), relay[element_index].state);
    #endif
}


#if DETECT_ZERO
void relay_check_control_hw(relay_t *relay){

    if(relay->is_control){
        relay->is_control = false;
        esp_err_t err = rd_wait_detect_zero();
        if(err == ESP_FAIL){
            ESP_LOGE("relay", "relay %d detect zero fail\n", relay->element + 1);
            return;
        }
        if(relay->state == ON_STATE){
            SLEEP_US(TIME_DETECT_ON);
        }else if(relay->state == OFF_STATE){
            SLEEP_US(TIME_DETECT_OFF);
        }
        rd_set_output_state(&(relay->gpio), relay->state);
    }

}

void relay_control_scan_all(void){
    for (uint8_t i = 0; i < RELAY_NUM; i++){
        relay_check_control_hw(&relay[i]);
    }
}
#endif