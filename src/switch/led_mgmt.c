#include "led_mgmt.h"
#include "lc8823.h"
#include "util.h"

#include "esp_timer.h"

#define min2(a,b)	 ((a) < (b) ? (a) : (b))

struct led_sigle_blink_t
{
	uint8_t num_of_cycle;
	uint32_t time_cycle_ms;
	int64_t last_clockTime_toggle_ms;
};

static struct led_sigle_blink_t led_blink_val[NUM_ELEMENT] = {0};
static led_t led_obj[MAX_NUM_LED_OBJECT];

static get_state_ele_cb_t get_state_ele_cb = NULL;

void led_mgmt_register_get_state_cb(get_state_ele_cb_t cb)
{
    if(cb) get_state_ele_cb = cb;
}

void led_mgmt_init(void)
{
    lc8823_config_t lc8823_config = {
        .clk_pin = LED_CLK,
        .data_pin = LED_DATA,
        .freq = FREQ_LED};
    for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
    {
        led_obj[i].index = i;
        led_obj[i].state = OFF_STATE;
    }
#if NUM_ELEMENT == 1
    led_obj[0].element = LED_WIFI;
    led_obj[1].element = LED_1; led_obj[2].element = LED_1;
    led_obj[3].element = LED_1; led_obj[4].element = LED_1;
    led_obj[MAX_NUM_LED_OBJECT - 1].element = LED_WIFI; // led wifi
#endif

#if NUM_ELEMENT == 2
    led_obj[0].element = LED_1; led_obj[1].element = LED_1;
    led_obj[2].element = LED_WIFI; 
    led_obj[3].element = LED_2; led_obj[4].element = LED_2;
    led_obj[MAX_NUM_LED_OBJECT - 1].element = LED_WIFI; // led wifi
#endif

#if NUM_ELEMENT == 3
    led_obj[0].element = LED_1; led_obj[1].element = LED_1;
    led_obj[2].element = LED_2; led_obj[3].element = LED_2;
    led_obj[4].element = LED_3; led_obj[5].element = LED_3;
    led_obj[MAX_NUM_LED_OBJECT - 1].element = LED_WIFI; // led wifi
#endif

#if NUM_ELEMENT == 4
    led_obj[0].element = LED_1; led_obj[1].element = LED_1;
    led_obj[2].element = LED_2; led_obj[3].element = LED_2;
    led_obj[4].element = LED_3; led_obj[5].element = LED_3;
    led_obj[6].element = LED_4; led_obj[7].element = LED_4;
    led_obj[MAX_NUM_LED_OBJECT - 1].element = LED_WIFI; // led wifi
#endif

    lc8823_init(&lc8823_config);
    lc8823_init_frame_buffer(MAX_NUM_LED_OBJECT);

    for (size_t i = 0; i < NUM_ELEMENT; i++)
    {
        for (size_t j = 0; j < MAX_NUM_LED_OBJECT; j++)
        {
            if (led_obj[j].element == i)
            {
                lc8823_load_value(&led_obj[j], ON_STATE, LIGHT_MAX, 0x00, 0xFF, 0xB2);
                lc8823_load_value(&led_obj[j], OFF_STATE, LIGHT_MIN, 0x00, 0xFF, 0xB2);
            }
            else if (led_obj[j].element == LED_WIFI)
            {
                lc8823_load_value(&led_obj[j], ON_STATE, LIGHT_MIN, 0x00, 0xFF, 0x00);  // green
                lc8823_load_value(&led_obj[j], OFF_STATE, LIGHT_MIN, 0xFF, 0x00, 0x00); // red
            }
        }
    }

    for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
    {
        lc8823_update_frame_buffer(i, (led_obj[i].state) ? &led_obj[i].value.ON : &led_obj[i].value.OFF);
    }
    lc8823_send_frame_buffer();
}

led_t *get_led_object(uint8_t element)
{
    for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
    {
        if (led_obj[i].element == element)
        {
            return &led_obj[i];
        }
    }
    return NULL;
}

/* Mỗi element gồm 2 led_t liền kề nhau*/
esp_err_t led_mgmt_set_state(uint8_t element, uint8_t state)
{
    if (element == ALL_ELE)
    {
        for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
        {
            if (led_obj[i].element != LED_WIFI)
                lc8823_set_state(&led_obj[i], state);
        }
    }
    else
    {
        // led_t *led = get_led_object(element);
        // if (led == NULL)
        //     return ESP_FAIL;

        for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
        {
            if (led_obj[i].element == element)
                lc8823_set_state(&led_obj[i], state);
        }
        // lc8823_set_state(led, state);
        // lc8823_set_state(led + 1, state);
    }

    return lc8823_send_frame_buffer();
}

