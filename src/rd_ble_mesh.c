/* main.c - Application main entry point */

/*
 * SPDX-FileCopyrightText: 2017 Intel Corporation
 * SPDX-FileContributor: 2018-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_lighting_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"
#include "rd_ble_mesh.h"


#define TAG "EXAMPLE_MESH"

#define CONFIG_MAX_NUM_ELEMENT  4

#define CID_ESP 0x0211
#define RD_OPCODE_E0            ESP_BLE_MESH_MODEL_OP_3(0xE0, CID_ESP)
#define RD_OPCODE_RSP_E0        ESP_BLE_MESH_MODEL_OP_3(0xE1, CID_ESP)
#define RD_OPCODE_E2            ESP_BLE_MESH_MODEL_OP_3(0xE2, CID_ESP)
#define RD_OPCODE_RSP_E2        ESP_BLE_MESH_MODEL_OP_3(0xE3, CID_ESP)

#define ESP_BLE_MESH_VND_MODEL_ID_CLIENT    0x0000
#define ESP_BLE_MESH_VND_MODEL_ID_SERVER    0x0001

static rd_handle_message_opcode_vender handle_mess_opcode_E0 = NULL;
static rd_handle_message_opcode_vender handle_mess_opcode_E2 = NULL;
static void ble_mesh_add_group(uint16_t id_group);
static void ble_mesh_del_group(uint16_t id_group);

static uint16_t GW_ADDR = 0x0001;
// UUID thiết bị
static uint8_t dev_uuid[16] = {0x28, 0x04};

// Khởi tạo các giá trị để cấu hình server
static esp_ble_mesh_cfg_srv_t config_server = {
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay = ESP_BLE_MESH_RELAY_ENABLED,
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
#if defined(CONFIG_BLE_MESH_GATT_PROXY_SERVER)
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
#else
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BLE_MESH_FRIEND)
    .friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
    .default_ttl = 7,
};

/*--------------------------scene-----------------------------*/
/**
 * @brief scene model
 * 
 */
ESP_BLE_MESH_MODEL_PUB_DEFINE(scene_pub_0, 5 + 3, ROLE_NODE);

/**
 * @brief generic model onoff 
 * 
 */
ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_pub_0, 2, ROLE_NODE);
static esp_ble_mesh_gen_onoff_srv_t onoff_server_0 = {
    .rsp_ctrl = {
        .get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
        .set_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    },
};

#if CONFIG_MAX_NUM_ELEMENT >=2
ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_pub_1, 2, ROLE_NODE);
static esp_ble_mesh_gen_onoff_srv_t onoff_server_1 = {
    .rsp_ctrl = {
        .get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
        .set_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    },
};
#endif

#if CONFIG_MAX_NUM_ELEMENT >=3
ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_pub_2, 2, ROLE_NODE);
static esp_ble_mesh_gen_onoff_srv_t onoff_server_2 = {
    .rsp_ctrl = {
        .get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
        .set_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    },
};
#endif

#if CONFIG_MAX_NUM_ELEMENT >=4
ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_pub_3, 2, ROLE_NODE);
static esp_ble_mesh_gen_onoff_srv_t onoff_server_3 = {
    .rsp_ctrl = {
        .get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
        .set_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP, //ESP_BLE_MESH_SERVER_RSP_BY_APP,
    },
};
#endif



#if CONFIG_ENABLE_LIGHT_DIM_CCT
/**
 * @brief lighting model: lightness
 * 
 */
