
extern void send_lighting_level_set(uint16_t adr, uint16_t level);
extern void send_gen_onoff_set(uint16_t adr, uint8_t state);

class cLamp {
    private:
        uint16_t adr ;
        uint8_t onOffState ;
        uint16_t lightLevel;
    public :
        cLamp(a) {
            adr = a ;
            onOffState = 0 ;
            lightLevel = 0 ;
        }
        void setOnOff(uint8_t state) {
            onOffState = state ;
            send_gen_onoff_set(adr, state) ;
        }
        void onOffStatus(uint8_t state){ onOffState = state ;}
        void setLightness(uint16_t level) {
            lightLevel = level ;
            send_lighting_level_set(adr, level);
        }
        void lightnessState(uint16_t level) {lightLevel = level ;}
        uint8_t getAddress() {return adr ;}
}

#define lampsMax 16

class cLamps {
    private :
        uint8_t nbrOfLamps ;

        lamp* theLamps[lampsMax];
    public :
        cLamp* getLamp(uint16_t adr) {
            for (int i=0, i<nbrOfLamps, i++) {
                if (theLamps[i]->getAddress() == adr) return theLamps[i] ;
            }
            uint8_t iLamp = newLamp(adr);
            if (iLamp != -1) return theLamps[iLamp] ; 
            return NULL ;
        }
        int8_t newLamp(uint16_t adr) {
            if (nbrOfLamps < lampsMax) {
                theLamps[nbrOfLamps++] = new cLamp(adr);
                return nbrOfLamps -1 ; }
            else return -1 ;
        }
}


/*
static void board_uart_task(void *p)
{
    uint8_t *data = calloc(1, UART_BUF_SIZE);
    uint32_t input;

    while (1) {
        int len = uart_read_bytes(MESH_UART_NUM, data, UART_BUF_SIZE, 100 / portTICK_PERIOD_MS);
        if (len > 0) {
            input = strtoul((const char *)data, NULL, 16);
            if (len) {
            data[len] = '\0';
            ESP_LOGI(TAG, "Recv str: %s", (char *) data);
            ESP_LOGI(TAG, "Recv value: %lu", input);
        }
//           remote_addr = input & 0xFFFF;
//            ESP_LOGI(TAG, "%s: input 0x%08x, remote_addr 0x%04x", __func__, input, remote_addr);
//            memset(data, 0, UART_BUF_SIZE);
        }
    }

    vTaskDelete(NULL);
}

static void board_uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(MESH_UART_NUM, &uart_config);
    uart_set_pin(MESH_UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(MESH_UART_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
}
*/
