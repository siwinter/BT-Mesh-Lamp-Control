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

#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_lighting_model_api.h"

#include "board.h"
#include "uart.h"
#include "ble_mesh_example_init.h"
#include "ble_mesh_example_nvs.h"

#define TAG "EXAMPLE"

#define CID_ESP 0x02E5

static uint8_t dev_uuid[16] = { 0xdd, 0xdd };

static struct example_info_store {
    uint16_t net_idx;   /* NetKey Index */
    uint16_t app_idx;   /* AppKey Index */
    uint8_t  onoff;     /* Remote OnOff */
    uint16_t  level;    // s.w. /* Remote Level */
    uint8_t  tid;       /* Message TID */ //s.w. ggf braucht man für jedes Model je einen tid
} __attribute__((packed)) store = {
    .net_idx = ESP_BLE_MESH_KEY_UNUSED,
    .app_idx = ESP_BLE_MESH_KEY_UNUSED,
    .onoff = LED_OFF,
    .level = 1,           //s.w.
    .tid = 0x0,
};

static nvs_handle_t NVS_HANDLE;
static const char * NVS_KEY = "onoff_client";

static esp_ble_mesh_client_t onoff_client;  // umfangreiche Struktur für den ClientData
static esp_ble_mesh_client_t level_client; //s.w.
static esp_ble_mesh_client_t light_client; //s.w.



static esp_ble_mesh_cfg_srv_t config_server = {
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
#if defined(CONFIG_BLE_MESH_FRIEND)
    .friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BLE_MESH_GATT_PROXY_SERVER)
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
#else
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
    .default_ttl = 7,
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_cli_pub, 2 + 1, ROLE_NODE);  // Struktur onfoff_cli_pub für pub-Msg 2+1 =Meldungslänge
ESP_BLE_MESH_MODEL_PUB_DEFINE(level_cli_pub, 2 + 1, ROLE_NODE);  // s.w. 2+1 ode 3+1 ??
ESP_BLE_MESH_MODEL_PUB_DEFINE(light_cli_pub, 2 + 1, ROLE_NODE);  // s.w. 2+1 ode 3+1 ??

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(&onoff_cli_pub, &onoff_client), //ModelID, OP-Code, Keys, ->ClientData, ->PubMsgStruktur
    ESP_BLE_MESH_MODEL_GEN_LEVEL_CLI(&level_cli_pub, &level_client),  //s.w.
    ESP_BLE_MESH_MODEL_LIGHT_LIGHTNESS_CLI(&light_cli_pub, &light_client),  //s.w.
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, ESP_BLE_MESH_MODEL_NONE),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .elements = elements,
    .element_count = ARRAY_SIZE(elements),
};

/* Disable OOB security for SILabs Android app */
static esp_ble_mesh_prov_t provision = {
    .uuid = dev_uuid,
#if 0
    .output_size = 4,
    .output_actions = ESP_BLE_MESH_DISPLAY_NUMBER,
    .input_actions = ESP_BLE_MESH_PUSH,
    .input_size = 4,
#else
    .output_size = 0,
    .output_actions = 0,
#endif
};

static void mesh_example_info_store(void)
{
    ble_mesh_nvs_store(NVS_HANDLE, NVS_KEY, &store, sizeof(store));
}

static void mesh_example_info_restore(void)
{
    esp_err_t err = ESP_OK;
    bool exist = false;

    err = ble_mesh_nvs_restore(NVS_HANDLE, NVS_KEY, &store, sizeof(store), &exist);
    if (err != ESP_OK) {
        return;
    }

    if (exist) {
        ESP_LOGI(TAG, "Restore, net_idx 0x%04x, app_idx 0x%04x, level xy, onoff %u, tid 0x%02x",
            store.net_idx, store.app_idx, store.onoff, store.tid);
    }
}

static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
    ESP_LOGI(TAG, "net_idx: 0x%04x, addr: 0x%04x", net_idx, addr);
    ESP_LOGI(TAG, "flags: 0x%02x, iv_index: 0x%08" PRIx32, flags, iv_index);
    board_led_operation(LED_G, LED_OFF);
    store.net_idx = net_idx;
    /* mesh_example_info_store() shall not be invoked here, because if the device
     * is restarted and goes into a provisioned state, then the following events
     * will come:
     * 1st: ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT
     * 2nd: ESP_BLE_MESH_PROV_REGISTER_COMP_EVT
     * So the store.net_idx will be updated here, and if we store the mesh example
     * info here, the wrong app_idx (initialized with 0xFFFF) will be stored in nvs
     * just before restoring it.
     */
}

