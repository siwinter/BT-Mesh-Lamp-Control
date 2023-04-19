/* board.c - Board-specific hooks */

/*
 * SPDX-FileCopyrightText: 2017 Intel Corporation
 * SPDX-FileContributor: 2018-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "mesh_main.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/uart.h"

#include "iot_button.h"

#include "board.h"

#define TAG "BOARD"

#define BUTTON1_IO_NUM           0
#define BUTTON2_IO_NUM           2
#define BUTTON_ACTIVE_LEVEL     0

#define MESH_UART_NUM UART_NUM_1
#define MESH_UART    (&UART1)

#define UART_BUF_SIZE 128

#define UART_TX_PIN  1
#define UART_RX_PIN  3

//extern uint16_t remote_addr;
uint16_t remote_addr;

extern void example_ble_mesh_send_gen_onoff_set(void);
extern void example_ble_mesh_send_lighting_level_set(void);

struct _led_state led_state[3] = {
    { LED_OFF, LED_OFF, LED_R, "red"   },
    { LED_OFF, LED_OFF, LED_G, "green" },
    { LED_OFF, LED_OFF, LED_B, "blue"  },
};

void board_led_operation(uint8_t pin, uint8_t onoff)
{
    for (int i = 0; i < ARRAY_SIZE(led_state); i++) {
        if (led_state[i].pin != pin) {
            continue;
        }
        if (onoff == led_state[i].previous) {
            ESP_LOGW(TAG, "led %s is already %s",
                led_state[i].name, (onoff ? "on" : "off"));
            return;
        }
        gpio_set_level(pin, onoff);
        led_state[i].previous = onoff;
        return;
    }
    ESP_LOGE(TAG, "LED is not found!");
}

static void board_led_init(void)
{
    for (int i = 0; i < ARRAY_SIZE(led_state); i++) {
        gpio_reset_pin(led_state[i].pin);
        gpio_set_direction(led_state[i].pin, GPIO_MODE_OUTPUT);
        gpio_set_level(led_state[i].pin, LED_OFF);
        led_state[i].previous = LED_OFF;
    }
}

static void button1_tap_cb(void* arg)
{
    ESP_LOGI(TAG, "btn1 cb (%s)", (char *)arg);

    example_ble_mesh_send_gen_onoff_set();
}

static void button2_tap_cb(void* arg)
{
    ESP_LOGI(TAG, "btn2 cb (%s)", (char *)arg);

    example_ble_mesh_send_lighting_level_set();
}

static void board_button_init(void)
{
    button_handle_t btn1_handle = iot_button_create(BUTTON1_IO_NUM, BUTTON_ACTIVE_LEVEL);
    if (btn1_handle) {
        iot_button_set_evt_cb(btn1_handle, BUTTON_CB_RELEASE, button1_tap_cb, "RELEASE");
    }
    button_handle_t btn2_handle = iot_button_create(BUTTON2_IO_NUM, BUTTON_ACTIVE_LEVEL);
    if (btn2_handle) {
        iot_button_set_evt_cb(btn2_handle, BUTTON_CB_RELEASE, button2_tap_cb, "RELEASE");
    }
}

void board_init(void)
{
    printf("start onoff_client");
    board_led_init();
    board_button_init();
//    board_uart_init();
//    xTaskCreate(board_uart_task, "board_uart_task", 2048, NULL, 5, NULL);
}
