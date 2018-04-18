//ESP32 WIFI-MANAGER
//
//APRIL 17 2018
//ANKIT.BHATNAGARINDIA@GMAIL.COM

#include "ESP32_WIFIMANAGER.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_err.h"
#include <string.h>

//INTERNAL FUNCTIONS
static bool s_wifi_connected;
static uint8_t s_operating_mode;
static uint8_t s_operating_trigger;
static uint8_t s_operating_configuration;
static uint8_t s_operating_credential_src;

static void* s_operating_wifi_credentials;

//INTERNAL VARIABLES
static void s_esp32_wifimanager_core_task(void);
static esp_err_t s_esp32_wifimanager_wifi_evt_handler(void* ctx, system_event_t* evt);

void ESP32_WIFIMANAGER_Init(esp32_wifimanager_mode_t mode, 
                                esp32_wifimanager_trigger_t trigger, 
                                esp32_wifimanager_configuration_t configuration,
                                esp32_wifimanager_credential_src_t cred_src,
                                void* credentials)
{
    //ESP32 WIFIMANAGER INITIALIZE

    s_operating_mode = mode;
    s_operating_trigger = trigger;
    s_operating_configuration = configuration;
    s_operating_credential_src = cred_src;

    s_wifi_connected = false;
    s_operating_wifi_credentials = credentials;

    ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Initialized (%u, %u, %u, %u)\n", s_operating_mode,
                                                                    s_operating_trigger,
                                                                    s_operating_configuration,
                                                                    s_operating_credential_src);
}

void ESP32_WIFIMANAGER_Start(void)
{
    //START ESP32 WIFIMANAGER
    //CREATE TASK IF RTOS MODE ELSE SIMPLE EXECUTE CODE

    if(s_operating_mode == ESP32_WIFIMANAGER_MODE_RTOS)
    {
        //RTOS MODE
        ESP_LOGI(ESP32_WIFIMANAGER_TAG, "RTOS Mode\n");

        //CREATE A RTOS TASK AND SET TO RUNNING
    }
    else if(s_operating_mode == ESP32_WIFIMANAGER_MODE_NORTOS)
    {
        //NON RTOS MODE
        ESP_LOGI(ESP32_WIFIMANAGER_TAG, "NON RTOS Mode\n");

        //SIMPLE CALL THE CORE FUNCTION
        s_esp32_wifimanager_core_task();
    }
}

static void s_esp32_wifimanager_core_task(void)
{
    //ESP32 WIFIMANAGER CORE TASK
    
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    
    //INITIALIZE ESP32 WIFI STACK
    esp_wifi_init(&config);
    esp_wifi_set_mode(WIFI_MODE_STA);

    //REGISTER WIFI EVENT HANDLER
    esp_event_loop_init(s_esp32_wifimanager_wifi_evt_handler, NULL);

    //WIFI STATION CONFIG
    wifi_config_t station_config;

    switch(s_operating_credential_src)
    {
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_HARDCODED:
            strcpy((char * restrict)station_config.sta.ssid, 
                ((esp32_wifimanager_credential_hardcoded_t*)s_operating_wifi_credentials)->wifi_ssid);
            strcpy((char * restrict)station_config.sta.password,
                ((esp32_wifimanager_credential_hardcoded_t*)s_operating_wifi_credentials)->wifi_password);
            station_config.sta.bssid_set = false;
            break;
        
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_EEPROM:
            //NOT SUPPORTED FOR NOW
            return;
            break;
        
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_FLASH:
            //NOT SUPPORTED FOR NOW
            return;
            break;
        
        default:
            return;
            break;
    }

    //SET CONFIGURATION
    esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t*)&station_config);

    //SET WIFI AUTOCONNECT TO FALSE
    esp_wifi_set_auto_connect(false);

    //START WIFI
    esp_wifi_start();

    //CONNECT TO WIFI
    esp_wifi_connect();
}

static esp_err_t s_esp32_wifimanager_wifi_evt_handler(void* ctx, system_event_t* evt)
{
    //WIFI EVENT HANDLER

    switch(evt->event_id)
    {
        case SYSTEM_EVENT_WIFI_READY:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "EVT_WIFI_READY\n");
            break;
        
        case SYSTEM_EVENT_SCAN_DONE:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "EVT_SCAN_DONE\n");
            break;
        
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "EVT_STA_START\n");
            break;

        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "EVT_STA_CONNECTED\n");
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "SSID : %s\n", 
                                            (evt->event_info).connected.ssid);
            break;
        
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "EVT_STA_DISCONNECTED\n");
            break;
        
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "EVT_STA_GOT_IP\n");
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "IP : %u.%u.%u.%u\n",
                                        ((evt->event_info).got_ip.ip_info.ip.addr & 0xFF000000) >> 24,
                                        ((evt->event_info).got_ip.ip_info.ip.addr & 0x00FF0000) >> 16,
                                        ((evt->event_info).got_ip.ip_info.ip.addr & 0x0000FF00) >> 8,
                                        ((evt->event_info).got_ip.ip_info.ip.addr & 0x000000FF));
            s_wifi_connected = true;
            break;
        
        default:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "EVT_UNKNOWN\n");
            break;
    }
    return ESP_OK;
}