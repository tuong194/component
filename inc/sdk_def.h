#ifndef _SDK_DEFINE_H__
#define _SDK_DEFINE_H__

#include "sdkconfig.h"

#define DETECT_ZERO 0

#define OFF_STATE    0
#define ON_STATE     1


#if DETECT_ZERO
#define DETECT_ZERO_PIN  6 
#define TIME_DETECT_ON 6000  // us
#define TIME_DETECT_OFF 4800 // us
#define NUM_CHECK_DETECH_MAX 150
#endif

#if CONFIG_SWITCH_DEVICE

#define NUM_ELEMENT   3
#define BTN_NUM       NUM_ELEMENT
#define RELAY_NUM     NUM_ELEMENT

#define ACTIVE_HIGH 1
#define ACTIVE_LOW  0

#define TOUCH_ACTIVE_POW			ACTIVE_LOW
#define TOUCH_NON_ACTIVE_POW		!TOUCH_ACTIVE_POW

#define BUTTON_ACTIVE_LEVEL ACTIVE_HIGH

#define RESET_TOUCH_PIN  1   

#define LED_DATA         21   
#define LED_CLK          20   

#define ALL_LED       0xff

#if NUM_ELEMENT >= 1
    #define ELE_1         0
    #define BTN_1         ELE_1
    #define BUTTON_PIN1   8 //GPIO_NUM_8
    #define	RELAY1_PIN	  10	
#endif

#if NUM_ELEMENT >= 2
    #define ELE_2         1
    #define BTN_2         ELE_2
    #define BUTTON_PIN2   3
    #define	RELAY2_PIN    4
#endif

#if NUM_ELEMENT >= 3
    #define ELE_3         2
    #define BTN_3         ELE_3
    #define BUTTON_PIN3   7
    #define	RELAY3_PIN	  5	
#endif

#if NUM_ELEMENT >= 4
    #define ELE_4         3
    #define BTN_4         ELE_4
    #define BUTTON_PIN4   0
    #define	RELAY4_PIN	  6
#endif

#elif CONFIG_GCOOL_DEVICE

#elif CONFIG_PLUG_DEVICE

#endif





#endif /* _SDK_DEFINE_H__ */
