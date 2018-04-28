/**************************************************
* ESP32 WIFI-MANAGER
*
* SOFTAP CREDENTIALS:
*   SSID : ESP32
*   PASSWORD : 123456789
*
* SET THE WIFI RECONNECT INTERVAL TO ATLEAST
* 4000ms(4 s), SO AS TO GIVE ENOUGH TIME TO
* ESP32 TO CONNECT TO WIFI
*
*  INPUT_MODE        TRIGGER                   IF NOT ABLE TO CONNECT TO WIFI
*  ----------        --------------            -----------------------------------------------
*
*  GPIO              GPIO PIN HIGH/LOW         - START SSID FRAMEWORK EITHER IN SMARTCONFIG
*                                                OR IN TCP WEBSERVER
*                                              - WIFI CONNECT SUCCESSFULL
*                                              - AUTO CONNECT ON RESTART FROM INTERNAL SSID CACHE
*
*  HARDCODED         NOT ABLE TO CONNECT       - START SSID FRAMEWORK EITHER IN SMARTCONFIG
*                    TO HARDCODED SSID           OR IN TCP WEBSERVER
*                                              - WIFI CONNECT SUCCESSFULL
*                                              - ON RESTART CHECK FOR VALID SSID CACHE. IF FOUND
*                                                CONNECT USING THAT ELSE USE HARDCODED ONE
*
*  INTERNAL          NOT ABLE TO CONNECT       - START SSID FRAMEWORK EITHER IN SMARTCONFIG
*                    TO INTERNALLY CACHED        OR IN TCP WEBSERVER
*                    SSID                      - WIFI CONNECT SUCCESSFULL
*                                              - AUTO CONNECT ON RESTART FROM INTERNAL SSID CACHE
*
*  FLASH             NOT ABLE TO CONNECT       - START SSID FRAMEWORK EITHER IN SMARTCONFIG
*  EEPROM            TO SSID READ FROM           OR IN TCP WEBSERVER
*                    FLASH/EEPROM AT GIVEN     - WIFI CONNECT SUCCESSFULL
*                    ADDRESS                   - SAVE WIFI CREDENTIAL TO FLASH/EEPROM
*                                              - ON RESTART, AGAIN READ FLASH/EEPROM ADDRESS
*                                                AND CONNECT TO READ SSID
* REFERENCES
* -----------
*   (1) ONLINE HTML EDITOR
*       http://bestonlinehtmleditor.com/
*	(2) BOOTSTRAP HTML GUI EDITOR
*		http://http://pingendo.com/
*		https://diyprojects.io/bootstrap-create-beautiful-web-interface-projects-esp8266/#.WdMcBmt95hH
*
* APRIL 17 2018
* ANKIT.BHATNAGARINDIA@GMAIL.COM
**************************************************/

#ifndef _ESP32_WIFIMANAGER_
#define _ESP32_WIFIMANAGER_

#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"

/*
#define ESP32_WIFIMANAGER_LOG(format, ...) \
            if(s_debug_on) \
            { \   
                ESP_LOGI(ESP32_WIFIMANAGER_TAG, format, ##__VA_ARGS__);\
            }
*/

#define ESP32_SSID_HARDCODED
#define ESP32_CONFIG_SMARTCONFIG

#define ESP32_WIFIMANAGER_TAG                       "ESP32:WIFIMANAGER"
#define ESP32_WIFIMANAGER_WIFI_RETRY_COUNT          (3)

#define ESP32_WIFIMANAGER_STATUS_LED_TOGGLE_MS      (200)

#define ESP32_WIFIMANAGER_WEBCONFIG_PATH            "/config"
#define ESP32_WIFIMANAGER_SSID_LEN                  (32)
#define ESP32_WIFIMANAGER_SSID_PWD_LEN              (64)
#define ESP32_WIFIMANAGER_CUSTOM_FIELD_MAX_COUNT    (5)

#if defined(ESP32_SSID_FLASH)
    #error "ESP32:WIFIMANAGER - SSID_FLASH not supported"
#elif defined(ESP32_SSID_EEPROM)
    #error "ESP32:WIFIMANAGER - SSID_EEPROM not supported"
#elif defined(ESP32_CONFIG_SMARTCONFIG)
    //#include "ESP32_SMARTCONFIG.h"
#elif defined(ESP32_CONFIG_WEBCONFIG)
    #error "ESP32:WIFIMANAGER - CONFIG_WEBCONFIG not supported"
#elif defined(ESP32_CONFIG_BLE)
    #error "ESP32:WIFIMANAGER - CONFIG_BLE not supported"
#endif

typedef enum
{
    ESP32_WIFIMANAGER_MODE_RTOS = 0,
    ESP32_WIFIMANAGER_MODE_NORTOS
}esp32_wifimanager_mode_t;