static esp_ble_mesh_light_lightness_state_t lightness_state = {
    .lightness_linear          = 0x0000,
    .target_lightness_linear   = 0x0000,

    .lightness_actual          = 0x0000,
    .target_lightness_actual   = 0x0000,

    .lightness_last            = 0x0000,
    .lightness_default         = 0x0000,

    .status_code               = 0x00,     

    .lightness_range_min       = 0x0001,   
    .lightness_range_max       = 0xFFFF,   
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(lightness_pub_0, 2+3, ROLE_NODE);
static esp_ble_mesh_light_lightness_srv_t lightness_server_0 = {
    .rsp_ctrl = {
        .get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
        .set_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    },
    .state = &lightness_state,
};

/**
 * @brief lighting model: cct
 * 
 */
esp_ble_mesh_light_ctl_state_t light_cct ={
    .lightness           = 0x0000,
    .target_lightness    = 0x0000,

    .temperature         = 0x0320,   // 800 Kelvil
    .target_temperature  = 0x0320,   // 20000 Kelvil

    .delta_uv            = 0x0000,
    .target_delta_uv     = 0x0000,

    .status_code         = 0x00,     // SUCCESS

    .temperature_range_min = 0x0320,  // >= 800
    .temperature_range_max = 0x4E20,  // <= 20000

    .lightness_default   = 0x0000,
    .temperature_default = 0x0320,
    .delta_uv_default    = 0x0000,    
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(lightcct_pub_0, 2+3, ROLE_NODE);
static esp_ble_mesh_light_ctl_temp_srv_t lightcct_server_0 = {
    .rsp_ctrl = {
        .get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
        .set_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    },
    .state = &light_cct,
};
#endif

/**
 * @brief Cấu hình sig model cho các node
 * 
 */
static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_pub_0, &onoff_server_0),
#if CONFIG_ENABLE_LIGHT_DIM_CCT
    ESP_BLE_MESH_MODEL_LIGHT_LIGHTNESS_SRV(&lightness_pub_0, &lightness_server_0),
    ESP_BLE_MESH_MODEL_LIGHT_CTL_TEMP_SRV(&lightcct_pub_0, &lightcct_server_0),
#endif
    // ESP_BLE_MESH_MODEL_SCENE_SRV(&scene_pub_0, &scene_srv_0),       
    // ESP_BLE_MESH_MODEL_SCENE_SETUP_SRV(NULL, &scene_setup_srv),      
};

#if CONFIG_MAX_NUM_ELEMENT >=2
esp_ble_mesh_model_t sig_models1[] = {
    ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_pub_1, &onoff_server_1),
};
#endif

#if CONFIG_MAX_NUM_ELEMENT >=3
esp_ble_mesh_model_t sig_models2[] = {
    ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_pub_2, &onoff_server_2),
};
#endif

#if CONFIG_MAX_NUM_ELEMENT >=4
esp_ble_mesh_model_t sig_models3[] = {
    ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_pub_3, &onoff_server_3),
};
#endif

/**
 * @brief cấu hình opcode vender cho từng node
 * 
 */
static esp_ble_mesh_model_op_t vnd_op[] = {
    ESP_BLE_MESH_MODEL_OP(RD_OPCODE_E0, 2), // RD_NOTE: config opcode vender
    ESP_BLE_MESH_MODEL_OP(RD_OPCODE_E2, 2),
    ESP_BLE_MESH_MODEL_OP_END,
};

#if CONFIG_MAX_NUM_ELEMENT >=2
static esp_ble_mesh_model_op_t vnd_op1[] = {
    ESP_BLE_MESH_MODEL_OP(RD_OPCODE_E0, 2),
    ESP_BLE_MESH_MODEL_OP(RD_OPCODE_E2, 2),
    ESP_BLE_MESH_MODEL_OP_END,
};
#endif

#if CONFIG_MAX_NUM_ELEMENT >=3
static esp_ble_mesh_model_op_t vnd_op2[] = {
    ESP_BLE_MESH_MODEL_OP(RD_OPCODE_E0, 2),
    ESP_BLE_MESH_MODEL_OP(RD_OPCODE_E2, 2),
    ESP_BLE_MESH_MODEL_OP_END,
};
#endif

#if CONFIG_MAX_NUM_ELEMENT >=4
static esp_ble_mesh_model_op_t vnd_op3[] = {
    ESP_BLE_MESH_MODEL_OP(RD_OPCODE_E0, 2),
    ESP_BLE_MESH_MODEL_OP(RD_OPCODE_E2, 2),
    ESP_BLE_MESH_MODEL_OP_END,
};
#endif

/**
 * @brief Cấu hình vender model cho các node
 * 
 */
esp_ble_mesh_model_t vnd_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_SERVER, vnd_op, NULL, NULL),
};

