#include "button.h"
#include "button_mgmt.h"
#include "sdk_def.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "util.h"

#define TAG "BUTTON"

typedef struct{
    uint8_t element;
    uint8_t is_keeping;
    uint8_t is_long_keeping;
    char name[16];
}button_info_t;

ESP_EVENT_DEFINE_BASE(BUTTON_EVENT_BASE);

static esp_event_loop_handle_t btn_event_loop;
static int64_t time_out_get_tick = 0; // time out kick out
static int64_t time_out_pair_k9b = 0; // time out pair k9b
static uint8_t btn_index_pair = 0xff;

static button_handle_t btn_handler[BTN_NUM];

static button_info_t btn_info[BTN_NUM] = {
    #ifdef BTN_1
        {BTN_1,0,0,"button_1"}
    #endif

    #ifdef BTN_2
        ,{BTN_2,0,0,"button_2"}
    #endif

    #ifdef BTN_3
        ,{BTN_3,0,0,"button_3"}
    #endif

    #ifdef BTN_4
        ,{BTN_4,0,0,"button_4"}
    #endif
};
static void board_button_event_cb(void *arg, void *data);

static inline uint8_t get_num_btn_keeping(){
    uint8_t number = 0;
    #ifdef BTN_1
        number += btn_info[BTN_1].is_keeping;
    #endif
    #ifdef BTN_2
        number += btn_info[BTN_2].is_keeping;
    #endif
    #ifdef BTN_3
        number += btn_info[BTN_3].is_keeping;
    #endif
    #ifdef BTN_4
        number += btn_info[BTN_4].is_keeping;
    #endif
    return number;
}

static inline uint8_t btn_is_keeping(uint8_t index)
{
    return btn_info[index].is_keeping;
}

static inline void clear_check_keep(uint8_t index)
{
    btn_info[index].is_keeping = 0;
}

esp_err_t button_gpio_config(void){
    esp_err_t ret = ESP_OK;
    const button_gpio_config_t button_gpio[BTN_NUM] = {
    #ifdef BUTTON_PIN1
        {
            .gpio_num = BUTTON_PIN1,
            .active_level = BUTTON_ACTIVE_LEVEL,
            .disable_pull = false,
        }
    #endif
    #ifdef BUTTON_PIN2
        ,{
            .gpio_num = BUTTON_PIN2,
            .active_level = BUTTON_ACTIVE_LEVEL,
            .disable_pull = false,
        }
    #endif
    #ifdef BUTTON_PIN3
        ,{
            .gpio_num = BUTTON_PIN3,
            .active_level = BUTTON_ACTIVE_LEVEL,
            .disable_pull = false,
        }
    #endif
    #ifdef BUTTON_PIN4
        ,{
            .gpio_num = BUTTON_PIN4,
            .active_level = BUTTON_ACTIVE_LEVEL,
            .disable_pull = false,
        }
    #endif
    };

    button_config_t btn_config[BTN_NUM] = {
        #ifdef BTN_1
        {
            .press_time = CONFIG_PRESS_TIME_MS,
            .keep_time = CONFIG_KEEP_TIME_MS,
            .long_keep_time = CONFIG_LONG_KEEP_TIME_MS,
            .button_gpio_config = button_gpio[BTN_1]
        }
        #endif
        #ifdef BTN_2
        ,{
            .press_time = CONFIG_PRESS_TIME_MS,
            .keep_time = CONFIG_KEEP_TIME_MS,
            .long_keep_time = CONFIG_LONG_KEEP_TIME_MS,
            .button_gpio_config = button_gpio[BTN_2]
        }
        #endif
        #ifdef BTN_3
        ,{
            .press_time = CONFIG_PRESS_TIME_MS,
            .keep_time = CONFIG_KEEP_TIME_MS,
            .long_keep_time = CONFIG_LONG_KEEP_TIME_MS,
            .button_gpio_config = button_gpio[BTN_3]
        }
        #endif
        #ifdef BTN_4
        ,{
            .press_time = CONFIG_PRESS_TIME_MS,
            .keep_time = CONFIG_KEEP_TIME_MS,
            .long_keep_time = CONFIG_LONG_KEEP_TIME_MS,
            .button_gpio_config = button_gpio[BTN_4]
        }
        #endif
    };
    button_reset_touch_pin_config(RESET_TOUCH_PIN, TOUCH_ACTIVE_POW);
    for (size_t i = 0; i < BTN_NUM; i++)
    {
        btn_handler[i] = button_create_button_gpio(&btn_config[i]);
        if(btn_handler[i] == NULL) ret = ESP_FAIL;
    }
    for (size_t i = 0; i < BTN_NUM; i++)
    {
        ret += button_register_callback(btn_handler[i], BUTTON_EVENT_PRESS, board_button_event_cb, &btn_info[i]);
        ret += button_register_callback(btn_handler[i], BUTTON_EVENT_KEEPING, board_button_event_cb, &btn_info[i]);
        ret += button_register_callback(btn_handler[i], BUTTON_EVENT_RELEASE_KEEPING, board_button_event_cb, &btn_info[i]);
        ret += button_register_callback(btn_handler[i], BUTTON_EVENT_LONG_KEEPING, board_button_event_cb, &btn_info[i]);
        ret += button_register_callback(btn_handler[i], BUTTON_EVENT_RELEASE_LONG_KEEPING, board_button_event_cb, &btn_info[i]);
    }
    return ret;
}