static void example_ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                             esp_ble_mesh_prov_cb_param_t *param)
{
    switch (event) {
    case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
        mesh_example_info_restore(); /* Restore proper mesh example info */
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
        prov_complete(param->node_prov_complete.net_idx, param->node_prov_complete.addr,
            param->node_prov_complete.flags, param->node_prov_complete.iv_index);
        break;
    case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
        break;
    case ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT, err_code %d", param->node_set_unprov_dev_name_comp.err_code);
        break;
    default:
        ESP_LOGI(TAG, "unknown provisioning event, err_code");
        break;
    }
}

void send_msg(uint16_t* m)
{
    printf ("send_msg %i, %i, %i", m[0],m[1],m[2]) ;
    esp_ble_mesh_client_common_param_t common = {0};
    esp_err_t err = ESP_OK;

    common.opcode = m[1];
    
    common.ctx.net_idx = store.net_idx;
    common.ctx.app_idx = store.app_idx;

    common.ctx.addr = m[0];   /* s.w. to my lamp */
    common.ctx.send_ttl = 3;
    common.ctx.send_rel = false;
    common.msg_timeout = 0;     /* 0 indicates that timeout value from menuconfig will be used */
    common.msg_role = ROLE_NODE;

    switch (m[1]) {
        case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET:
            {
                common.model = onoff_client.model;
                esp_ble_mesh_generic_client_get_state_t set = {0};
                err = esp_ble_mesh_generic_client_get_state(&common, &set);
                if (err)  ESP_LOGE(TAG, "Send Generic OnOff Set Unack failed");
                break; }
        case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET:
        case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK:
            {
                common.model = onoff_client.model;
                esp_ble_mesh_generic_client_set_state_t set = {0};
                set.onoff_set.op_en = false;
                set.onoff_set.onoff = 1;
                if (m[2] == 0) set.onoff_set.onoff = 0;
                set.onoff_set.tid = store.tid++;

                err = esp_ble_mesh_generic_client_set_state(&common, &set);
                if (err)  ESP_LOGE(TAG, "Send Generic OnOff Set Unack failed");
                break; }
        case ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_GET:
            {
                common.model = light_client.model;
                esp_ble_mesh_light_client_get_state_t set = {0};
                err = esp_ble_mesh_light_client_get_state(&common, &set);
                if (err)  ESP_LOGE(TAG, "Send Generic OnOff Set Unack failed");
                break; }
        case ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET:
        case ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET_UNACK:
            {
                common.model = light_client.model;
                esp_ble_mesh_light_client_set_state_t set = {0};

                set.lightness_set.op_en = false;
                set.lightness_set.lightness = m[2];
                set.lightness_set.tid = store.tid++;
                
                err = esp_ble_mesh_light_client_set_state(&common, &set);
                if (err)  ESP_LOGE(TAG, "Send Light Lighting Set failed");
                break ; }
        case ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_GET:
            {
                common.model = light_client.model;
                esp_ble_mesh_light_client_get_state_t set = {0};
                
                err = esp_ble_mesh_light_client_get_state(&common, &set);
                if (err)  ESP_LOGE(TAG, "Send Light Lighting Range Get failed");
                break ; }

        default: break ;
    }
}

void send_lighting_level_set(uint16_t adr, uint16_t level) {
    uint16_t codes [3] ;
    codes[0] = adr ;
    codes[1] = ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET;
    codes[2] = level ;
    send_msg(codes) ; }

void send_gen_onoff_set(uint16_t adr, bool state) {
    uint16_t codes [3] ;
    codes[0] = adr ;
    codes[1] = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET;
//    mscodesg[1] = ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_SET_UNACK;
    codes[2] = 0;
    if (state) codes[2] = 1 ;
    send_msg(codes) ; }


void example_ble_mesh_send_gen_onoff_set(void) {
    send_gen_onoff_set(0xFFFF, store.onoff);
    store.onoff = !store.onoff;
    mesh_example_info_store(); /* Store proper mesh example info */
}



void example_ble_mesh_send_lighting_level_set(void) {
    send_lighting_level_set(0xFFFF, store.level);
    store.level = store.level + 1;
    if( store.level >= 51) store.level = 1;
    ESP_LOGI(TAG, "store.level ist %i", store.level);
    mesh_example_info_store(); /* Store proper mesh example info */
}

uint16_t msgCodes[4] ;
int8_t nbrOfCodes = 0 ;

