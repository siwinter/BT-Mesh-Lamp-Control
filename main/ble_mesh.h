#ifndef _BLE_MESH_H_
#define _BLE_MESH_H_

#include "esp_err.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_lighting_model_api.h"

typedef union client_status_cb_t
{
    esp_ble_mesh_gen_client_status_cb_t *   generic ;
    esp_ble_mesh_light_client_status_cb_t * light ;
} client_status_cb_t;

esp_err_t ble_mesh_init(void) ;

typedef void (* ble_model_evt_cb_t)(client_status_cb_t* status, esp_ble_mesh_client_common_param_t* params) ;

void register_evt_cb(ble_model_evt_cb_t callback) ;

void send_gen_onoff_set(uint16_t adr, bool state);
void send_level_set(uint16_t adr, uint16_t level) ;
void send_light_set(uint16_t adr, uint16_t level) ;

#endif