static void board_button_event_cb(void *arg, void *data){
    button_handle_t btn_handler = (button_handle_t) arg;
    button_info_t *btn = (button_info_t *)data;
    button_event_t event = button_get_event(btn_handler);
    switch (event)
    {
    case BUTTON_EVENT_PRESS:
        if(btn_index_pair == btn->element){
            esp_event_post_to(btn_event_loop, BUTTON_EVENT_BASE, EVENT_BUTTON_DELETE_ALL_K9B, &btn->element, 1, portMAX_DELAY);
            btn_index_pair = 0xff;
            break;
        }
        esp_event_post_to(btn_event_loop, BUTTON_EVENT_BASE, EVENT_BUTTON_PRESS, &btn->element, 1, portMAX_DELAY);
        break;
    case BUTTON_EVENT_KEEPING:{
        btn->is_keeping = 1;
        get_tick_time(&time_out_get_tick);
        get_tick_time(&time_out_pair_k9b);
        break;
    }
    case BUTTON_EVENT_RELEASE_KEEPING:{
        // printf("[BOARD] btn %d release keeping\n",btn->element + 1);
        btn->is_keeping = 0;
        get_tick_time(&time_out_get_tick);
        get_tick_time(&time_out_pair_k9b);
        break;
    }
    case BUTTON_EVENT_LONG_KEEPING:{
        btn->is_keeping = 0;
        btn->is_long_keeping = 1;
#if BTN_NUM == 1
        if(btn_info[BTN_1].is_long_keeping == 1){
            esp_event_post_to(btn_event_loop, BUTTON_EVENT_BASE, EVENT_BUTTON_CONFIG_WIFI, NULL, 0, portMAX_DELAY);
        }
#else
        if(btn_info[BTN_1].is_long_keeping == 1 && btn_info[BTN_2].is_long_keeping == 1){
            esp_event_post_to(btn_event_loop, BUTTON_EVENT_BASE, EVENT_BUTTON_CONFIG_WIFI, NULL, 0, portMAX_DELAY);
        }
#endif
        break;
    }
    case BUTTON_EVENT_RELEASE_LONG_KEEPING:
        btn->is_long_keeping = 0;
        // printf("[BOARD] btn %d release long keeping\n",btn->element + 1);
        break;
    
    default:
        break;
    }
}

