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


#include "ESP32_WIFIMANAGER.h"
#include "ESP32_GPIO.h"
#include "ESP32_TIMER.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_err.h"
#include <string.h>

//INTERNAL VARIABLES
//DEBUG RELATED
static bool s_debug_on;

//OPERATION RELATED
static esp32_wifimanager_mode_t s_operating_mode;
static volatile bool s_wifi_connected;
static volatile uint8_t s_wifi_attempt_count;

static esp32_wifimanager_credential_src_t s_esp32_wifimanager_credential_src;
static esp32_wifimanager_config_mode_t s_esp32_wifimanager_config_mode;

//GPIO RELATED
static uint8_t s_esp32_wifimanager_gpio_led;
static volatile bool s_esp32_wifimanager_led_status;
static esp32_wifimanager_status_led_type_t s_esp32_wifimanager_led_idle_level;
static uint8_t s_esp32_wifimanager_gpio_trigger_pin;
static esp32_wifimanager_gpio_trigger_type_t s_esp32_wifimanager_gpio_trigger_type;

//TIMER RELATED

//WEBCONFIG HTML DATA RELATED
static char* s_esp32_wifimanagger_project_name;

//SSID RELATED
static esp32_wifimanager_credential_hardcoded_t s_esp32_wifimanager_ssid_hardcoded_details;
static esp32_wifimanager_credential_external_storage_t s_esp32_wifimanager_eeprom_flash_details;

//INTERNAL FUNCTIONS
static bool s_esp32_wifimanager_check_valid_stationconfig(void);

//CB FUNCTIONS
static void (*s_esp32_wifimanager_wifi_connected_user_cb)(char**, bool);

//INTERNAL FUNCTIONS
static void s_esp32_wifimanager_core_task(void);
static void s_esp32_wifimanager_led_toggle_cb(void* pArg);
//static void s_esp32_wifimanager_wifi_connect_timer_cb(void* pArg);
static void s_esp32_wifimanager_wifi_event_handler_cb(system_event_t* event);
//static void s_esp32_wifimanager_wifi_start_ssid_configuration(void);

//static void s_esp32_wifimanager_wifi_start_connection_process(struct station_config* sconfig);
//static void s_esp32_wifimanager_wifi_start_softap(void);
//static void s_esp32_wifimanager_tcp_server_path_config_cb(void);
//static void s_esp32_wifimanager_tcp_server_post_data_cb(char* data, uint16_t len, uint8_t post_flag);
static bool s_esp32_wifimanager_connect(void);
static void s_esp32_wifimanager_start_smartconfig(void);

static esp_err_t s_esp32_wifimanager_wifi_evt_handler(void* ctx, system_event_t* evt);

void ESP32_WIFIMANAGER_SetDebug(uint8_t debug)
{
    //SET MODULE DEBUG FLAG
    
    s_debug_on = debug;

    ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Debug = %u", s_debug_on);
}

