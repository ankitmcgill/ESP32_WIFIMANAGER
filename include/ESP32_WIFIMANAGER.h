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


#define ESP32_WIFIMANAGER_TAG                       "ESP32:WIFIMANAGER"
#define ESP32_WIFIMANAGER_WIFI_RETRY_COUNT          (3)
#define ESP32_WIFIMANAGER_WIFI_CONNECT_CHECK_MS     (4000)

#define ESP32_WIFIMANAGER_STATUS_LED_TOGGLE_MS      (200)

#define ESP32_WIFIMANAGER_WEBCONFIG_PATH            "/config"
#define ESP32_WIFIMANAGER_SSID_LEN                  (32)
#define ESP32_WIFIMANAGER_SSID_PWD_LEN              (64)
#define ESP32_WIFIMANAGER_CUSTOM_FIELD_MAX_COUNT    (5)

typedef enum
{
    ESP32_WIFIMANAGER_STATE_INITIALIZE = 0,
    ESP32_WIFIMANAGER_STATE_CONNECTING,
    ESP32_WIFIMANAGER_STATE_CONNECTED,
    ESP32_WIFIMANAGER_STATE_DISCONNECTED,
    ESP32_WIFIMANAGER_STATE_CONNECTION_FAILED,
    ESP32_WIFIMANAGER_STATE_IDLE
}esp32_wifimanager_state_t;

typedef enum
{
    ESP32_WIFIMANAGER_CREDENTIAL_SRC_GPIO = 0,
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

//END CUSTOM VARIABLE STRUCTURES-------------------------------------------------

void ESP32_WIFIMANAGER_SetDebug(uint8_t debug);
void ESP32_WIFIMANAGER_SetParameters(esp32_wifimanager_credential_src_t input_mode,
                                        esp32_wifimanager_config_mode_t config_mode,
                                        void* user_data,
                                        uint8_t gpio_led_pin,
                                        char* project_name);
void ESP32_WIFIMANAGER_SetStatusLedType(esp32_wifimanager_status_led_type_t led_type);                                       
void ESP32_WIFIMANAGER_SetGpioTriggerLevel(esp32_wifimanager_gpio_trigger_type_t level);
void ESP32_WIFIMANAGER_SetUserCbFunction(void (*wifi_connected_cb)(char**, bool));

//OPERATION FUNCTIONS
void ESP32_WIFIMANAGER_Mainiter(void);

#endif