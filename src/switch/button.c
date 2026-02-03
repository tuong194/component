#include "button.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "esp_timer.h"
#include "sdk_def.h"

#define TAG   "button"

#define TICK_INTERVAL    10 //ms

#define TICKS_TIME_PRESS_DEFAULT      (100/TICK_INTERVAL)
#define TICKS_TIME_KEEP_DEFAULT       (1000/TICK_INTERVAL)
#define TICKS_TIME_LONG_KEEP_DEFAULT  (5000/TICK_INTERVAL)


#define TIME_TO_TICKS(time, tick_default) ((0==(time))?tick_default:(((time)/TICK_INTERVAL)<tick_default)?tick_default:((time)/TICK_INTERVAL))

#define CALL_EVENT_CB(ev)                                                   \
    if (btn->cb_info[ev]) {                                                 \
        for (int i = 0; i < btn->size[ev]; i++) {                           \
            btn->cb_info[ev][i].cb(btn, btn->cb_info[ev][i].usr_data);      \
        }                                                                   \
    }                                                                       \

typedef struct {
    button_cb_t cb;
    void *usr_data;
    // button_event_data_t event_data;
} button_cb_info_t;

typedef struct Button{
    uint32_t ticks;
    uint16_t press_ticks;
    uint16_t keep_ticks;
    uint16_t long_keep_ticks;
    uint8_t active_level;
    uint8_t (*hal_button_get_Level)(void *hardware_data);
    void     *hardware_data;
    button_event_t event;
    button_cb_info_t *cb_info[BUTTON_EVENT_MAX];
    size_t           size[BUTTON_EVENT_MAX];
    struct Button *next;
}button_dev_t;

//button handle list head.
static button_dev_t *g_head_handle = NULL;
static esp_timer_handle_t g_button_timer_handle = NULL;
static bool g_is_timer_running = false;

static void button_cb(void *args);
static void button_handler(button_dev_t *btn);

void button_reset_touch_pin_config(gpio_num_t RST_TOUCH_PIN, uint8_t active_level){
    esp_err_t ret = ESP_OK;
    gpio_config_t gpio_config_pin = {
        .pin_bit_mask = 1ULL << RST_TOUCH_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&gpio_config_pin);    
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "config RST_TOUCH_PIN FAIL");
    }   
    gpio_set_level(RST_TOUCH_PIN, active_level); 
}

esp_err_t button_gpio_init(const button_gpio_config_t *config){
    if(config == NULL){
        return ESP_FAIL;
    }
    gpio_config_t gpio_conf;
    gpio_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_conf.mode = GPIO_MODE_INPUT;
    gpio_conf.pin_bit_mask = (1ULL << config->gpio_num);
    if (config->disable_pull) {
        gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    } else {
        if (config->active_level) {
            gpio_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
            gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        } else {
            gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        }
    }
    gpio_config(&gpio_conf);
    return ESP_OK;
}

esp_err_t button_gpio_deinit(int gpio_num)
{
    return gpio_reset_pin(gpio_num);;
}

static inline uint8_t button_gpio_get_key_level(void *gpio_num)
{
    return (uint8_t)gpio_get_level((uint32_t)gpio_num);
}

static button_dev_t *button_create_com(uint8_t active_level, uint8_t (*hal_button_get_Level)(void *hardware_data), void *hardware, uint16_t press_ticks, uint16_t keep_ticks, uint16_t long_keep_ticks){
    button_dev_t *btn = (button_dev_t *) calloc(1, sizeof(button_dev_t));
    if(btn == NULL){
        ESP_LOGE(TAG, "button device calloc FAIL");
    }
    btn->event = BUTTON_EVENT_NONE_PRESS;
    btn->active_level = active_level;
    btn->hardware_data = hardware;
    btn->hal_button_get_Level = hal_button_get_Level;
    btn->press_ticks = press_ticks;
    btn->keep_ticks = keep_ticks;
    btn->long_keep_ticks = long_keep_ticks;
    // Add to LIST
    btn->next = g_head_handle;
    g_head_handle = btn;

    if(!g_button_timer_handle){
        esp_timer_create_args_t button_timer = {0};
        button_timer.arg = NULL;
        button_timer.callback = button_cb;
        button_timer.dispatch_method = ESP_TIMER_TASK;
        button_timer.name = "button_timer";
        esp_timer_create(&button_timer, &g_button_timer_handle);
        // esp_timer_start_periodic(g_button_timer_handle, TICK_INTERVAL * 1000U);        
    }
    return btn;
}

static esp_err_t button_delete_com(button_dev_t *btn){
    if(!btn){
        ESP_LOGE(TAG, "Pointer of handle is invalid");
        return ESP_ERR_INVALID_ARG;
    }
    button_dev_t **curr;
    for (curr = &g_head_handle; *curr;) {
        button_dev_t *entry = *curr;
        if (entry == btn) {
            *curr = entry->next;
            free(entry);
        } else {
            curr = &entry->next;
        }
    }   
    
    uint8_t num_handle = 0;
    button_dev_t *head = g_head_handle;
    while(head){
        head = head->next;
        num_handle ++;
    }

    ESP_LOGW(TAG, "remain btn handle : %d", num_handle);

    if(num_handle == 0 && g_is_timer_running == true){
        esp_timer_stop(g_button_timer_handle);
        esp_timer_delete(g_button_timer_handle);
        g_is_timer_running = false;
        g_button_timer_handle = NULL;
    }
    return ESP_OK;
}

