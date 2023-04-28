/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _UART_H_
#define _UART_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

#define UART_TAG "UART"

static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)


# define bufMax 100

typedef void (* uart_cb_t)(char* data) ;

uart_cb_t msgCb ;
void tx_task(char* txt) { uart_write_bytes(UART_NUM_1, txt, strlen(txt)); }

char msg[bufMax] ;
bool msgReading = false ;
int8_t msgIndex = 0 ;

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);

    uint8_t* inBuffer = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, inBuffer, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            inBuffer[rxBytes] = 0;

            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, inBuffer);

                for (int i=0; i<rxBytes; i++) {
                    if (inBuffer[i] == '>') {
                        msgReading = true ;
                        msgIndex = 0 ; }
                    else {
                        if (msgReading) {
                            if (msgIndex > bufMax) msgReading = false ;
                            else if ( (inBuffer[i] == 10) || (inBuffer[i] == 13)) {
                                msg[msgIndex] = 0 ;
                                msgReading = false ;
                                msgCb(msg) ;           //callback
                            }
                            else msg[msgIndex ++] = (char)inBuffer[i] ;
                    }  } } } }
    free(inBuffer); }

void uartInit(uart_cb_t callback){
    msgCb = callback ;
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
}


#endif