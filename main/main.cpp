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
}
#define TAG "MAIN"

void sendMsg(char* msg) {}

class cLamp {
    private:
        uint16_t address ;
        char name[19] ;
    public :
        cLamp(uint16_t a) {
            address = a ;
            strcpy(name,"lamp1") ; } ;

        void onEvent(uint16_t opcode, uint16_t* params) {
            printf("lamp->onEvent opcode: %i \n", opcode) ;
            char outString[20];
            strcpy(outString, ">evt/");
            strcat(outString, name) ;
            strcat(outString, ":") ;
            switch (opcode) {
            case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS:
                if(params[0] == 0) strcat(outString, "off") ;
                else strcat(outString, "on") ;
                break;
            case ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_STATUS:
                strcat(outString, "level,") ;
                itoa (params[0],outString + strlen(outString), 10);
                break;
            case ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_STATUS:
                strcat(outString, "range,") ;
                itoa (params[0],outString + strlen(outString),10) ;
                strcat(outString, ",") ;
                itoa (params[1],outString + strlen(outString),10) ;
                break;
            default:
                printf("unexpected opcode: %i", opcode) ;
                return ;  }   // nothing to send
            sendMsg(outString) ; }

        bool onMsg(char* msg) {
            printf("onmsg: msg0 = %s \n",msg);
            printf("onmsg: name = %s \n",name);

            int i ;
            for(i=0 ; i < 4 ; i++) if ("cmd/"[i] != msg[i]) return false ;
            msg = msg + i ;
            printf("onMsg: msg0 = %s \n",msg);
            for(i=0 ; i < strlen(name) ; i++) if ( name[i] != msg[i] ) return false ;
            if (msg[i++] != ':') return false ;
            msg = msg + i ;
            printf("onMsg: msg1 = %s \n",msg);
            char * param = strchr(msg, ',');
            if (param != NULL) {
                param[0] = 0 ;
                param = param + 1 ; }

            if ( strcmp(msg,"on") == 0) {send_gen_onoff_set(address, true); }
            else if ( strcmp(msg,"off") == 0) {send_gen_onoff_set(address, false); }
            else {
                i = 0 ;
                uint16_t level = 0 ;
                while (param[i]){
                    if ( (param[i] < '0') || (param[i] > '9') ) return true;
                    level = level * 10 + (param[i] - '0');
                    i++; }
                if ( strcmp(msg,"level") == 0) {send_level_set(address, level) ; }
                else if ( strcmp(msg,"light") == 0) {send_light_set(address, level) ; }
            }
            return true ;}
        uint16_t getAddress() { return address ;} ;
} ;

#define lampsMax 16

class cLamps {
    private :
        uint8_t nbrOfLamps ;
        cLamp* lamps[lampsMax] ;
    public :
        cLamps() { nbrOfLamps = 0 ; }
        void onEvent(uint16_t adr, uint16_t opcode, uint16_t* params) {
            cLamp * l = NULL ;
            for (int i=0 ; i<nbrOfLamps ; i++) if(lamps[i]->getAddress() == adr) l=lamps[i] ;
            if (l == NULL) {
                if (nbrOfLamps < lampsMax) {
                    l = new cLamp(adr) ;
                    lamps[nbrOfLamps++] = l ; }
                else return ;
            }
            printf("onEvent Lamp found \n") ;
            l->onEvent(opcode, params) ; 
        }
        void onMsg(char* msg) {
            printf("theLamps onMsg nbrOfLamps = %i \n", nbrOfLamps) ;
            for (int i=0 ; i<nbrOfLamps ; i++) if (lamps[i]->onMsg(msg)) return; 
        printf("onMsg lamp not found");}

} ;

cLamps * theLamps ;

void modelCb (uint16_t adr, uint16_t opcode, uint16_t* params) {
    theLamps->onEvent(adr, opcode, params) ; };

void msgReceived(char* msg){
    printf("main msgRecieved msg: %s \n", msg);
    theLamps->onMsg(msg);}

extern "C" void app_main(void)
{
    esp_err_t err;
    ESP_LOGI(TAG, "Initializing...");
    board_init();
    uartInit(msgReceived);

    /* Initialize the Bluetooth Mesh Subsystem */
    err = ble_mesh_init();
    if (err) ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);

    register_ble_model_evt(modelCb) ;

    theLamps = new cLamps ;
}
