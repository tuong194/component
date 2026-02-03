#include "rd_uart.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char* TAG = "UART";
static QueueHandle_t uart_queue = NULL;

static void uart_init(const gpio_num_t tx_pin, const gpio_num_t rx_pin, \
    const uart_port_t port_num, const int baudrate, const int tx_rx_buff_size)
{
    const uart_config_t uart_config = {
        .baud_rate = baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_driver_install(port_num, tx_rx_buff_size * 2, tx_rx_buff_size * 2, 20, &uart_queue, 0);
    uart_param_config(port_num, &uart_config);
    uart_set_pin(port_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_LOGI(TAG, "UART initialized successfully");
}

void rd_uart_write_bytes(uart_port_t port_num, uint8_t* data, size_t size) {
    uart_write_bytes(port_num, data, size);
    uart_wait_tx_done(port_num, 100 / portTICK_PERIOD_MS); //wait max 100ms
}

void rd_uart_read_bytes(uart_port_t port_num, uint8_t* data, size_t size) {
    uart_read_bytes(port_num, data, size, 20 / portTICK_PERIOD_MS);
}
void rd_uart_clear_buffer(uart_port_t port_num) {
    uart_flush_input(port_num);
}

static void uart_task(void* param)
{
    rd_uart_config_t* my_uart = (rd_uart_config_t*)param;
    uart_init(my_uart->tx_pin, my_uart->rx_pin, my_uart->uart_num, my_uart->baudrate, my_uart->txrx_buff_size);
    ESP_LOGI(TAG, "===>> MY UART Task started");
    uart_event_t event;
    size_t len_buf = 0;
    size_t rx_count = 0;
    int64_t last_time_rec_uart = 0;
    while (1)
    {
        if (xQueueReceive(uart_queue, (void*)&event, 5))
        {
            switch (event.type)
            {
            case UART_DATA:
                if (esp_timer_get_time() - last_time_rec_uart > 1000) // us
                {
                    rx_count = 0;
                    // memset(my_uart->data_buffer, 0, MAX_DATA_BUFF_RX);
                    if (my_uart->reset_state)
                        my_uart->reset_state();
                }
                last_time_rec_uart = esp_timer_get_time();

                ESP_ERROR_CHECK(uart_get_buffered_data_len(my_uart->uart_num, (size_t*)&len_buf));
                if (my_uart->data_buffer) {
                    len_buf = uart_read_bytes(my_uart->uart_num, my_uart->data_buffer + rx_count, len_buf, portMAX_DELAY);
                    rx_count += len_buf;

                }

                if (my_uart->process_data) {
                    my_uart->process_data(len_buf);
                }

                break;
            case UART_FIFO_OVF:
                ESP_LOGW(TAG, "HW FIFO Overflow\n");
                uart_flush_input(my_uart->uart_num);
                xQueueReset(uart_queue);
                break;

            case UART_BUFFER_FULL:
                ESP_LOGW(TAG, "Ring Buffer Full\n");
                uart_flush_input(my_uart->uart_num);
                xQueueReset(uart_queue);
                break;

            case UART_BREAK:
                // ESP_LOGE(TAG,"UART Break\n");
                break;

            case UART_PARITY_ERR:
                ESP_LOGE(TAG, "Parity Error\n");
                break;

            case UART_FRAME_ERR:
                ESP_LOGE(TAG, "Frame Error\n");
                break;

            default:
                ESP_LOGE(TAG, "Other event: %d\n", event.type);
                break;
            }
        }
    }
}

void rd_uart_init_task(const rd_uart_config_t* my_uart) {
    xTaskCreate(uart_task, "rs485_uart_task", 2048, (void*)my_uart, configMAX_PRIORITIES - 3, NULL);
}