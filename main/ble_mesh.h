#ifndef _BLE_MESH_H_
#define _BLE_MESH_H_

#include "esp_err.h"
esp_err_t ble_mesh_init(void) ;

typedef void (* ble_model_evt_cb_t)(uint16_t adr, uint16_t opcode, uint16_t* params) ;

void register_evt_cb(ble_model_evt_cb_t callback) ;

void send_gen_onoff_set(uint16_t adr, bool state);
void send_level_set(uint16_t adr, uint16_t level) ;
void send_light_set(uint16_t adr, uint16_t level) ;

#endif