esp_err_t led_mgmt_set_dim(uint8_t element, uint8_t onoff, uint8_t dim_value)
{
    if (element == ALL_ELE)
    {
        for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
        {
            if (led_obj[i].element != LED_WIFI)
                lc8823_set_dim(&led_obj[i], onoff, dim_value);
        }
    }
    else
    {
        // led_t *led = get_led_object(element);
        // if (led == NULL)
        //     return ESP_FAIL;
        for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
        {
            if (led_obj[i].element == element)
                lc8823_set_dim(&led_obj[i], onoff, dim_value);
        }
        // lc8823_set_dim(led, onoff, dim_value);
        // lc8823_set_dim(led + 1, onoff, dim_value);
    }

    return lc8823_send_frame_buffer();
}

esp_err_t led_mgmt_set_color(uint8_t element, uint8_t onoff, uint8_t red, uint8_t green, uint8_t blue)
{
    if (element == ALL_ELE)
    {
        for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
        {
            if (led_obj[i].element != LED_WIFI)
                lc8823_set_color(&led_obj[i], onoff, red, green, blue);
        }
    }
    else
    {
        // led_t *led = get_led_object(element);
        // if (led == NULL)
        //     return ESP_FAIL;
        for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
        {
            if (led_obj[i].element == element)
                lc8823_set_color(&led_obj[i], onoff, red, green, blue);
        }
        // lc8823_set_color(led, onoff, red, green, blue);
        // lc8823_set_color(led + 1, onoff, red, green, blue);
    }

    return lc8823_send_frame_buffer();
}

uint8_t led_mgmt_get_dim_onoff(uint8_t element, uint8_t onoff)
{
    led_t *led = get_led_object(element);
    if (led == NULL)
        return 0;

    return get_dim_onoff(led, onoff);
}

uint8_t led_mgmt_get_red_color(uint8_t element, uint8_t onoff)
{
    led_t *led = get_led_object(element);
    if (led == NULL)
        return 0;

    return get_red_color(led, onoff);
}

uint8_t led_mgmt_get_green_color(uint8_t element, uint8_t onoff)
{
    led_t *led = get_led_object(element);
    if (led == NULL)
        return 0;

    return get_green_color(led, onoff);
}

uint8_t led_mgmt_get_blue_color(uint8_t element, uint8_t onoff)
{
    led_t *led = get_led_object(element);
    if (led == NULL)
        return 0;

    return get_blue_color(led, onoff);
}

esp_err_t led_mgmt_set_wifi_color(uint8_t stt)
{
    uint8_t state = OFF_STATE; //(stt) ? ON_STATE : OFF_STATE;
    uint8_t red = (stt) ? 0x00 : 0xFF;
    uint8_t green = (stt) ? 0xFF : 0x00;
    uint8_t blue = 0x00;
    led_t *led = NULL;
    for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
    {
        if (led_obj[i].element == LED_WIFI)
        {
            led = &led_obj[i];
            lc8823_set_color(led, state, red, green, blue);
        }
    }
    if (led == NULL)
        return ESP_FAIL;
    return lc8823_send_frame_buffer();
}

esp_err_t led_mgmt_reload_data(uint8_t element)
{
    if (element == ALL_ELE)
    {
        for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
        {
            uint8_t state = OFF_STATE;
            if(led_obj[i].element != LED_WIFI){
                if(get_state_ele_cb) state = get_state_ele_cb(led_obj[i].element);
                lc8823_set_state(&led_obj[i], state);
            }
        }
    }
    else
    {
        led_t *led = get_led_object(element);
        if (led == NULL)
            return ESP_FAIL;

        uint8_t state = OFF_STATE;
        if(get_state_ele_cb) state = get_state_ele_cb(element);

        for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
        {
            if (led_obj[i].element == element)
                lc8823_set_state(&led_obj[i], state);
        }
        // lc8823_set_state(led, state);
        // lc8823_set_state(led + 1, state);
    }
    return lc8823_send_frame_buffer();
}

esp_err_t led_mgmt_load_value(uint8_t ele, uint8_t onoff, uint8_t dim_value, uint8_t red, uint8_t green, uint8_t blue){
    for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
    {
        if (led_obj[i].element == ele)
            lc8823_load_value(&led_obj[i], onoff, dim_value, red, green, blue);
    }
    return ESP_OK;    
}

#if 1
void led_mgmt_blink_delay(uint8_t ele, uint8_t num_cycle, uint16_t time_delay_ms)
{

    while (num_cycle > 0)
    {

        if (num_cycle == 1)
        {
            led_mgmt_reload_data(ele);
            return;
        }
        
        if (num_cycle > 1)
        {
            if (num_cycle % 2 == 0)
            {
                led_mgmt_set_state(ele, ON_STATE);
            }
            else
            {
                led_mgmt_set_state(ele, OFF_STATE);
            }
        }

        num_cycle--;
        SLEEP_US(time_delay_ms * 1000);
    }
}
#endif

