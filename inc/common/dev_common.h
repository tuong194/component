#ifndef _DEV_COMMON_H__
#define _DEV_COMMON_H__

#include "stdint.h"
#include "sdk_def.h"

#if (CONFIG_SWITCH_DEVICE)
    #define MAX_NUM_ELEMENT   NUM_ELEMENT
#endif

#if (CONFIG_GCOOL_DEVICE)
    #define MAX_NUM_ELEMENT   2
    #define FAN_INDEX       0
    #define LAMP_INDEX      1
#endif

void dev_common_init(void);
void dev_control_onoff(uint8_t element_index, uint8_t on_off);
uint8_t dev_get_state_present(uint8_t ele);

#if (CONFIG_GCOOL_DEVICE)
inline void dev_control_onoff_fan(uint8_t on_off)
{
    dev_control_onoff(FAN_INDEX, on_off);
}

inline void dev_control_onoff_lamp(uint8_t on_off)
{
    dev_control_onoff(LAMP_INDEX, on_off);
}
#endif

#endif /* _DEV_COMMON_H__ */