#if CONFIG_MAX_NUM_ELEMENT >=2
esp_ble_mesh_model_t vnd_models1[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_SERVER, vnd_op1, NULL, NULL),
};
#endif

#if CONFIG_MAX_NUM_ELEMENT >=3
esp_ble_mesh_model_t vnd_models2[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_SERVER, vnd_op2, NULL, NULL),
};
#endif

#if CONFIG_MAX_NUM_ELEMENT >=4
esp_ble_mesh_model_t vnd_models3[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_SERVER, vnd_op3, NULL, NULL),
};
#endif

/**
 * @brief khởi tạo các element
 * 
 */
static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, vnd_models),
#if CONFIG_MAX_NUM_ELEMENT >=2
    ESP_BLE_MESH_ELEMENT(0, sig_models1, vnd_models1),
#endif
#if CONFIG_MAX_NUM_ELEMENT >=3
    ESP_BLE_MESH_ELEMENT(0, sig_models2, vnd_models2),
#endif
#if CONFIG_MAX_NUM_ELEMENT >=4
    ESP_BLE_MESH_ELEMENT(0, sig_models3, vnd_models3),
#endif
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .element_count = ARRAY_SIZE(elements),
    .elements = elements,
};

static esp_ble_mesh_model_t *sig_model_onoff[CONFIG_MAX_NUM_ELEMENT] = 
{
#if CONFIG_MAX_NUM_ELEMENT >= 1
    [0] = &root_models[1]
#endif
#if CONFIG_MAX_NUM_ELEMENT >= 2
    ,[1] = &sig_models1[0]
#endif
#if CONFIG_MAX_NUM_ELEMENT >= 3
    ,[2] = &sig_models2[0]
#endif
#if CONFIG_MAX_NUM_ELEMENT >= 4
    ,[3] = &sig_models3[0]
#endif
};

/* Disable OOB security for SILabs Android app */
static esp_ble_mesh_prov_t provision = {
    .uuid = dev_uuid,
#if 0
    .output_size = 4,
    .output_actions = ESP_BLE_MESH_DISPLAY_NUMBER,
    .input_size = 4,
    .input_actions = ESP_BLE_MESH_PUSH,
#else
    .output_size = 0,
    .output_actions = 0,
#endif
};

/**
 * @brief hàm log thông báo thiết bị đã provision thành công
 * 
 * @param net_idx index network
 * @param addr  địa chỉ thiết bị
 * @param flags 
 * @param iv_index chỉ số IV
 */
static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
    printf("provision success!!\n");
    ESP_LOGI(TAG, "net_idx: 0x%04x, addr: 0x%04x", net_idx, addr);
    ESP_LOGI(TAG, "flags: 0x%02x, iv_index: 0x%08" PRIx32, flags, iv_index);
}

/**
 * @brief hàm ví dụ điều khiển trạng thái led
 * 
 * @param ctx chứa địa chỉ của địa chỉ nguồn và địa chỉ đích
 * @param onoff trạng thái điều khiển
 */
static void example_change_led_state(esp_ble_mesh_model_t *model,
                                     esp_ble_mesh_msg_ctx_t *ctx, uint8_t onoff)
{
    uint16_t primary_addr = esp_ble_mesh_get_primary_element_address();
    uint8_t elem_count = esp_ble_mesh_get_element_count();
    uint8_t i;

    if (ESP_BLE_MESH_ADDR_IS_UNICAST(ctx->recv_dst)) {
        for (i = 0; i < elem_count; i++) {
            if (ctx->recv_dst == (primary_addr + i)) {
                //control 1 element
                printf("control ele: %d, addr: 0x%04X, stt: %d\n", i, ctx->recv_dst, onoff);
            }
        }
    } else if (ESP_BLE_MESH_ADDR_IS_GROUP(ctx->recv_dst)) {
        if (esp_ble_mesh_is_model_subscribed_to_group(model, ctx->recv_dst)) {
            // control group
            printf("control group addr: 0x%04X, stt: %d\n", ctx->recv_dst, onoff);
        }
    } else if (ctx->recv_dst == 0xFFFF) {
        // control all element
        printf("control all ele, stt: %d\n", onoff);
    }
}