extern void K9B_Pair_OnOff_ClearFlag(void);
static void button_manager_cb(void *args){
    static uint8_t count_kick_out = 0;
#if BTN_NUM == 1
    if(get_num_btn_keeping() == 1 && rd_exceed_us(time_out_get_tick, CLOCK_TIME_KICK_OUT)){
        for (size_t i = 0; i < BTN_NUM; i++)
        {
            if(btn_is_keeping(i)) clear_check_keep(1);          
        }
        count_kick_out++;
        if(count_kick_out >= 3){
            count_kick_out = 0;
            esp_event_post_to(btn_event_loop, BUTTON_EVENT_BASE, EVENT_BUTTON_KICK_OUT, NULL, 0, portMAX_DELAY);
        }else{
            esp_event_post_to(btn_event_loop, BUTTON_EVENT_BASE, EVENT_BUTTON_INC_COUNT_KICK_OUT, NULL, 0, portMAX_DELAY);
        }         
    }      
#endif
    if(get_num_btn_keeping() == 1 && rd_exceed_us(time_out_get_tick, CLOCK_TIME_SET_PAIR_K9B)){
        for (size_t i = 0; i < BTN_NUM; i++)
        {
            if(btn_is_keeping(i)){
                clear_check_keep(i);
                btn_index_pair = i;
                esp_event_post_to(btn_event_loop, BUTTON_EVENT_BASE, EVENT_BUTTON_PAIR_K9B, &btn_index_pair, 1, portMAX_DELAY);
            }
        }
    }    
#if BTN_NUM > 1
    else if(get_num_btn_keeping() == 2 && rd_exceed_us(time_out_get_tick, CLOCK_TIME_KICK_OUT)){
        for (size_t i = 0; i < BTN_NUM; i++)
        {
            if(btn_is_keeping(i)) clear_check_keep(i);          
        }
        count_kick_out++;
        if(count_kick_out >= 3){
            count_kick_out = 0;
            esp_event_post_to(btn_event_loop, BUTTON_EVENT_BASE, EVENT_BUTTON_KICK_OUT, NULL, 0, portMAX_DELAY);
        }else{
            esp_event_post_to(btn_event_loop, BUTTON_EVENT_BASE, EVENT_BUTTON_INC_COUNT_KICK_OUT, NULL, 0, portMAX_DELAY);
        }
    }
#endif

    if (count_kick_out > 0 && rd_exceed_us(time_out_get_tick, CLOCK_TIME_OUT_KICK_OUT))
    {
        count_kick_out = 0;
        ESP_LOGW("BTN_MANAGER", "time out kick out, reset count");
    }  
    
    if (btn_index_pair != 0xff && rd_exceed_us(time_out_pair_k9b, CLOCK_TIME_OUT_KICK_OUT))
    {
        btn_index_pair = 0xff;
        ESP_LOGW("BTN_MANAGER", "time out pair k9b, reset btn_index_pair");
        K9B_Pair_OnOff_ClearFlag();
    }  
}

void button_manager_init(void)
{
    static esp_timer_handle_t g_button_timer_handle = NULL;
    if(!g_button_timer_handle){
        esp_timer_create_args_t button_timer = {0};
        button_timer.arg = NULL;
        button_timer.callback = button_manager_cb;
        button_timer.dispatch_method = ESP_TIMER_TASK;
        button_timer.name = "button_timer";
        esp_timer_create(&button_timer, &g_button_timer_handle);
        esp_timer_start_periodic(g_button_timer_handle, 100 * 1000U); //100ms
    }
}

static void dummy_btn_handler(void *arg,
                             esp_event_base_t base,
                             int32_t id,
                             void *data)
{
    // do nothing
}

void btn_event_init(esp_event_handler_t event_handler){
    esp_event_loop_args_t loop_args = {
        .queue_size = 5,
        .task_name = "btn_event_loop",
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 2048*2,
        .task_core_id = tskNO_AFFINITY
    };
    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &btn_event_loop));
    if(!event_handler){
        event_handler = dummy_btn_handler;
    }
    esp_event_handler_register_with(btn_event_loop, BUTTON_EVENT_BASE, ESP_EVENT_ANY_ID, event_handler, NULL);
    
    // button_manager_init();
}


