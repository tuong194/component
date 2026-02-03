#ifndef GCOOL_DEF_H__
#define GCOOL_DEF_H__


#define VERSION_MODULE      0x00 //module esp
#define VERSION_MCU         0x03 //mcu gcool

/*======================= CMD BASIC PROTOCOL ========================*/
#define CMD_HEARTBEAT           0x00
#define CMD_PRODUCT_INFO        0x01
#define CMD_WORKING_MODE        0x02  //set by the MCU
#define CMD_WIFI_STT            0x03
#define CMD_RESET_WIFI          0x04
#define CMD_NETWORK_CONFIG_MODE 0x05

#define CMD_PRODUCT_FUNC        0x06
#define CMD_WORKING_STT_MCU     0x07
#define CMD_WORKING_STT_MODULE  0x08

#define CMD_UPGRADE_STARTUP      0x0A
#define CMD_UPGRADE_PACKET_TRANS 0x0B
#define CMD_LOCAL_TIME           0x1C
#define CMD_TEST_WIFI            0x0E
#define CMD_GET_STT_WIFI         0x2B

#define UNKNOWN_CMD 0xff


/*======================= FUNCTION CODE ========================*/
#define NO_FUNC             0x00
#define LAMP                0x14
#define COUNTDOWN           0x1a
#define FAN_SWITCH          0x3c
#define FAN_MODE            0x3d
#define WIND_SPEED          0x3e
#define DIRECTION_FAN       0x3f
#define FAN_COUNTDOWN_LEFT  0x40
#define WORK_MODE           0x65
#define COLOUR              0x66
#define DREAMLIGHT_SCENE    0x67

/*======================= NETWORK STATUS ========================*/
#define SMART_NETWORK_CONF        0x00
#define AP_MODE                   0x01
#define CONFIGURED_CONNECT_FAIL   0x02
#define CONFIGURED_CONNECT_SUCESS 0x03
#define CONNECTED_TO_CLOUD        0x04
#define LOW_POWER_MODE            0x05
#define SMART_AND_AP_STATE        0x06


/*======================= FAN MODE ========================*/
#define NORMAL 0x00
#define SLEEP  0x01
#define TURBO  0x02
#define NATURE 0x03


/*======================= FAN DIRECTION ========================*/
#define CW   0x00  // xoay cùng chiều kim đồng hồ
#define CCW  0x01  // xoay ngược chiều kim đồng hồ

#endif
