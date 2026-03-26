#include "gcool.h"
#include "gcool_def.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#define GCOOL_ID_DEFAULT      UART_NUM_0
#define GCOOL_TX_PIN_DEFAULT  GPIO_NUM_21 //UART_PIN_NO_CHANGE 
#define GCOOL_RX_PIN_DEFAULT  GPIO_NUM_20 //UART_PIN_NO_CHANGE 

#define GCOOL_BAUD_DEFAULT                9600
#define GCOOL_TXRX_BUF_SIZE_DEFAULT       256

#define HEADER1 0x55
#define HEADER2 0xAA

#define MAX_DATA_BUFF_RX    128

#define TIME_OUT_PING       30*1000*1000 // 15s
#define TIME_OUT_PING_FAIL  1000*1000 // 1000ms

typedef enum {
    CHECK_DATA = 0,
    HANDLE_DATA
}process_state_e;

typedef struct {
    process_state_e state;
}check_data_rec;

typedef struct __attribute__((packed))
{
    uint8_t header[2];
    uint8_t version;
    uint8_t command_word;
    uint8_t length[2];
    uint8_t data[100];
    //checksum: 1 byte; 
}gcool_packet_t;

typedef struct __attribute__((packed))
{
    uint8_t DP_ID;
    uint8_t data_type;
    uint16_t data_length;
    uint8_t* func_cmd;
}data_packet_product_func_t;

static const char* GCOOL_TAG = "GCOOL";
static uint8_t data_buff[MAX_DATA_BUFF_RX] = { 0 };
static check_data_rec check_data;
static gcool_packet_t* data_receive;
static bool flag_check_heartbreak = false;
static uint8_t status_wifi = AP_MODE;
static uint8_t pos_start_frame = 0;
static bool is_ping_heartbeat = true;


static void gcool_process_data(uint8_t len);
static void gcool_reset_check_data(void);
esp_err_t gcool_check_header(const gcool_packet_t* data_rec);
esp_err_t gcool_check_sum(const gcool_packet_t* data_rec, uint8_t* buff, uint8_t pos);

static esp_err_t handle_CMD_HEARTBEAT(uint8_t data_rsp);
static esp_err_t handle_CMD_PRODUCT_INFO(uint8_t* data, uint8_t length);
static esp_err_t mcu_rsp_working_stt_handle(uint8_t* data, uint8_t length);
static esp_err_t handle_CMD_WORKING_MODE(uint8_t* data, uint8_t length);
static esp_err_t handle_CMD_GET_STT_WIFI(uint8_t* data, uint8_t length);

esp_err_t ping_heartbreak(void);
esp_err_t query_product_info(void);
esp_err_t query_working_mode(void);
esp_err_t report_wifi_stt(uint8_t stt);

extern esp_err_t Handle_Product_Function(uint8_t* data);

static rd_uart_config_t gcool_uart_config = {
    .tx_pin = GCOOL_TX_PIN_DEFAULT,
    .rx_pin = GCOOL_RX_PIN_DEFAULT,
    .uart_num = GCOOL_ID_DEFAULT,
    .baudrate = GCOOL_BAUD_DEFAULT,
    .txrx_buff_size = GCOOL_TXRX_BUF_SIZE_DEFAULT,
    .data_buffer = data_buff,
    .process_data = gcool_process_data,
    .reset_state = gcool_reset_check_data
};

static inline void gcool_set_pos_start_frame(uint8_t pos){
    pos_start_frame = pos;
}

static inline uint8_t gcool_get_pos_start_frame(void){
    return pos_start_frame;
}

uint8_t check_sum(uint8_t* data, uint8_t leng)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < leng; i++)
    {
        sum += data[i];
    }
    return sum;
}

