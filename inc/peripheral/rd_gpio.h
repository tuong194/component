#ifndef _RD_GPIO_H__
#define _RD_GPIO_H__

#include "stdint.h"
#include "stdbool.h"
#include "driver/gpio.h"

// typedef enum {
//     ACTIVE_HIGH = 1,
//     ACTIVE_LOW  = 0
// } active_level_t;

typedef uint8_t active_level_t;

typedef struct {
    gpio_num_t gpio_pin;
    active_level_t active_level;
    uint8_t state;
}output_t;

typedef struct {
    gpio_num_t gpio_pin;
    active_level_t active_level;
    bool pull_enable;
}input_t;

void rd_set_output_state(output_t *output, uint8_t state);
void rd_init_output(output_t *output);
void rd_init_input(input_t *config);


#endif /*  */