typedef enum
{
    ESP32_WIFIMANAGER_CREDENTIAL_SRC_AUTO = 0,
    ESP32_WIFIMANAGER_CREDENTIAL_SRC_GPIO,
    ESP32_WIFIMANAGER_CREDENTIAL_SRC_HARDCODED,
    ESP32_WIFIMANAGER_CREDENTIAL_SRC_INTERNAL,
    ESP32_WIFIMANAGER_CREDENTIAL_SRC_FLASH,
    ESP32_WIFIMANAGER_CREDENTIAL_SRC_EEPROM
}esp32_wifimanager_credential_src_t;

typedef enum
{
    ESP32_WIFIMANAGER_GPIO_TRIGGER_LOW = 0,
    ESP32_WIFIMANAGER_GPIO_TRIGGER_HIGH
}esp32_wifimanager_gpio_trigger_type_t;

typedef enum
{
    ESP32_WIFIMANAGER_STATUS_LED_IDLE_LOW = 0,
    ESP32_WIFIMANAGER_STATUS_LED_IDLE_HIGH
}esp32_wifimanager_status_led_type_t;

typedef struct
{
    char* ssid_name;
    char* ssid_pwd;
}esp32_wifimanager_credential_hardcoded_t;

typedef struct
{
    uint32_t ssid_name_addr;
    uint32_t ssid_pwd_addr;
}esp32_wifimanager_credential_external_storage_t;

typedef enum
{
    ESP32_WIFIMANAGER_CONFIG_SMARTCONFIG = 0,
    ESP32_WIFIMANAGER_CONFIG_WEBCONFIG,
    ESP32_WIFIMANAGER_CONFIG_BLE
}esp32_wifimanager_config_mode_t;

typedef struct
{
    char* custom_field_name;
    char* custom_field_label;
}esp32_wifimanager_config_user_feild_t;

typedef struct
{
    esp32_wifimanager_config_user_feild_t* custom_fields;
    uint8_t custom_fields_count;
}esp32_wifimanager_config_user_feild_group_t;
//END CUSTOM VARIABLE STRUCTURES-------------------------------------------------

/*typedef enum
{
    ESP32_WIFIMANAGER_TRIGGER_GPIO = 0,
    ESP32_WIFIMANAGER_TRIGGER_NOCONNECTION
}esp32_wifimanager_trigger_t;

typedef enum
{
    ESP32_WIFIMANAGER_CONFIGURATION_SMARTCONFIG = 0,
    ESP32_WIFIMANAGER_CONFIGURATION_BLE,
}esp32_wifimanager_configuration_t;*/

/*typedef enum
{
    ESP32_WIFIMANAGER_CREDENTIAL_SRC_AUTO = 0,
    ESP32_WIFIMANAGER_CREDENTIAL_SRC_HARDCODED,
    ESP32_WIFIMANAGER_CREDENTIAL_SRC_EEPROM,
    ESP32_WIFIMANAGER_CREDENTIAL_SRC_FLASH
}esp32_wifimanager_credential_src_t;*/

/*typedef struct
{
    char* wifi_ssid;
    char* wifi_password;
}esp32_wifimanager_credential_hardcoded_t;

typedef struct
{
    uint32_t wifi_ssid_aaddress;
    uint32_t wifi_password_address;
}esp32_wifimanager_credential_eeprom_t;

typedef struct
{
    uint32_t wifi_ssid_aaddress;
    uint32_t wifi_password_address;
}esp32_wifimanager_credential_flash_t;*/


void ESP32_WIFIMANAGER_SetDebug(uint8_t debug);
void ESP32_WIFIMANAGER_SetParameters(esp32_wifimanager_mode_t mode,
                                        esp32_wifimanager_credential_src_t input_mode,
                                        esp32_wifimanager_config_mode_t config_mode,
                                        void* user_data,
                                        esp32_wifimanager_config_user_feild_group_t* user_field_data,
                                        uint8_t gpio_led_pin,
                                        char* project_name);
void ESP32_WIFIMANAGER_SetStatusLedType(esp32_wifimanager_status_led_type_t led_type);                                       
void ESP32_WIFIMANAGER_SetGpioTriggerLevel(esp32_wifimanager_gpio_trigger_type_t level);
void ESP32_WIFIMANAGER_SetCbFunctions(void (*wifi_connected_cb)(char**, bool));

//OPERATION FUNCTIONS
void ESP32_WIFIMANAGER_Start(void);

/*void ESP32_WIFIMANAGER_Init(esp32_wifimanager_mode_t mode, 
                                esp32_wifimanager_trigger_t trigger, 
                                esp32_wifimanager_configuration_t configuration,
                                esp32_wifimanager_credential_src_t cred_src,
                                void* credentials);

void ESP32_WIFIMANAGER_Start(void);*/

#endif