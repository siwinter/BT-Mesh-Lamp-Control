/* main.cpp - Application main entry point
 *
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
extern "C" {
#include "esp_err.h"
#include "esp_log.h"
#include "uart.h"
#include "ble_mesh.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_lighting_model_api.h"
}
#define TAG "MAIN"

void eventReceived (client_status_cb_t* status, esp_ble_mesh_client_common_param_t* params) {
    char outString[100];
    strcpy(outString, ">evt/BT_");
    if (params->ctx.addr < 16) strcat(outString, "0") ;
    itoa (params->ctx.addr, outString + strlen(outString), 16);
    strcat(outString, "/") ;
    switch (params->ctx.recv_op) {
        case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS:
            if (status->generic->onoff_status.present_onoff) strcat(outString, "state:on\n") ;
            else strcat(outString, "state:off\n") ;
            break;
        case ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_STATUS:
            strcat(outString, "level:") ;
            itoa (status->generic->level_status.present_level,outString + strlen(outString), 10);
            strcat(outString, "\n") ;
            break;
        case ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_STATUS:
            strcat(outString, "light:") ;
            itoa (status->light->lightness_status.present_lightness,outString + strlen(outString), 10);
            strcat(outString, "\n") ;
            break;
        case ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_STATUS:
            strcat(outString, "range:") ;
            itoa (status->light->lightness_range_status.range_min,outString + strlen(outString),10) ;
            strcat(outString, "-") ;
            itoa (status->light->lightness_range_status.range_max,outString + strlen(outString),10) ;
            strcat(outString, "\n") ;
            break; }
    tx_task(outString) ;}

void msgReceived(char* msg){
    int i ;
    for(i=0 ; i < 7 ; i++) if ("cmd/BT_"[i] != msg[i]) return ;
    uint16_t adr = 0 ;
    char* cmd ;
    char* param ;
    adr = strtol (&msg[i], &cmd, 16);
    if (cmd[0] == '/') {cmd = cmd +1 ;} else return ;
    while(msg[i] != ':') {i++ ;} msg[i++] = 0;
    param = msg + i ;
    int16_t value ;
    printf("cmd: %s, param: %s, adr: %i \n", cmd, param, adr) ;
    if (strcmp(cmd,"state") == 0) {
        if (strcmp(param,"on") == 0) set_gen_onoff(adr, true);
        else if (strcmp(param,"off") == 0) set_gen_onoff(adr, false) ; }
    else if (strcmp(cmd,"light") == 0) { set_light_lightness(adr, strtol(param, NULL, 10)) ; }
    else if (strcmp(cmd,"get") == 0) {
        if (strcmp(param,"state") == 0) get_gen_onoff(adr) ;
        else if (strcmp(param,"level") == 0) get_gen_level(adr) ;
        else if (strcmp(param,"light") == 0) get_light_lightness(adr) ;
        else if (strcmp(param,"range") == 0) get_light_range(adr) ; } }

extern "C" void app_main(void) {
    esp_err_t err;
    ESP_LOGI(TAG, "Initializing...");
    uartInit(msgReceived);
    err = ble_mesh_init();
    if (err) ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
    register_evt_cb(eventReceived) ; }
