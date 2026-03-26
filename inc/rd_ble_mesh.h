#ifndef _RD_BLE_MESH_H__
#define _RD_BLE_MESH_H__

#include "stdint.h"
#include "esp_err.h"

typedef void *ble_mesh_cb_param_t; //ble_mesh_cb_param_t ~ (void *)
typedef void (*rd_handle_message_opcode_vender)(ble_mesh_cb_param_t param);

void      rd_ble_mesh_init(void);
void      rd_ble_mesh_register_cb_handle_mess_opcode_E0(rd_handle_message_opcode_vender cb);
void      rd_ble_mesh_register_cb_handle_mess_opcode_E2(rd_handle_message_opcode_vender cb);
void      ble_mesh_get_mess_buf(ble_mesh_cb_param_t param, uint8_t **buff, uint16_t *len);
uint32_t  ble_mesh_get_opcode(ble_mesh_cb_param_t param);
esp_err_t ble_mesh_rsp_opcode_vender_E0(ble_mesh_cb_param_t param, uint8_t *par, uint8_t len);
esp_err_t ble_mesh_rsp_opcode_vender_E2(ble_mesh_cb_param_t param, uint8_t *par, uint8_t len);
esp_err_t ble_mesh_rsp_state(uint8_t eleIdx, uint8_t onoff);
void      ble_mesh_save_gw_addr(uint16_t addr);
uint16_t  ble_mesh_get_gw_addr(void);
void      ble_mesh_kick_out(void);

#endif /* _RD_BLE_MESH_H__ */