static void light_client_cb(esp_ble_mesh_light_client_cb_event_t event,
                                               esp_ble_mesh_light_client_cb_param_t *param)
{
    ESP_LOGI(TAG, "Light client, event %u, error code %d, opcode is 0x%04" PRIx32, event, param->error_code, param->params->opcode);

    msgCodes[0] = param->params->ctx.addr ;

    switch (param->params->ctx.recv_op) {
    case ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_STATUS:
        msgCodes[1] = ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_STATUS ;
        msgCodes[2] = param->status_cb.lightness_status.present_lightness ;
        nbrOfCodes = 3 ;
        break ;
    case ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_STATUS:
        msgCodes[1] = ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_STATUS ;
        msgCodes[2] = param->status_cb.lightness_range_status.range_min ;
        msgCodes[3] = param->status_cb.lightness_range_status.range_max ;
        nbrOfCodes = 4 ;
        break ;

    }
    if (nbrOfCodes > 0) evtMsg(nbrOfCodes, msgCodes) ;

    switch (event) {
    case ESP_BLE_MESH_LIGHT_CLIENT_GET_STATE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_LIGHT_CLIENT_GET_STATE_EVT");
        break;
    case ESP_BLE_MESH_LIGHT_CLIENT_SET_STATE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_LIGHT_CLIENT_SET_STATE_EVT");
        break;
    case ESP_BLE_MESH_LIGHT_CLIENT_PUBLISH_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_LIGHT_CLIENT_PUBLISH_EVT");
        break;
    case ESP_BLE_MESH_LIGHT_CLIENT_TIMEOUT_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_LIGHT_CLIENT_TIMEOUT_EVT");
        break;
    default:
        break;
    }
}

static void generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event,
                                               esp_ble_mesh_generic_client_cb_param_t *param)
{
    ESP_LOGI(TAG, "Generic client, event %u, error code %d, opcode is 0x%04" PRIx32, event, param->error_code, param->params->opcode);

    msgCodes[0] = param->params->ctx.addr ;
    uint8_t nbrOfCodes = 0 ;

    switch (param->params->ctx.recv_op) {
    case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS:
        msgCodes[1] = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS ;
        msgCodes[2] = param->status_cb.onoff_status.present_onoff ;
        nbrOfCodes = 3 ;
        break ;
    case ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_STATUS:
        msgCodes[1] = ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_STATUS ;
        msgCodes[2] = param->status_cb.level_status.present_level ;
        nbrOfCodes = 3 ;
        break ;
    default:
        nbrOfCodes = 0 ;
        break ;
    }
    if (nbrOfCodes > 0) evtMsg(nbrOfCodes, msgCodes) ;

    switch (event) {
    case ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT");
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT");
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT");
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_TIMEOUT_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_TIMEOUT_EVT");
        break;
    default:
        break;
    }
}

static void example_ble_mesh_config_server_cb(esp_ble_mesh_cfg_server_cb_event_t event,
                                              esp_ble_mesh_cfg_server_cb_param_t *param)
{
    if (event == ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT) {
        switch (param->ctx.recv_op) {
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
            if (param->value.state_change.mod_app_bind.company_id == 0xFFFF &&
                param->value.state_change.mod_app_bind.model_id == ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_CLI) {
                store.app_idx = param->value.state_change.mod_app_bind.app_idx;
                mesh_example_info_store(); /* Store proper mesh example info */
            }
            break;
        default:
            break;
        }
    }
}

static esp_err_t ble_mesh_init(void)
{
    esp_err_t err = ESP_OK;

    esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);
    esp_ble_mesh_register_generic_client_callback(generic_client_cb);
    esp_ble_mesh_register_light_client_callback(light_client_cb);
    esp_ble_mesh_register_config_server_callback(example_ble_mesh_config_server_cb);

    err = esp_ble_mesh_init(&provision, &composition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize mesh stack (err %d)", err);
        return err;
    }

    err = esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable mesh node (err %d)", err);
        return err;
    }

    ESP_LOGI(TAG, "BLE Mesh Node initialized");

    board_led_operation(LED_G, LED_ON);

    return err;
}

void app_main(void)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing...");

    board_init();

    uartInit();

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

    /* Open nvs namespace for storing/restoring mesh example info */
    err = ble_mesh_nvs_open(&NVS_HANDLE);
    if (err) {
        return;
    }

    ble_mesh_get_dev_uuid(dev_uuid);

    /* Initialize the Bluetooth Mesh Subsystem */
    err = ble_mesh_init();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
    }
}