void led_mgmt_set_blink(uint8_t ele, uint8_t num_cycle, uint32_t time_tongle_ms)
{
    if (ele == ALL_ELE)
    {
        for (uint8_t i = 0; i < NUM_ELEMENT; i++)
        {
            led_blink_val[i].num_of_cycle = num_cycle;
            led_blink_val[i].time_cycle_ms = time_tongle_ms * 1000;
        }
    }
    else
    {
        ele = min2(ele, NUM_ELEMENT - 1);
        led_blink_val[ele].num_of_cycle = num_cycle;
        led_blink_val[ele].time_cycle_ms = time_tongle_ms * 1000;
    }
}

void led_mgmt_scan_blink(void)
{
    for (uint8_t i = 0; i < NUM_ELEMENT; i++)
    {
        if (led_blink_val[i].num_of_cycle > 0)
        {
            if (rd_exceed_us(led_blink_val[i].last_clockTime_toggle_ms, led_blink_val[i].time_cycle_ms))
            {
                if (led_blink_val[i].num_of_cycle == 1)
                {
                    led_mgmt_reload_data(i);
                    led_blink_val[i].num_of_cycle--;
                    return;
                }
                if (led_blink_val[i].num_of_cycle % 2 == 0)
                {
                    led_mgmt_set_state(i, ON_STATE); // dim high
                }
                else
                {
                    led_mgmt_set_state(i, OFF_STATE); // dim low
                }
                led_blink_val[i].num_of_cycle--;
                led_blink_val[i].last_clockTime_toggle_ms = esp_timer_get_time();
            }
        }
    }
}

#include "math.h"
int led_mgmt_convert_hsv_to_rgb(uint8_t ele, double H, double S, double V){
    double r, g, b;
    uint8_t Red,Green,Blue;

    H = H/10;
    S = S/10;
    V = V/10;
    // Clamp inputs
    if (S < 0) S = 0; 
    if (S > 100) S = 100;
    if (V < 0) V = 0; 
    if (V > 100) V = 100;

    // Normalize H to [0, 360)
    H = fmod(H, 360.0);
    if (H < 0.0) H += 360.0;

    S /= 100.0;
    V /= 100.0;

    int i = (int)(H / 60.0);
    double f = (H / 60.0) - i;
    double p = V * (1 - S);
    double q = V * (1 - S * f);
    double t = V * (1 - S * (1 - f));

    switch (i) {
        case 0: r = V; g = t; b = p; break;
        case 1: r = q; g = V; b = p; break;
        case 2: r = p; g = V; b = t; break;
        case 3: r = p; g = q; b = V; break;
        case 4: r = t; g = p; b = V; break;
        case 5: r = V; g = p; b = q; break;
        default: r = g = b = 0; break;
    }

    Red = (uint8_t)(r * 255.0 + 0.5);
    Green = (uint8_t)(g * 255.0 + 0.5);
    Blue = (uint8_t)(b * 255.0 + 0.5);

    printf("[LC8823] index: #%d R: %d G: %d B: %d\n",ele, Red, Green, Blue);
    // led_mgmt_set_color(ele, ON_STATE, Red, Green, Blue); //ON
    // led_mgmt_set_color(ele, OFF_STATE, Red, Green, Blue); //OFF
    if (ele == ALL_ELE)
    {
        for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
        {
            if (led_obj[i].element != LED_WIFI)
            {
                lc8823_load_value(&led_obj[i], ON_STATE, LIGHT_MAX, Red, Green, Blue);
                lc8823_load_value(&led_obj[i], OFF_STATE, LIGHT_MIN, Red, Green, Blue);
                // lc8823_set_color(&led_obj[i], ON_STATE, Red, Green, Blue);
                // lc8823_set_color(&led_obj[i], OFF_STATE, Red, Green, Blue);
            }
        }
    }
    else
    {
        for (size_t i = 0; i < MAX_NUM_LED_OBJECT; i++)
        {
            if (led_obj[i].element == ele){
                lc8823_load_value(&led_obj[i], ON_STATE, LIGHT_MAX, Red, Green, Blue);
                lc8823_load_value(&led_obj[i], OFF_STATE, LIGHT_MIN, Red, Green, Blue);
                // lc8823_set_color(&led_obj[i], ON_STATE, Red, Green, Blue);
                // lc8823_set_color(&led_obj[i], OFF_STATE, Red, Green, Blue);
            }
        }
    }

    led_mgmt_reload_data(ele);
    return 0;
}