/**
 * @brief hàm reset cứng thiết bị, hàm này sẽ xóa toàn bộ thông tin mạng mesh và khởi động lại chip
 * 
 */
void ble_mesh_kick_out(void)
{
    printf("BLE_MESH: kick out\n");
    vTaskDelay(500 / portTICK_PERIOD_MS);
    esp_ble_mesh_node_local_reset(); // reset 
    vTaskDelay(500 / portTICK_PERIOD_MS);
    esp_restart();
}

/**
 * @brief hàm xử lý điều khiển onoff khi set get_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP
 * 
 * @param model 
 * @param ctx 
 * @param set 
 */
static void example_handle_gen_onoff_msg(esp_ble_mesh_model_t *model,
                                         esp_ble_mesh_msg_ctx_t *ctx,
                                         esp_ble_mesh_server_recv_gen_onoff_set_t *set)
{
    esp_ble_mesh_gen_onoff_srv_t *srv = (esp_ble_mesh_gen_onoff_srv_t *)model->user_data;

    switch (ctx->recv_op) {
    case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET:
        esp_ble_mesh_server_model_send_msg(model, ctx,
            ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, sizeof(srv->state.onoff), &srv->state.onoff);
        break;
    case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET:
    case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK:
        if (set->op_en == false) {
            srv->state.onoff = set->onoff;
        } else {
            /* TODO: Delay and state transition */
            srv->state.onoff = set->onoff;
        }
        if (ctx->recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET) {
            esp_ble_mesh_server_model_send_msg(model, ctx,
                ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, sizeof(srv->state.onoff), &srv->state.onoff);
        }
        esp_ble_mesh_model_publish(model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS,
            sizeof(srv->state.onoff), &srv->state.onoff, ROLE_NODE);
        example_change_led_state(model, ctx, srv->state.onoff);
        break;
    default:
        break;
    }
}

/**
 * @brief hàm callback xử lý quá trình provision
 * 
 * @param event các sự kiện liên quan tới provision
 * @param param giá trị truyền đến
 */
static void example_ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                             esp_ble_mesh_prov_cb_param_t *param)
{
    switch (event) {
    case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT, err_code %d", param->node_prov_enable_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT, bearer %s",
            param->node_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT, bearer %s",
            param->node_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT");
        prov_complete(param->node_prov_complete.net_idx, param->node_prov_complete.addr, //NOTE event provision complete
            param->node_prov_complete.flags, param->node_prov_complete.iv_index);
        break;
    case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_RESET_EVT");
        //NOTE reset => kick out
        ble_mesh_kick_out();
        break;
    case ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT, err_code %d", param->node_set_unprov_dev_name_comp.err_code);
        break;
    default:
        break;
    }
}

/**
 * @brief hàm callback xử lý các sự kiện liên quan đến Binding, group, ...
 * 
 * @param event các sự kiện liên quan
 * @param param 
 */
static void example_ble_mesh_config_server_cb(esp_ble_mesh_cfg_server_cb_event_t event,
                                              esp_ble_mesh_cfg_server_cb_param_t *param)
{
    if (event == ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT)
    {
        switch (param->ctx.recv_op)
        {
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD");
            ESP_LOGI(TAG, "net_idx 0x%04x, app_idx 0x%04x",
                     param->value.state_change.appkey_add.net_idx,
                     param->value.state_change.appkey_add.app_idx);
            ESP_LOG_BUFFER_HEX("AppKey", param->value.state_change.appkey_add.app_key, 16);
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
            ESP_LOGI(TAG, "elem_addr 0x%04x, app_idx 0x%04x, cid 0x%04x, mod_id 0x%04x",
                     param->value.state_change.mod_app_bind.element_addr,
                     param->value.state_change.mod_app_bind.app_idx,
                     param->value.state_change.mod_app_bind.company_id,
                     param->value.state_change.mod_app_bind.model_id);
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD");
            ESP_LOGI(TAG, "elem_addr 0x%04x, sub_addr 0x%04x, cid 0x%04x, mod_id 0x%04x",
                     param->value.state_change.mod_sub_add.element_addr,
                     param->value.state_change.mod_sub_add.sub_addr,
                     param->value.state_change.mod_sub_add.company_id,
                     param->value.state_change.mod_sub_add.model_id);
            ble_mesh_add_group(param->value.state_change.mod_sub_add.sub_addr);
            
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_SUB_DELETE:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_SUB_DELETE delete group id: %04x", param->value.state_change.mod_sub_delete.sub_addr);
            ble_mesh_del_group(param->value.state_change.mod_sub_delete.sub_addr);
        default:
            break;
        }
    }
}