esp_err_t button_delete_handle(button_handle_t btn_handle){
    button_dev_t *btn = (button_dev_t *)btn_handle;
    if(!btn){
        ESP_LOGE(TAG, "Pointer of handle is invalid");
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = button_gpio_deinit((int)(btn->hardware_data));
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "button deinit fail");
        return ESP_FAIL;
    }
    for (size_t i = 0; i < BUTTON_EVENT_MAX; i++)
    {
        if(btn->cb_info[i]) free(btn->cb_info[i]);
    }
    
    return button_delete_com(btn);
}

button_handle_t button_create_button_gpio(const button_config_t *config){
    esp_err_t ret = ESP_OK;
    
    button_dev_t *btn = NULL;
    uint16_t press_ticks = TIME_TO_TICKS(config->press_time, TICKS_TIME_PRESS_DEFAULT);
    uint16_t keep_ticks = TIME_TO_TICKS(config->keep_time, TICKS_TIME_KEEP_DEFAULT);
    uint16_t long_keep_ticks = TIME_TO_TICKS(config->long_keep_time, TICKS_TIME_LONG_KEEP_DEFAULT);
    const button_gpio_config_t *cfg = &(config->button_gpio_config); 
    ret = button_gpio_init(cfg);
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "button gpio init FAIL");
        return NULL;
    }
    ESP_LOGI(TAG, "press %d, keeping: %d, long keeping: %d", press_ticks, keep_ticks, long_keep_ticks);
    btn = button_create_com(cfg->active_level,button_gpio_get_key_level, (void *)cfg->gpio_num, press_ticks, keep_ticks, long_keep_ticks);
    if(!g_is_timer_running){
        g_is_timer_running = true;
        esp_timer_start_periodic(g_button_timer_handle, TICK_INTERVAL * 1000U);
    }
    return btn;
}

esp_err_t button_register_callback(button_handle_t btn_handle, button_event_t event, button_cb_t cb, void *usr_data){
    if(btn_handle == NULL) return ESP_FAIL;
    button_dev_t *btn = (button_dev_t *) btn_handle;
    if(event == BUTTON_EVENT_KEEPING || event == BUTTON_EVENT_RELEASE_KEEPING){
        if(btn->keep_ticks <= btn->press_ticks){
            ESP_LOGE(TAG, "keep time is less than press time");
            return ESP_FAIL;
        }
    }
    if(event == BUTTON_EVENT_LONG_KEEPING || event == BUTTON_EVENT_RELEASE_LONG_KEEPING){
        if(btn->long_keep_ticks <= btn->keep_ticks){
            ESP_LOGE(TAG, "long keep time is less than keep time");
            return ESP_FAIL;
        }
    }
    if(!btn->cb_info[event]){
        btn->cb_info[event] = calloc(1, sizeof(button_cb_info_t));
        if(btn->cb_info[event] == NULL){
            ESP_LOGE(TAG, "calloc call back fail");
            return ESP_FAIL;
        }
    }else{
        button_cb_info_t *p = realloc(btn->cb_info[event], sizeof(button_cb_info_t) * (btn->size[event] +1));
        if(p == NULL){
            ESP_LOGE(TAG, "p calloc call back fail");
            return ESP_FAIL;
        }
        btn->cb_info[event] = p;
    }

    btn->cb_info[event][btn->size[event]].cb = cb;
    btn->cb_info[event][btn->size[event]].usr_data = usr_data;
    btn->size[event]++;

    return ESP_OK;
}

button_event_t button_get_event(button_handle_t btn_handle)
{
    button_dev_t *btn = (button_dev_t *) btn_handle;
    return btn->event;
}

static void button_cb(void *args)
{
    button_dev_t *node;
    for(node = g_head_handle;node; node = node->next){
        button_handler(node);
    }
}

static void button_handler(button_dev_t *btn){
    uint8_t gpio_level = btn->hal_button_get_Level(btn->hardware_data);
    if(gpio_level == btn->active_level){
        btn->ticks++;
    }else{
        btn->ticks = 0;
    }

    if(btn->event == BUTTON_EVENT_NONE_PRESS && btn->ticks >= btn->press_ticks){
        btn->event = BUTTON_EVENT_PRESS;
    }else if(btn->event == BUTTON_EVENT_PRESS &&  btn->ticks >= btn->keep_ticks){
        btn->event = BUTTON_EVENT_KEEPING;
        //cb keeping
        CALL_EVENT_CB(BUTTON_EVENT_KEEPING);
    }else if(btn->event == BUTTON_EVENT_KEEPING &&  btn->ticks >= btn->long_keep_ticks){
        btn->event = BUTTON_EVENT_LONG_KEEPING;
        //cb long keeping
        CALL_EVENT_CB(BUTTON_EVENT_LONG_KEEPING);
    }else if(btn->ticks == 0){
        if(btn->event == BUTTON_EVENT_LONG_KEEPING){
            btn->event = BUTTON_EVENT_RELEASE_LONG_KEEPING;
            //cb release long keeping
            CALL_EVENT_CB(BUTTON_EVENT_RELEASE_LONG_KEEPING);
        }else if(btn->event == BUTTON_EVENT_KEEPING){
            btn->event = BUTTON_EVENT_RELEASE_KEEPING;
            //cb release keeping
            CALL_EVENT_CB(BUTTON_EVENT_RELEASE_KEEPING);
        }else if(btn->event == BUTTON_EVENT_PRESS){
            //cb press
            CALL_EVENT_CB(BUTTON_EVENT_PRESS);
        }
        btn->event = BUTTON_EVENT_NONE_PRESS;
    }
}