static esp_err_t gcool_gen_product_func(data_packet_product_func_t* data_func) {
    gcool_packet_t data_packet_send = { 0 };
    uint8_t length = 4 + data_func->data_length; // DP_ID + data_type + data_length + func_cmd
    uint8_t* data = malloc(length);
    data[0] = data_func->DP_ID;
    data[1] = data_func->data_type;
    data[2] = (data_func->data_length >> 8) & 0xff;
    data[3] = data_func->data_length & 0xff;
    if (data_func->data_length > 0)
    {
        memcpy(&data[4], data_func->func_cmd, data_func->data_length);
    }
    data_packet_send.header[0] = HEADER1;
    data_packet_send.header[1] = HEADER2;
    data_packet_send.version = VERSION_MODULE;
    data_packet_send.command_word = CMD_PRODUCT_FUNC;
    data_packet_send.length[0] = (length >> 8) & 0xff;
    data_packet_send.length[1] = length & 0xff;
    memcpy(data_packet_send.data, data, length);
    data_packet_send.data[length] = check_sum((uint8_t*)&data_packet_send, length + 6); //check_sum
    rd_uart_write_bytes(gcool_uart_config.uart_num, (uint8_t*)&data_packet_send, length + 7);
    return ESP_OK;
}

esp_err_t gcool_init_product_cmd(uint8_t DP_IP, uint8_t data_type, uint16_t data_length, uint8_t* func_cmd) {
    data_packet_product_func_t data_func = { 0 };
    data_func.DP_ID = DP_IP;
    data_func.data_type = data_type;
    data_func.data_length = data_length;
    data_func.func_cmd = func_cmd;
    return gcool_gen_product_func(&data_func);
}

esp_err_t gcool_init_basic_cmd(uint8_t ver_module, uint8_t cmd_word, uint8_t length, uint8_t* data) {
    gcool_packet_t data_packet_send = { 0 };
    data_packet_send.header[0] = HEADER1;
    data_packet_send.header[1] = HEADER2;
    data_packet_send.version = ver_module;
    data_packet_send.command_word = cmd_word;
    data_packet_send.length[0] = (length >> 8) & 0xff;
    data_packet_send.length[1] = length & 0xff;
    if (length > 0)
        memcpy(data_packet_send.data, data, length);
    data_packet_send.data[length] = check_sum((uint8_t*)&data_packet_send, length + 6); //check_sum
    rd_uart_write_bytes(gcool_uart_config.uart_num, (uint8_t*)&data_packet_send, length + 7);
    return ESP_OK;
}

void gcool_set_status_wifi(uint8_t stt) {
    status_wifi = stt;
}

uint8_t gcool_get_status_wifi(void) {
    return status_wifi;
}