/*vender*/
/**
 * @brief hàm callback xử lý các sự kiện thuộc vender model
 * 
 * @param event 
 * @param param 
 */
static void example_ble_mesh_custom_model_cb(esp_ble_mesh_model_cb_event_t event,
                                             esp_ble_mesh_model_cb_param_t *param)
{
    ESP_LOGI(TAG, "src 0x%04x, dst 0x%04x",
             param->model_operation.ctx->addr, param->model_operation.ctx->recv_dst);
    switch (event)
    {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT:
        if (param->model_operation.opcode == RD_OPCODE_E0)
        {
            //NOTE todo
            if(handle_mess_opcode_E0) handle_mess_opcode_E0((ble_mesh_cb_param_t)param);
        }else if (param->model_operation.opcode == RD_OPCODE_E2)
        {
            //NOTE todo
            if(handle_mess_opcode_E2) handle_mess_opcode_E2((ble_mesh_cb_param_t)param);
        }

        break;
    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT: // RD_NOTE: log send msg from element
        if (param->model_send_comp.err_code)
        {
            ESP_LOGE(TAG, "Failed to send message 0x%06" PRIx32, param->model_send_comp.opcode);
            break;
        }
        ESP_LOGI(TAG, "Element: %u, Send rsp_opcode: 0x%06" PRIx32, param->model_send_comp.model->element_idx, param->model_send_comp.opcode); // RD_NOTE device rsp
        break;
    default:
        break;
    }
}

/**
 * @brief hàm callback xử lý các sự kiện thuộc Generic model
 * 
 * @param event 
 * @param param 
 */
static void example_ble_mesh_generic_server_cb(esp_ble_mesh_generic_server_cb_event_t event,
                                               esp_ble_mesh_generic_server_cb_param_t *param)
{
    esp_ble_mesh_gen_onoff_srv_t *srv;
    ESP_LOGI(TAG, "event 0x%02x, opcode 0x%04" PRIx32 ", src 0x%04x, dst 0x%04x",
        event, param->ctx.recv_op, param->ctx.addr, param->ctx.recv_dst);

    switch (event) {
    case ESP_BLE_MESH_GENERIC_SERVER_STATE_CHANGE_EVT:{ //NOTE set_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_SERVER_STATE_CHANGE_EVT");
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET ||
            param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK)
        {
            ESP_LOGI(TAG, "onoff 0x%02x", param->value.state_change.onoff_set.onoff);
            //NOTE todo
            example_change_led_state(param->model, &param->ctx, param->value.state_change.onoff_set.onoff);
        }

        break;
    }
    case ESP_BLE_MESH_GENERIC_SERVER_RECV_GET_MSG_EVT: //NOTE get_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_SERVER_RECV_GET_MSG_EVT");
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET) {
            srv = (esp_ble_mesh_gen_onoff_srv_t *)param->model->user_data;
            ESP_LOGI(TAG, "onoff 0x%02x", srv->state.onoff);
            example_handle_gen_onoff_msg(param->model, &param->ctx, NULL);
        }
        break;
    case ESP_BLE_MESH_GENERIC_SERVER_RECV_SET_MSG_EVT: //NOTE set_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_SERVER_RECV_SET_MSG_EVT");
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET ||
            param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK) {
            ESP_LOGI(TAG, "onoff 0x%02x, tid 0x%02x", param->value.set.onoff.onoff, param->value.set.onoff.tid);
            if (param->value.set.onoff.op_en) {
                ESP_LOGI(TAG, "trans_time 0x%02x, delay 0x%02x",
                    param->value.set.onoff.trans_time, param->value.set.onoff.delay);
            }
            example_handle_gen_onoff_msg(param->model, &param->ctx, &param->value.set.onoff);
        }
        break;
    default:
        ESP_LOGE(TAG, "Unknown Generic Server event 0x%02x", event);
        break;
    }
}

