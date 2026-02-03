#ifndef UTIL_H
#define UTIL_H

#include "esp_rom_sys.h"

// macro delay_us
#define SLEEP_US(us)     esp_rom_delay_us(us)

/**
 * @brief Check whether the elapsed time since a reference point has exceeded a specified span
 *
 * @param ref      Reference start time
 * @param span_us  Time span to check (in microseconds)
 * @return 1  The current time has exceeded the specified time span
 * @return 0  The current time has not yet exceeded the specified time span
 */
uint8_t rd_exceed_us(int64_t ref, int64_t span_us);

/**
 * @brief Get the current tick counter value
 *
 * @param ref  Pointer to the variable where the returned value will be stored
 */
void get_tick_time(int64_t *ref);

/**
 * @brief Get the MAC address
 *
 * @param mac  Pointer to a buffer to store the MAC address (6 bytes)
 */
void get_mac_uint(uint8_t mac[6]);

/**
 * @brief Get the MAC address and convert it to string format
 *
 * @param mac_str  Pointer to the buffer where the MAC string will be stored
 */
void get_mac_str(char *mac_str);

/**
 * @brief Initialize the DEVICE_NAME for the device
 *
 * @param prefix  Prefix for the DEVICE_NAME
 * @return        Pointer to the resulting DEVICE_NAME string
 */
char *get_device_name(const char *prefix);
char *ConvertToCharLower(char *str);

/**
 * @brief Convert hex->ascii
 * 
 * @param hex_string 
 * @return char* 
 */
char *hex_to_ascii(char *hex_string);

#endif