static void gcool_process_data(uint8_t len) {
    uint8_t count_check = 0;
    esp_err_t ret = ESP_OK;
    while (len--) {
        data_receive = (gcool_packet_t*)(data_buff + count_check);

        ret = gcool_check_header(data_receive);
        if (ret != ESP_OK) {
            count_check++;
            continue;
        }

        gcool_set_pos_start_frame(count_check);
        ret = gcool_check_sum(data_receive, data_buff, count_check);
        if (ret == ESP_OK){
            check_data.state = HANDLE_DATA;
            return;
        } 
        else check_data.state = CHECK_DATA;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static void gcool_reset_check_data(void) {
    check_data.state = CHECK_DATA;
    memset(data_buff, 0, MAX_DATA_BUFF_RX);
}

esp_err_t gcool_check_header(const gcool_packet_t* data_rec) {
    if (data_rec->header[0] == HEADER1 && data_rec->header[1] == HEADER2) {
        // ESP_LOGI(GCOOL_TAG, "check header done");
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t gcool_check_sum(const gcool_packet_t* data_rec, uint8_t* buff, uint8_t pos) {
    uint16_t leng_data = (data_rec->length[0] << 8) | data_rec->length[1];
    uint8_t checksum = buff[pos + leng_data + 6];
    if (check_sum(&buff[pos], leng_data + 6) == checksum)
    {
        // ESP_LOGI(GCOOL_TAG, "Checksum is valid");
        return ESP_OK;
    }
    ESP_LOGE(GCOOL_TAG, "Checksum failed");
    return ESP_FAIL;
}

int gcool_handle_data(void) {
    esp_err_t ret = ESP_OK;
    if (check_data.state == HANDLE_DATA) {
        uint16_t leng_data = (data_receive->length[0] << 8) | data_receive->length[1];
        //ESP_LOGI(GCOOL_TAG, "Processing data...");

        switch (data_receive->command_word)
        {
        case CMD_HEARTBEAT:
            ESP_LOGI(GCOOL_TAG, "CMD Heartbeat");
            ret = handle_CMD_HEARTBEAT(data_receive->data[0]);
            break;
        case CMD_PRODUCT_INFO:
            ESP_LOGI(GCOOL_TAG, "CMD product info");
            ret = handle_CMD_PRODUCT_INFO(data_receive->data, leng_data);
            break;
        case CMD_WORKING_MODE:
            ESP_LOGI(GCOOL_TAG, "CMD working mode");
            ret = handle_CMD_WORKING_MODE(data_receive->data, leng_data);
            break;
        case CMD_WIFI_STT:
            ESP_LOGI(GCOOL_TAG, "CMD wifi status");
            break;
        case CMD_NETWORK_CONFIG_MODE:
            ESP_LOGI(GCOOL_TAG, "CMD_NETWORK_CONFIG_MODE");
            ret = gcool_init_basic_cmd(VERSION_MODULE, CMD_NETWORK_CONFIG_MODE, 0, NULL);
            break;
        case CMD_WORKING_STT_MCU:
            ESP_LOGI(GCOOL_TAG, "CMD working status");
            ret = mcu_rsp_working_stt_handle(data_receive->data, leng_data);
            break;
        case CMD_GET_STT_WIFI:
            //ESP_LOGI(GCOOL_TAG, "CMD get stt wifi");
            ret = handle_CMD_GET_STT_WIFI(data_receive->data, leng_data);
            break;
        default:
            ESP_LOGE(GCOOL_TAG, "Unknown command: %02X", data_receive->command_word);
            break;
        }

        // gcool_reset_check_data();
        uint8_t pos_start_frame_current = gcool_get_pos_start_frame();
        uint8_t pos_start_frame_next = pos_start_frame_current + 7 + leng_data; 

        if(data_buff[pos_start_frame_next] == HEADER1 && data_buff[pos_start_frame_next+1] == HEADER2){
            is_ping_heartbeat = false;
            data_receive = (gcool_packet_t *)&data_buff[pos_start_frame_next];
            gcool_set_pos_start_frame(pos_start_frame_next);
            return gcool_handle_data();
        }else{
            is_ping_heartbeat = true;
            gcool_reset_check_data();
        }
    }
    return ret;
}

static void gcool_handle_task_cb(void* arg) {
    gcool_handle_data();
}

void gcool_init_handle_data_task(void)
{
    static esp_timer_handle_t g_gcool_timer_handle = NULL;
    if (!g_gcool_timer_handle) {
        esp_timer_create_args_t gcool_timer = { 0 };
        gcool_timer.arg = NULL;
        gcool_timer.callback = gcool_handle_task_cb;
        gcool_timer.dispatch_method = ESP_TIMER_TASK;
        gcool_timer.name = "gcool_timer";
        esp_timer_create(&gcool_timer, &g_gcool_timer_handle);
        esp_timer_start_periodic(g_gcool_timer_handle, 10 * 1000U); //10ms
    }
}

/***********************************HANDLE FUNCTION*****************************************/

static esp_err_t handle_CMD_HEARTBEAT(uint8_t data_rsp)
{
    // ESP_LOGW(GCOOL_TAG, "data ping: %02X", data_rsp);
    if (data_rsp == 0x00)
    {
        if (flag_check_heartbreak == false)
        {
            flag_check_heartbreak = true;
            query_product_info();
        }
        else
            flag_check_heartbreak = false;
    }
    else if (data_rsp == 0x01)
    {
        flag_check_heartbreak = true;
    }
    return ESP_OK;
}

static esp_err_t handle_CMD_PRODUCT_INFO(uint8_t* data, uint8_t length)
{
    char* product_info = malloc(length * sizeof(char));
    memcpy(product_info, data, length);
    printf("GCOOL info: %.*s\n", length, product_info);
    free(product_info);
    query_working_mode();
    return ESP_OK;
}

static esp_err_t handle_CMD_WORKING_MODE(uint8_t* data, uint8_t length)
{
    if (length > 0)
    {
        ESP_LOGI(GCOOL_TAG, "processing by the module: %02x, %02x", data[0], data[1]);
        // uint8_t stt_led_wifi = data[0];
        // uint8_t stt_btn_reset_wifi = data[1];
        //TODO
    }
    report_wifi_stt(status_wifi);
    return ESP_OK;
}

static esp_err_t handle_CMD_GET_STT_WIFI(uint8_t* data, uint8_t length) {
    if (length > 0) {
        return gcool_init_basic_cmd(VERSION_MODULE, CMD_GET_STT_WIFI, length, data);
    }
    return gcool_init_basic_cmd(VERSION_MODULE, CMD_GET_STT_WIFI, 1, &status_wifi);
}

// Communication protocol (product function)
static esp_err_t mcu_rsp_working_stt_handle(uint8_t* data, uint8_t length)
{
    esp_err_t err = ESP_OK;
    if (length == 0)
    {
        ESP_LOGE(GCOOL_TAG, "no data rsp");
        return ESP_FAIL;
    }
    else
    {
        err = Handle_Product_Function(data);
    }
    return err;
}

/*===========================MODULE QUERY,MODULE SEND===========================*/
esp_err_t ping_heartbreak(void)
{
    return gcool_init_basic_cmd(VERSION_MODULE, CMD_HEARTBEAT, 0, NULL);
}
esp_err_t query_product_info(void)
{
    ESP_LOGI(GCOOL_TAG, "get product info");
    return gcool_init_basic_cmd(VERSION_MODULE, CMD_PRODUCT_INFO, 0, NULL);
}
esp_err_t query_working_mode(void)
{
    ESP_LOGI(GCOOL_TAG, "get working mode");
    return gcool_init_basic_cmd(VERSION_MODULE, CMD_WORKING_MODE, 0, NULL);
}

esp_err_t report_wifi_stt(uint8_t stt)
{
    ESP_LOGI(GCOOL_TAG, "rsp stt wifi: 0x%02x", stt);
    return gcool_init_basic_cmd(VERSION_MODULE, CMD_WIFI_STT, 1, &stt);
}

/*********************************************************************************/

void loop_hertbreak_detect(void)
{
    if(!is_ping_heartbeat) return;
    static int64_t last_heartbreak_time = 0;
    if (!flag_check_heartbreak)
    {
        if (esp_timer_get_time() - last_heartbreak_time > TIME_OUT_PING_FAIL)
        {
            ESP_LOGW(GCOOL_TAG, "sending heartbreak detection");
            ping_heartbreak();
            last_heartbreak_time = esp_timer_get_time();
        }
    }
    else
    {
        if (esp_timer_get_time() - last_heartbreak_time > TIME_OUT_PING)
        {
            ESP_LOGW(GCOOL_TAG, "sending heartbreak detection");
            ping_heartbreak();
            last_heartbreak_time = esp_timer_get_time();
        }
    }
}

static void gcool_task(void* pvParameters)
{

    while (1)
    {
        loop_hertbreak_detect();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void gcool_init(const gpio_num_t tx_pin, const gpio_num_t rx_pin, const uart_port_t port_num) {
    static bool is_config_uart = false;
    gcool_uart_config.tx_pin = tx_pin;
    gcool_uart_config.rx_pin = rx_pin;
    gcool_uart_config.uart_num = port_num;
    if (!is_config_uart) {
        rd_uart_init_task(&gcool_uart_config);
        is_config_uart = true;
    }
    gcool_init_handle_data_task();
    xTaskCreate(gcool_task, "gcool_task", 2048, NULL, configMAX_PRIORITIES - 3, NULL);
}