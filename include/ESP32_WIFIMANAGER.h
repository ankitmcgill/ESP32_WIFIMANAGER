//ESP32 WIFI-MANAGER
//
//APRIL 17 2018
//ANKIT.BHATNAGARINDIA@GMAIL.COM

#ifndef _ESP32_WIFIMANAGER_
#define _ESP32_WIFIMANAGER_

#include <stdio.h>
#include "esp_log.h"
#include "sdkconfig.h"

#define ESP32_WIFIMANAGER_TAG               "ESP32:WIFIMANAGER"
#define ESP32_WIFIMANAGER_WIFI_RETRY_COUNT  (3)

typedef enum
{
    ESP32_WIFIMANAGER_MODE_RTOS = 0,
    ESP32_WIFIMANAGER_MODE_NORTOS
}esp32_wifimanager_mode_t;

typedef enum
{
    ESP32_WIFIMANAGER_TRIGGER_GPIO = 0,
    ESP32_WIFIMANAGER_TRIGGER_NOCONNECTION
}esp32_wifimanager_trigger_t;

typedef enum
{
    ESP32_WIFIMANAGER_CONFIGURATION_SMARTCONFIG = 0,
    ESP32_WIFIMANAGER_CONFIGURATION_BLE,
}esp32_wifimanager_configuration_t;

typedef enum
{
    ESP32_WIFIMANAGER_CREDENTIAL_SRC_HARDCODED = 0,
    ESP32_WIFIMANAGER_CREDENTIAL_SRC_EEPROM,
    ESP32_WIFIMANAGER_CREDENTIAL_SRC_FLASH
}esp32_wifimanager_credential_src_t;

typedef struct
{
    char* wifi_ssid;
    char* wifi_password;
}esp32_wifimanager_credential_hardcoded_t;

void ESP32_WIFIMANAGER_Init(esp32_wifimanager_mode_t mode, 
                                esp32_wifimanager_trigger_t trigger, 
                                esp32_wifimanager_configuration_t configuration,
                                esp32_wifimanager_credential_src_t cred_src,
                                void* credentials);

void ESP32_WIFIMANAGER_Start(void);

#endif