void ESP32_WIFIMANAGER_SetParameters(esp32_wifimanager_mode_t mode,
                                        esp32_wifimanager_credential_src_t input_mode,
                                        esp32_wifimanager_config_mode_t config_mode,
                                        void* user_data,
                                        esp32_wifimanager_config_user_feild_group_t* user_field_data,
                                        uint8_t gpio_led_pin,
                                        char* project_name)
{
    //ESP32 WIFIMANAGER SET PARAMETERS

    s_operating_mode = mode;
    s_esp32_wifimanager_credential_src = input_mode;
    s_esp32_wifimanager_config_mode = config_mode;

    s_esp32_wifimanager_gpio_led = gpio_led_pin;

    s_wifi_attempt_count = 0;
    s_wifi_connected = false;

    switch(s_esp32_wifimanager_credential_src)
    {
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_GPIO:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Input SRC = GPIO");
            s_esp32_wifimanager_gpio_trigger_pin = *(uint8_t*)user_data;
            //SET TRIGGER GPIO AS INPUT
            ESP32_GPIO_SetDirection(s_esp32_wifimanager_gpio_trigger_pin,
                                        GPIO_DIRECTION_INPUT);
            break;
        
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_HARDCODED:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Input SRC = HARDCODED");
            s_esp32_wifimanager_ssid_hardcoded_details.ssid_name= 
                ((esp32_wifimanager_credential_hardcoded_t*)user_data)->ssid_name;
            s_esp32_wifimanager_ssid_hardcoded_details.ssid_pwd= 
                ((esp32_wifimanager_credential_hardcoded_t*)user_data)->ssid_pwd;
            break;
        
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_INTERNAL:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Input SRC = INTERNAL");
            break;
        
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_FLASH:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Input SRC = EXTERNAL FLASH");
            s_esp32_wifimanager_eeprom_flash_details.ssid_name_addr = 
                ((esp32_wifimanager_credential_external_storage_t*)user_data)->ssid_name_addr;
            s_esp32_wifimanager_eeprom_flash_details.ssid_pwd_addr = 
                ((esp32_wifimanager_credential_external_storage_t*)user_data)->ssid_pwd_addr;
            break;
        
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_EEPROM:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Input SRC = EXTERNAL EEPROM");
            s_esp32_wifimanager_eeprom_flash_details.ssid_name_addr = 
                ((esp32_wifimanager_credential_external_storage_t*)user_data)->ssid_name_addr;
            s_esp32_wifimanager_eeprom_flash_details.ssid_pwd_addr = 
                ((esp32_wifimanager_credential_external_storage_t*)user_data)->ssid_pwd_addr;
            break;
        
        default:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Input SRC = INVALID");
            break;
    }

    switch(s_esp32_wifimanager_config_mode)
    {
        case ESP32_WIFIMANAGER_CONFIG_SMARTCONFIG:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Config Mode = SMARTCONFIG");
            break;
        
        case ESP32_WIFIMANAGER_CONFIG_WEBCONFIG:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Config Mode = WEBCONFIG");
            break;
        
        case ESP32_WIFIMANAGER_CONFIG_BLE:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Config Mode = BLE");
            break;
        
        default:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Config Mode = INVALID");
            break;
    }

    //SET LED GPIO AS OUTPUT
    ESP32_GPIO_SetDirection(s_esp32_wifimanager_gpio_led, GPIO_DIRECTION_OUTPUT);

    //SET UP LED FLASHING TIMER
    s_esp32_wifimanager_led_status = true;
    ESP32_TIMER_SetDebug(true);
    ESP32_TIMER_Initialize(TIMER_GROUP0,
                            TIMER0,
                            true,
                            TIMER_DIRECTION_UP,
                            true,
                            2);
    ESP32_TIMER_SetInterruptCb(TIMER_GROUP0,
                            TIMER0,
                            ESP32_TIMER_MS_TO_CNT_VALUE(ESP32_WIFIMANAGER_STATUS_LED_TOGGLE_MS, 2),
                            s_esp32_wifimanager_led_toggle_cb);

    if(s_debug_on)
    {
        ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Initialized (%u, %u, %u)", s_operating_mode,
                                                                s_esp32_wifimanager_credential_src,
                                                                s_esp32_wifimanager_config_mode);
    }
}

void ESP32_WIFIMANAGER_SetStatusLedType(esp32_wifimanager_status_led_type_t led_type)
{
    //SET STATUS LED TYPE

    s_esp32_wifimanager_led_idle_level = led_type;

    if(s_debug_on)
    {
        ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Status LED idle level = %u", led_type);
    }
}

void ESP32_WIFIMANAGER_SetGpioTriggerLevel(esp32_wifimanager_gpio_trigger_type_t level)
{
    //SET GPIO TRIGGER LEVEL TYPE

    s_esp32_wifimanager_gpio_trigger_type = level;
    if(s_debug_on)
    {
        ESP_LOGI(ESP32_WIFIMANAGER_TAG, "GPIO trigger type = %u", level);
    }
}

void ESP32_WIFIMANAGER_SetCbFunctions(void (*wifi_connected_cb)(char**, bool))
{
    //SET WIFI CONNECT CB FN

    if(wifi_connected_cb != NULL)
    {
        s_esp32_wifimanager_wifi_connected_user_cb = wifi_connected_cb;
    }
}

