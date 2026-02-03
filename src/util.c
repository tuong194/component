#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "esp_mac.h"
#include "esp_timer.h"

uint8_t rd_exceed_us(int64_t ref, int64_t span_us)
{
    return (esp_timer_get_time() - ref) >= span_us;
}

void get_tick_time(int64_t *ref){
	*ref = esp_timer_get_time();
}

void get_mac_uint(uint8_t mac[6]){
	esp_read_mac(mac, ESP_MAC_WIFI_STA);
}

void get_mac_str(char *mac_str)
{
	uint8_t mac[6] = {0};
	ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
	sprintf((char *)mac_str, (const char *)"%.2x%.2x%.2x%.2x%.2x%.2x",
					mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

char *get_device_name(const char *prefix)
{
	static char device_name[20] = {0};
	uint8_t mac[6] = {0};
	ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
	sprintf(device_name, "%s%02x%02x%02x%02x%02x%02x",prefix, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return device_name;
}

char *ConvertToCharLower(char *str)
{
	int i = 0;
	while (str[i] != '\0')
	{
		if (str[i] >= 'A' && str[i] <= 'Z')
		{
			str[i] = str[i] + 32;
		}
		i++;
	}
	return str;
}

char *hex_to_ascii(char *hex_string)
{
	int len = strlen(hex_string);
	if (len % 2 != 0)
		return NULL;
	int ascii_len = len / 2;
	char *ascii_string = (char *)malloc((ascii_len + 1) * sizeof(char));
	if (!ascii_string)
		return NULL;
	for (int i = 0; i < ascii_len; ++i)
	{
		char hex_byte[3] = {hex_string[2 * i], hex_string[2 * i + 1], '\0'};
		ascii_string[i] = (char)strtol(hex_byte, NULL, 16);
	}
	ascii_string[ascii_len] = '\0';
	return ascii_string;
}
