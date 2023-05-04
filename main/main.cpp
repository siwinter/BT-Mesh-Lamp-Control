/* main.cpp - Application main entry point */

/*
 * SPDX-FileCopyrightText: 2017 Intel Corporation
 * SPDX-FileContributor: 2018-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
extern "C" {
#include "esp_err.h"
#include "esp_log.h"
#include "board.h"
#include "uart.h"

#include "ble_mesh.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_lighting_model_api.h"
}
#define TAG "MAIN"

class cLamp {
    private:
        uint16_t address ;
        char name[19] ;
    public :
        cLamp(uint16_t a, uint8_t n) {
            address = a ;
            strcpy(name,"lamp") ;
            itoa(n, name + strlen(name), 10) ;} ;
        cLamp(uint16_t a, const char* n) {
            address = a ;
            strcpy(name, n) ;} ;

        bool onEvent(client_status_cb_t* status, esp_ble_mesh_client_common_param_t* params) {
            if (params->ctx.addr != address) return false ;
            char outString[100];
            strcpy(outString, ">evt/");
            strcat(outString, name) ;
            strcat(outString, ":") ;
            switch (params->opcode) {
            case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS:
                if (status->generic->onoff_status.present_onoff) strcat(outString, "on\n") ;
                else strcat(outString, "off\n") ;
                break;
            case ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_STATUS:
                strcat(outString, "light,") ;
                itoa (status->light->lightness_status.present_lightness,outString + strlen(outString), 10);
                strcat(outString, "\n") ;
                break;
            case ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_STATUS:
                strcat(outString, "range,") ;
                itoa (status->light->lightness_range_status.range_min,outString + strlen(outString),10) ;
                strcat(outString, ",") ;
                itoa (status->light->lightness_range_status.range_max,outString + strlen(outString),10) ;
                strcat(outString, "\n") ;
                break;
            default:
//                printf("unexpected opcode: %i", opcode) ;
                return true;  }   // nothing to send
            tx_task(outString) ; 
            return true; }

        bool onMsg(char* msg) {

            int i ;
            for(i=0 ; i < 4 ; i++) if ("cmd/"[i] != msg[i]) return false ;
            msg = msg + i ;
            for(i=0 ; i < strlen(name) ; i++) if ( name[i] != msg[i] ) return false ;
            if (msg[i++] != ':') return false ;
            msg = msg + i ;
            uint8_t paramIndex = 0 ;
            uint16_t paramArray[2] ;
            char * params = strchr(msg, ',');
            if (params != NULL) {
                params [0] = 0 ;
                params = params +1 ;}

            while (params != NULL) {
                char* nextParam = params ;
                params = strchr(params, ',');
                if(params != NULL) {
                    params[0] = 0 ;
                    params = params + 1 ; }
                paramArray[paramIndex] = 0 ;
                i = 0 ;
                while (nextParam[i]) {
                    if ( (nextParam[i] < '0') || (nextParam[i] > '9') ) return true;
                    paramArray[paramIndex] = paramArray[paramIndex] * 10 + (nextParam[i] - '0');
                    i++; }
                paramIndex++ ;}
            printf( "onMsg paramIndex = %i :", paramIndex) ;
            for (int j=0 ; j< paramIndex ; j++) printf(" %i", paramArray[j]) ;
            printf("\n") ;

            if (paramIndex == 0) {
                if ( strcmp(msg,"on") == 0) send_gen_onoff_set(address, true);
                else if ( strcmp(msg,"off") == 0) send_gen_onoff_set(address, false); }
            else if (paramIndex == 1) {
                if ( strcmp(msg,"level") == 0) {send_level_set(address, paramArray[0]) ; }
                else if ( strcmp(msg,"light") == 0) {send_light_set(address, paramArray[0]) ; } }
            return true ;
            }
        uint16_t getAddress() { return address ;} ;
} ;

#define lampsMax 16
uint8_t nbrOfLamps ;
cLamp* theLamps[lampsMax] ;

void eventReceived (client_status_cb_t* status, esp_ble_mesh_client_common_param_t* params) {
    for (int i=0 ; i<nbrOfLamps ; i++) 
        if (theLamps[i]->onEvent(status, params)) return;
    cLamp* l = new cLamp(params->ctx.addr, nbrOfLamps) ;
    theLamps[nbrOfLamps++] = l ;
    l->onEvent(status, params) ;
};

void msgReceived(char* msg){
    for (int i=0 ; i<nbrOfLamps ; i++) if (theLamps[i]->onMsg(msg)) return;
    printf("msgReceived: lamp not found");}

extern "C" void app_main(void)
{
    esp_err_t err;
    ESP_LOGI(TAG, "Initializing...");
    board_init();
    uartInit(msgReceived);

    /* Initialize the Bluetooth Mesh Subsystem */
    err = ble_mesh_init();
    if (err) ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);

    register_evt_cb(eventReceived) ;

    nbrOfLamps = 0 ;
    theLamps[nbrOfLamps++] = new cLamp(0xFFFF, "lamps") ;
}
