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

//#include "board.h"
//#include "uart.h"
#include "ble_mesh_example_init.h"
#include "ble_mesh_example_nvs.h"

#include "ble_mesh.h"
#define TAG "EXAMPLE"

#define CID_ESP 0x02E5

ble_model_evt_cb_t onEvent ;
void register_evt_cb(ble_model_evt_cb_t callback) {
    onEvent = callback ; }

static uint8_t dev_uuid[16] = { 0xdd, 0xdd };

static struct example_info_store {
    uint16_t net_idx;   /* NetKey Index */
    uint16_t app_idx;   /* AppKey Index */
    uint8_t  tid;       /* Message TID */ //s.w. ggf braucht man für jedes Model je einen tid
} __attribute__((packed)) store = {
    .net_idx = ESP_BLE_MESH_KEY_UNUSED,
    .app_idx = ESP_BLE_MESH_KEY_UNUSED,
    .tid = 0x4f,
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
    .output_size = 0,
    .output_actions = 0,
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
        ESP_LOGI(TAG, "Restore, net_idx 0x%04x, app_idx 0x%04x, level xy, tid 0x%02x",
            store.net_idx, store.app_idx, store.tid);
    }
}

static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
    ESP_LOGI(TAG, "net_idx: 0x%04x, addr: 0x%04x", net_idx, addr);
    ESP_LOGI(TAG, "flags: 0x%02x, iv_index: 0x%08" PRIx32, flags, iv_index);
//    board_led_operation(LED_G, LED_OFF);
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
//--------------------------- SET / GET COMMANDs ------------------------------------------------
void completeParam(esp_ble_mesh_client_common_param_t * common) {
    common->ctx.net_idx = store.net_idx;
    common->ctx.app_idx = store.app_idx;

    common->ctx.send_ttl = 3;
    common->ctx.send_rel = false;
    common->msg_timeout = 0;     /* 0 indicates that timeout value from menuconfig will be used */
    common->msg_role = ROLE_NODE;
}
void get_gen_onoff(uint16_t adr) {
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_generic_client_get_state_t get = {0};

    common.opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET ;
    common.ctx.addr = adr ;
    common.model = onoff_client.model;
    completeParam(&common) ;
    if (esp_ble_mesh_generic_client_get_state(&common, &get))  ESP_LOGE(TAG, "Get Generic OnOff failed"); }

void get_gen_level(uint16_t adr)  {
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_generic_client_get_state_t get = {0};

    common.opcode = ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_GET ;
    common.ctx.addr = adr ;
    common.model = level_client.model;
    completeParam(&common) ;
    if( esp_ble_mesh_generic_client_get_state(&common, &get)) ESP_LOGE(TAG, "Get Generic level failed"); }


void set_gen_onoff(uint16_t adr, bool state) {
    esp_ble_mesh_client_common_param_t common = {0};

    common.opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET ;
//    common.opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK ;
    common.ctx.addr = adr ;
    common.model = onoff_client.model;;
    completeParam(&common) ;

    esp_ble_mesh_generic_client_set_state_t set = {0};
    set.onoff_set.op_en = false;
    if (state) set.onoff_set.onoff = 1; else set.onoff_set.onoff = 0;
    set.onoff_set.tid = store.tid++;

    if( esp_ble_mesh_generic_client_set_state(&common, &set)) ESP_LOGE(TAG, "Set Generic OnOff failed"); }

void set_gen_level(uint16_t adr, uint16_t level) {
    esp_ble_mesh_client_common_param_t common = {0};

    common.opcode = ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_SET ;
//    common.opcode = ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_SET_UNACK ;
    common.ctx.addr = adr ;
    common.model = level_client.model;
    completeParam(&common) ;
    esp_ble_mesh_generic_client_set_state_t set = {0};
    set.level_set.op_en = false;
    set.level_set.level = level;
    set.level_set.tid = store.tid++;
                
    if ( esp_ble_mesh_generic_client_set_state(&common, &set) ) ESP_LOGE(TAG, "set_gen_level failed"); }

void get_light_status(uint16_t adr, uint16_t opcode) {
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_light_client_get_state_t get = {0};

    common.opcode = opcode ;
    common.ctx.addr = adr ;
    common.model = light_client.model;
    completeParam(&common) ;

    if ( esp_ble_mesh_light_client_get_state(&common, &get)) ESP_LOGE(TAG, "Get Light Status failed"); }

void get_light_lightness(uint16_t adr) {
    get_light_status(adr, ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_GET) ; }

void get_light_range(uint16_t adr) {
    get_light_status(adr, ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_GET) ; }

void set_light_lightness(uint16_t adr, uint16_t level) {
    esp_ble_mesh_client_common_param_t common = {0};

    common.opcode = ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET ;
//    common.opcode = ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET_UNACK ;
    common.ctx.addr = adr ;
    common.model = light_client.model;
    completeParam(&common) ;
    esp_ble_mesh_light_client_set_state_t set = {0};
    set.lightness_set.op_en = false;
    set.lightness_set.lightness = level;
    set.lightness_set.tid = store.tid++;
                
    if (esp_ble_mesh_light_client_set_state(&common, &set)) ESP_LOGE(TAG, "Set Light Lighting failed"); }

static void light_client_cb(esp_ble_mesh_light_client_cb_event_t event,
                                               esp_ble_mesh_light_client_cb_param_t *param)
{
    ESP_LOGI(TAG, "Light client, event %u, error code %d, opcode is 0x%04" PRIx32, event, param->error_code, param->params->opcode);

    client_status_cb_t p;
    p.light = &param->status_cb ;     
    onEvent(&p, param->params) ; }

static void generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event,
                                               esp_ble_mesh_generic_client_cb_param_t *param) {
    ESP_LOGI(TAG, "Generic client, event %u, error code %d, rec opcode is 0x%04" PRIx32,
     event, param->error_code, /*param->params->opcode, */param->params->ctx.recv_op);
     
    client_status_cb_t p;
    p.generic = &param->status_cb ;     
    onEvent(&p, param->params) ;}


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

esp_err_t ble_mesh_init(void)
{
    esp_err_t err = ESP_OK;

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init(); }

    ESP_ERROR_CHECK(err);

    err = bluetooth_init();
    if (err) {
        ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
        return err; }

    /* Open nvs namespace for storing/restoring mesh example info */
    err = ble_mesh_nvs_open(&NVS_HANDLE);
    if (err) {
        return err; }

    ble_mesh_get_dev_uuid(dev_uuid);

    esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);
    esp_ble_mesh_register_generic_client_callback(generic_client_cb);
    esp_ble_mesh_register_light_client_callback(light_client_cb);
    esp_ble_mesh_register_config_server_callback(example_ble_mesh_config_server_cb);

    err = esp_ble_mesh_init(&provision, &composition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize mesh stack (err %d)", err);
        return err; }

    err = esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable mesh node (err %d)", err);
        return err; }

    ESP_LOGI(TAG, "BLE Mesh Node initialized");

    return err;
}