void ESP32_WIFIMANAGER_Start(void)
{
    //START ESP32 WIFIMANAGER
    //CREATE TASK IF RTOS MODE ELSE SIMPLE EXECUTE CODE

    if(s_operating_mode == ESP32_WIFIMANAGER_MODE_RTOS)
    {
        //RTOS MODE
        if(s_debug_on)
        {
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "RTOS Mode");
        }

        //CREATE A RTOS TASK AND SET TO RUNNING
    }
    else if(s_operating_mode == ESP32_WIFIMANAGER_MODE_NORTOS)
    {
        //NON RTOS MODE
        if(s_debug_on)
        {
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "NON RTOS Mode");
        }

        //SIMPLE CALL THE CORE FUNCTION
        s_esp32_wifimanager_core_task();
    }
}

static void s_esp32_wifimanager_core_task(void)
{
    //ESP32 WIFIMANAGER CORE TASK

    if(s_debug_on)
    {
        ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Started");
    }

    //START LED FLASHING TIMER
    ESP32_TIMER_Start(TIMER_GROUP0, TIMER0);

    //CHECK FOR TRIGGER
    //IF TRIGGER = GPIO DO CHECK RIGHT NOW
    //IF TRIGGER = NOCONNECTION, LET IT PROCEED TO CONNECT TO WIFI
    if(s_esp32_wifimanager_credential_src == ESP32_WIFIMANAGER_CREDENTIAL_SRC_GPIO)
    {
        //CHECK IF GPIO ACTIVATED

        //START SMARTCONFIG
        s_esp32_wifimanager_start_smartconfig();
    }
    
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    
    //INITIALIZE ESP32 WIFI STACK
    esp_wifi_init(&config);
    esp_wifi_set_mode(WIFI_MODE_STA);

    //REGISTER WIFI EVENT HANDLER
    esp_event_loop_init(s_esp32_wifimanager_wifi_evt_handler, NULL);

    //WIFI STATION CONFIG
    wifi_config_t station_config;

    switch(s_esp32_wifimanager_credential_src)
    {
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_AUTO:
            //USED INTERNALLY SAVED WIFI CREDENTIALS
            //SET AUTOCONNECT STORAGE TO FLASH
            esp_wifi_set_storage(WIFI_STORAGE_FLASH);
            //SET WIFI AUTOCONNECT TO TRUE
            esp_wifi_set_auto_connect(true);
            break;

        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_GPIO:
            break;
        
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_HARDCODED:
            //USE HARDCODED SUPPLIED WIFI CREDENTIALS
            //SET WIFI AUTOCONNECT TO FALSE
            esp_wifi_set_auto_connect(false);
            strcpy((char * restrict)station_config.sta.ssid, 
                s_esp32_wifimanager_ssid_hardcoded_details.ssid_name);
            strcpy((char * restrict)station_config.sta.password,
                s_esp32_wifimanager_ssid_hardcoded_details.ssid_pwd);
            station_config.sta.bssid_set = false;
            break;
        
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_INTERNAL:
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

    //START WIFI
    esp_wifi_start();

    //CONNECT TO WIFI
    s_esp32_wifimanager_connect();
}

static void s_esp32_wifimanager_led_toggle_cb(void* pArg)
{
    //LED TOGGLE TIMER CB FUNCTION
    //THIS IS A CB FUNCTION FROM TIMER ISR
    //TURN OFF ALL DEBUG PRINTFS !!!

    ESP32_GPIO_SetDebug(false);
    s_esp32_wifimanager_led_status = !s_esp32_wifimanager_led_status;
    ESP32_GPIO_SetValue(s_esp32_wifimanager_gpio_led, s_esp32_wifimanager_led_status);
    ESP32_GPIO_SetDebug(true);
}

static bool s_esp32_wifimanager_connect(void)
{
    //CONNECT TO WIFI NETWORK

    if(s_wifi_attempt_count >= ESP32_WIFIMANAGER_WIFI_RETRY_COUNT)
    {
        ESP_LOGE(ESP32_WIFIMANAGER_TAG, "Wifi connect max retries done !");
        return false;
    }

    //CONNECT TO WIFI
    ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Wifi connect retry %u", s_wifi_attempt_count);
    esp_wifi_connect();
    return true;
}

static void s_esp32_wifimanager_start_smartconfig(void)
{
    //START SMARTCONFIG FOR WIFI CONFIGURATION
}

static esp_err_t s_esp32_wifimanager_wifi_evt_handler(void* ctx, system_event_t* evt)
{
    //WIFI EVENT HANDLER

    switch(evt->event_id)
    {
        case SYSTEM_EVENT_WIFI_READY:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "EVT_WIFI_READY");
            break;
        
        case SYSTEM_EVENT_SCAN_DONE:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "EVT_SCAN_DONE");
            break;
        
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "EVT_STA_START");
            break;

        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "EVT_STA_CONNECTED");
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "SSID : %s", 
                                            (evt->event_info).connected.ssid);
            break;
        
        case SYSTEM_EVENT_STA_DISCONNECTED:
            //CAN HAPPEN IF THE ESP DISCONNECTS FROM AN ALREADY CONNECTED AP
            //OR IF IT WAS NOT ABLE TO CONNECT TO IT IN THE FIRST PLACE
            //(BECAUSE OF WRONG CREDENTIALS)
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "EVT_STA_DISCONNECTED");
            if(s_wifi_connected)
            {
                //ESP WAS CONNECTED BEFORE
                //RESTART CONNECTION CYCLE FROM START
                ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Disconnected from a connected AP");
                s_wifi_connected = false;
                //RESET ATTEMPT COUNT
                s_wifi_attempt_count = 0;
                s_esp32_wifimanager_connect();
            }
            else
            {
                //ESP WAS NOT ABLE TO CONNECT
                ESP_LOGI(ESP32_WIFIMANAGER_TAG, "Not able to connect to AP");
                s_wifi_connected = false;
                s_wifi_attempt_count++;
                if(!s_esp32_wifimanager_connect())
                {
                    //MAX WIFI ATTEMPT EXPIRED
                    //CALL USERCB WITH FALSE
                    //TURN OFF LED TIMER
                    ESP32_TIMER_Stop(TIMER_GROUP0, TIMER0);
                    //SET LED TO OFF
                    ESP32_GPIO_SetValue(s_esp32_wifimanager_gpio_led, false);
                    //CALL USER CB FUNCTION IF NOT NULL
                    (*s_esp32_wifimanager_wifi_connected_user_cb)(NULL, false);
                }
            }
            break;
        
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "EVT_STA_GOT_IP");
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "IP : %u.%u.%u.%u",
                                        ((evt->event_info).got_ip.ip_info.ip.addr & 0x000000FF),
                                        ((evt->event_info).got_ip.ip_info.ip.addr & 0x0000FF00) >> 8,
                                        ((evt->event_info).got_ip.ip_info.ip.addr & 0x00FF0000) >> 16,
                                        ((evt->event_info).got_ip.ip_info.ip.addr & 0xFF000000) >> 24);
            s_wifi_connected = true;
            //TURN OFF LED TIMER
            ESP32_TIMER_Stop(TIMER_GROUP0, TIMER0);
            //SET LED TO SOLID
            ESP32_GPIO_SetValue(s_esp32_wifimanager_gpio_led, true);
            //CALL USER CB FUNCTION IF NOT NULL
            (*s_esp32_wifimanager_wifi_connected_user_cb)(NULL, true);
            break;
        
        default:
            ESP_LOGI(ESP32_WIFIMANAGER_TAG, "EVT_UNKNOWN");
            break;
    }
    return ESP_OK;
}

static bool s_esp32_wifimanager_check_valid_stationconfig(void)
{
    //CHECK IF PROVIDED STATION CONFIG IS VALID
    /*
    if(config->ssid[0] < 48 || 
        config->ssid[0] > 126 || 
        config->password[0] < 48 || 
        config->password[0] > 126)
    {
        return false;
    }*/
    return true;
}