#if CONFIG_ENABLE_LIGHT_DIM_CCT
/**
 * @brief hàm callback xử lý các sự kiện Lighting model 
 * 
 * @param event 
 * @param param 
 */
static void example_ble_mesh_lighting_server_cb(esp_ble_mesh_lighting_server_cb_event_t event,
                                                   esp_ble_mesh_lighting_server_cb_param_t *param){
    ESP_LOGI(TAG, "event 0x%02x, opcode 0x%04" PRIx32 ", src 0x%04x, dst 0x%04x",
                event, param->ctx.recv_op, param->ctx.addr, param->ctx.recv_dst);
    switch(event){
        case ESP_BLE_MESH_LIGHTING_SERVER_STATE_CHANGE_EVT:{
            ESP_LOGI(TAG, "ESP_BLE_MESH_LIGHTING_SERVER_STATE_CHANGE_EVT");
            if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET ||
                param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET_UNACK)
            {
                ESP_LOGI(TAG, "lightness 0x%04x", param->value.state_change.lightness_set.lightness);
                //Todo
            }  
            if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_LIGHT_CTL_TEMPERATURE_SET ||
                param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_LIGHT_CTL_TEMPERATURE_SET_UNACK)
            {
                ESP_LOGI(TAG, "cct 0x%04x", param->value.state_change.ctl_temp_set.temperature);
                //Todo
            }            
            break;
        }
        default:
            break;
    }
    
}
#endif

/**
 * @brief hàm khởi tạo BLE mesh
 * 
 * @return ESP_OK khởi tạo thành công
 *         còn lại: khởi tạo thất bại
 */
static esp_err_t ble_mesh_init(void)
{
    esp_err_t err = ESP_OK;

    esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);
    esp_ble_mesh_register_config_server_callback(example_ble_mesh_config_server_cb);
    esp_ble_mesh_register_custom_model_callback(example_ble_mesh_custom_model_cb);     // vender
    esp_ble_mesh_register_generic_server_callback(example_ble_mesh_generic_server_cb);  // sigmesh: generic model
    // esp_ble_mesh_register_lighting_server_callback(example_ble_mesh_lighting_server_cb);// sigmesh: lighting model
    // esp_ble_mesh_register_time_scene_server_callback(ble_mesh_time_scene_server_callback); // scene model

    err = esp_ble_mesh_init(&provision, &composition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize mesh stack (err %d)", err);
        return err;
    }

    err = esp_ble_mesh_node_prov_enable((esp_ble_mesh_prov_bearer_t)(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable mesh node (err %d)", err);
        return err;
    }

    ESP_LOGI(TAG, "BLE Mesh Node initialized");
    return err;
}

void ble_mesh_get_dev_uuid(uint8_t *dev_uuid)
{
    if (dev_uuid == NULL) {
        ESP_LOGE(TAG, "%s, Invalid device uuid", __func__);
        return;
    }
    memcpy(dev_uuid + 2, esp_bt_dev_get_address(), BD_ADDR_LEN);
}

esp_err_t bluetooth_init(void)
{
    esp_err_t ret;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "%s initialize controller failed", __func__);
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed", __func__);
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "%s init bluetooth failed", __func__);
        return ret;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "%s enable bluetooth failed", __func__);
        return ret;
    }

    return ret;
}

/**
 * @brief hàm xử lý chính của chương trình
 * 
 */
void rd_ble_mesh_init(void)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing...");

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = bluetooth_init();
    if (err) {
        ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
        return;
    }

    ble_mesh_get_dev_uuid(dev_uuid);
    // init_scene_values(); //NOTE init scene value
    /* Initialize the Bluetooth Mesh Subsystem */
    err = ble_mesh_init();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
    }
    if (esp_ble_mesh_node_is_provisioned())
    {
        printf("provisioned\n");
    }
    else
    {
        printf("un provision\n");
    }
}

/*=====================================================================================
                                RANG DONG IMPLEMENT
=====================================================================================*/

