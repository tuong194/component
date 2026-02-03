#ifndef _RELAY_MGMT_H__
#define _RELAY_MGMT_H__

#include "stdint.h"

void relay_gpio_config(void);
void relay_set_state(uint8_t element_index, uint8_t state);
#if DETECT_ZERO
void relay_control_scan_all(void);
#endif

#endif /*  */
