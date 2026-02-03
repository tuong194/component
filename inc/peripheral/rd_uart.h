#ifndef _RD_UART_H__
#define _RD_UART_H__

#include <stdint.h>
#include "driver/uart.h"
#include <driver/gpio.h>

// #define MAX_DATA_BUFF_RX 100

typedef struct {
    gpio_num_t tx_pin;
    gpio_num_t rx_pin;
    uart_port_t uart_num;
    int baudrate;
    uint16_t txrx_buff_size;
    uint8_t* data_buffer;
    void (*reset_state)(void);
    void (*process_data)(uint8_t len);
}rd_uart_config_t;


void rd_uart_init_task(const rd_uart_config_t* my_uart);
void rd_uart_write_bytes(uart_port_t port_num, uint8_t* data, size_t size);
void rd_uart_read_bytes(uart_port_t port_num, uint8_t* data, size_t size);
void rd_uart_clear_buffer(uart_port_t port_num);

#endif