void rd_ble_mesh_register_cb_handle_mess_opcode_E0(rd_handle_message_opcode_vender cb){
    if(cb) handle_mess_opcode_E0 = cb;
}

void rd_ble_mesh_register_cb_handle_mess_opcode_E2(rd_handle_message_opcode_vender cb){
    if(cb) handle_mess_opcode_E2 = cb;
}

void ble_mesh_get_mess_buf(ble_mesh_cb_param_t param, uint8_t **buff, uint16_t *len){
    esp_ble_mesh_model_cb_param_t *cb_par = (esp_ble_mesh_model_cb_param_t *)param;
    *buff = cb_par->model_operation.msg;
    *len = cb_par->model_operation.length;
}

uint32_t ble_mesh_get_opcode(ble_mesh_cb_param_t param){
    esp_ble_mesh_model_cb_param_t *cb_par = (esp_ble_mesh_model_cb_param_t *)param;
    return cb_par->model_operation.opcode;
}

esp_err_t ble_mesh_rsp_opcode_vender_E0(ble_mesh_cb_param_t param, uint8_t *par, uint8_t len){
    esp_ble_mesh_model_cb_param_t *cb_par = (esp_ble_mesh_model_cb_param_t *)param;
    esp_err_t err = esp_ble_mesh_server_model_send_msg(vnd_models,
                                                        cb_par->model_operation.ctx, RD_OPCODE_E0,
                                                        len, par);
    if (err)
    { 
        ESP_LOGE("MESS_RSP_E0", "Failed to send message 0x%06x", RD_OPCODE_RSP_E0);
    }
    return err;
}

esp_err_t ble_mesh_rsp_opcode_vender_E2(ble_mesh_cb_param_t param, uint8_t *par, uint8_t len){
    esp_ble_mesh_model_cb_param_t *cb_par = (esp_ble_mesh_model_cb_param_t *)param;
    esp_err_t err = esp_ble_mesh_server_model_send_msg(vnd_models,
                                                        cb_par->model_operation.ctx, RD_OPCODE_E2,
                                                        len, par);
    if (err)
    { 
        ESP_LOGE("MESS_RSP_E2", "Failed to send message 0x%06x", RD_OPCODE_RSP_E2);
    }
    return err;
}

esp_err_t ble_mesh_rsp_state(uint8_t eleIdx, uint8_t onoff){
    esp_err_t err = ESP_OK;
    esp_ble_mesh_msg_ctx_t ctx;
    ctx.net_idx = 0x0000;
    ctx.app_idx = 0x0000;
    ctx.addr = GW_ADDR; //df 0x0001
    ctx.send_ttl = ESP_BLE_MESH_TTL_DEFAULT;
    ctx.send_rel = false;  // no ack
    if (esp_ble_mesh_node_is_provisioned()){
        err = esp_ble_mesh_server_model_send_msg(sig_model_onoff[eleIdx], &ctx, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS,
                                                        1, &onoff);
        if (err)
        {
            ESP_LOGE("MESS_ONOFF", "Failed to send message 0x%04x", ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS);
        }
    }else{
        ESP_LOGW("MESS_ONOFF", "device unprovision");
    }
    return err;
}

static void ble_mesh_add_group(uint16_t id_group){
    uint16_t primary_addr = esp_ble_mesh_get_primary_element_address();
    uint16_t model_id = ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV;
    uint16_t company_id = 0xffff; // SIG_MODEL: company_id = 0xffff

    printf("add group: 0x%04x\n", id_group);
    esp_ble_mesh_model_subscribe_group_addr(primary_addr, company_id, model_id, id_group);
}


static void ble_mesh_del_group(uint16_t id_group){
    uint16_t primary_addr = esp_ble_mesh_get_primary_element_address();
    uint16_t model_id = ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV;
    uint16_t company_id = 0xffff; // SIG_MODEL: company_id = 0xffff

    printf("delete group: 0x%04x\n", id_group);
    esp_ble_mesh_model_unsubscribe_group_addr(primary_addr, company_id, model_id, id_group);

}

void ble_mesh_save_gw_addr(uint16_t addr){
    GW_ADDR = addr;
    //save to flash
}

uint16_t ble_mesh_get_gw_addr(void){
    return GW_ADDR;
}




