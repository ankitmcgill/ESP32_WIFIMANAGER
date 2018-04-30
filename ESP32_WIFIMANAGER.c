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
#include "esp_smartconfig.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_smartconfig.h"
#include "esp_err.h"
#include <string.h>

//INTERNAL VARIABLES
//DEBUG RELATED
static bool s_debug_on;

//OPERATION RELATED
static esp32_wifimanager_state_t s_state;
static volatile bool s_wifi_connected;
static volatile uint8_t s_wifi_attempt_count;

static wifi_config_t s_station_config;

//GPIO RELATED
static uint8_t s_esp32_wifimanager_gpio_led;
static volatile bool s_esp32_wifimanager_led_status;
static esp32_wifimanager_status_led_type_t s_esp32_wifimanager_led_idle_level;
static uint8_t s_esp32_wifimanager_gpio_trigger_pin;
static esp32_wifimanager_gpio_trigger_type_t s_esp32_wifimanager_gpio_trigger_type;

//CREDENTIAL SRC & CONFIG MODE RELATED
static esp32_wifimanager_credential_src_t s_esp32_wifimanager_credential_src;
static esp32_wifimanager_config_mode_t s_esp32_wifimanager_config_mode;

//SSID RELATED
static esp32_wifimanager_credential_hardcoded_t s_esp32_wifimanager_ssid_hardcoded_details;
static esp32_wifimanager_credential_external_storage_t s_esp32_wifimanager_eeprom_flash_details;

//CB FUNCTIONS
static void (*s_esp32_wifimanager_wifi_connected_user_cb)(char**, bool);

//INTERNAL FUNCTIONS
static void s_esp32_wifimanager_intialize(void);
static bool s_esp32_wifimanager_connecting(void);
static void s_esp32_wifimanager_connected(void);
static void s_esp32_wifimanager_disconnected(void);
static void s_esp32_wifimanager_connection_failed(void);

static void s_esp32_wifimanager_led_toggle_cb(void* pArg);
static void s_esp32_wifimanager_wifi_connect_check_cb(void* pArg);
static esp_err_t s_esp32_wifimanager_wifi_evt_handler(void* ctx, system_event_t* evt);
static void s_esp32_wifimanager_smartconfig_cb(smartconfig_status_t status, void *pdata);

void ESP32_WIFIMANAGER_SetDebug(uint8_t debug)
{
    //SET MODULE DEBUG FLAG
    
    s_debug_on = debug;

    ets_printf(ESP32_WIFIMANAGER_TAG" : Debug = %u\n", s_debug_on);
}

void ESP32_WIFIMANAGER_SetParameters(esp32_wifimanager_credential_src_t input_mode,
                                        esp32_wifimanager_config_mode_t config_mode,
                                        void* user_data,
                                        uint8_t gpio_led_pin,
                                        char* project_name)
{
    //ESP32 WIFIMANAGER SET PARAMETERS

    s_state = ESP32_WIFIMANAGER_STATE_INITIALIZE;

    s_esp32_wifimanager_credential_src = input_mode;
    s_esp32_wifimanager_config_mode = config_mode;
    s_esp32_wifimanager_gpio_led = gpio_led_pin;

    //INIT OPERATIONAL PARAMETERS
    s_wifi_attempt_count = 0;
    s_wifi_connected = false;

    switch(s_esp32_wifimanager_credential_src)
    { 
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_GPIO:
            ets_printf(ESP32_WIFIMANAGER_TAG" : Input SRC = GPIO\n");
            s_esp32_wifimanager_gpio_trigger_pin = *(uint8_t*)user_data;
            ets_printf(ESP32_WIFIMANAGER_TAG" : GPIO pin = %u\n", s_esp32_wifimanager_gpio_trigger_pin);
            //SET TRIGGER GPIO AS INPUT
            ESP32_GPIO_SetDirection(s_esp32_wifimanager_gpio_trigger_pin,
                                        GPIO_DIRECTION_INPUT);
            break;
        
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_HARDCODED:
            ets_printf(ESP32_WIFIMANAGER_TAG" : Input SRC = HARDCODED\n");
            s_esp32_wifimanager_ssid_hardcoded_details.ssid_name= 
                ((esp32_wifimanager_credential_hardcoded_t*)user_data)->ssid_name;
            s_esp32_wifimanager_ssid_hardcoded_details.ssid_pwd= 
                ((esp32_wifimanager_credential_hardcoded_t*)user_data)->ssid_pwd;
            break;
        
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_INTERNAL:
            ets_printf(ESP32_WIFIMANAGER_TAG" : Input SRC = INTERNAL\n");
            //DO NOTHING. WILL TAKE CARE OF IT LATER ON
            break;
        
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_FLASH:
            ets_printf(ESP32_WIFIMANAGER_TAG" : Input SRC = EXTERNAL FLASH\n");
            s_esp32_wifimanager_eeprom_flash_details.ssid_name_addr = 
                ((esp32_wifimanager_credential_external_storage_t*)user_data)->ssid_name_addr;
            s_esp32_wifimanager_eeprom_flash_details.ssid_pwd_addr = 
                ((esp32_wifimanager_credential_external_storage_t*)user_data)->ssid_pwd_addr;
            break;
        
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_EEPROM:
            ets_printf(ESP32_WIFIMANAGER_TAG" : Input SRC = EXTERNAL EEPROM\n");
            s_esp32_wifimanager_eeprom_flash_details.ssid_name_addr = 
                ((esp32_wifimanager_credential_external_storage_t*)user_data)->ssid_name_addr;
            s_esp32_wifimanager_eeprom_flash_details.ssid_pwd_addr = 
                ((esp32_wifimanager_credential_external_storage_t*)user_data)->ssid_pwd_addr;
            break;
        
        default:
            ets_printf(ESP32_WIFIMANAGER_TAG" : Input SRC = INVALID\n");
            break;
    }

    switch(s_esp32_wifimanager_config_mode)
    {
        case ESP32_WIFIMANAGER_CONFIG_SMARTCONFIG:
            ets_printf(ESP32_WIFIMANAGER_TAG" : Config Mode = SMARTCONFIG\n");
            break;
        
        case ESP32_WIFIMANAGER_CONFIG_WEBCONFIG:
            ets_printf(ESP32_WIFIMANAGER_TAG" : Config Mode = WEBCONFIG\n");
            break;
        
        case ESP32_WIFIMANAGER_CONFIG_BLE:
            ets_printf(ESP32_WIFIMANAGER_TAG" : Config Mode = BLE\n");
            break;
        
        default:
            ets_printf(ESP32_WIFIMANAGER_TAG" : Config Mode = INVALID\n");
            break;
    }
}

void ESP32_WIFIMANAGER_SetStatusLedType(esp32_wifimanager_status_led_type_t led_type)
{
    //SET STATUS LED TYPE

    s_esp32_wifimanager_led_idle_level = led_type;

    if(s_debug_on)
    {
        ets_printf(ESP32_WIFIMANAGER_TAG" : Status LED idle level = %u\n", led_type);
    }
}

void ESP32_WIFIMANAGER_SetGpioTriggerLevel(esp32_wifimanager_gpio_trigger_type_t level)
{
    //SET GPIO TRIGGER LEVEL TYPE

    s_esp32_wifimanager_gpio_trigger_type = level;
    if(s_debug_on)
    {
        ets_printf(ESP32_WIFIMANAGER_TAG" : GPIO trigger type = %u\n", level);
    }
}

void ESP32_WIFIMANAGER_SetUserCbFunction(void (*wifi_connected_cb)(char**, bool))
{
    //SET WIFI CONNECT CB FN

    if(wifi_connected_cb != NULL)
    {
        s_esp32_wifimanager_wifi_connected_user_cb = wifi_connected_cb;
    }
}


void ESP32_WIFIMANAGER_Mainiter(void)
{
    switch (s_state)
    {
        case  ESP32_WIFIMANAGER_STATE_INITIALIZE:
            ets_printf(ESP32_WIFIMANAGER_TAG" : ESP32_WIFIMANAGER_STATE_INITIALIZE\n");
            s_esp32_wifimanager_intialize();
            s_state = ESP32_WIFIMANAGER_STATE_CONNECTING;
            break;

        case ESP32_WIFIMANAGER_STATE_CONNECTING:
            ets_printf(ESP32_WIFIMANAGER_TAG" : ESP32_WIFIMANAGER_STATE_CONNECTING\n");
            if(!s_esp32_wifimanager_connecting())
            {
                s_state = ESP32_WIFIMANAGER_STATE_CONNECTION_FAILED;
                break;
            }
            s_state = ESP32_WIFIMANAGER_STATE_IDLE;
            break;

        case ESP32_WIFIMANAGER_STATE_CONNECTED:
            ets_printf(ESP32_WIFIMANAGER_TAG" : ESP32_WIFIMANAGER_STATE_CONNECTED\n");
            s_esp32_wifimanager_connected();
            s_state = ESP32_WIFIMANAGER_STATE_IDLE;
            break;

        case ESP32_WIFIMANAGER_STATE_DISCONNECTED:
            ets_printf(ESP32_WIFIMANAGER_TAG" : ESP32_WIFIMANAGER_STATE_DISCONNECTED\n");
            s_esp32_wifimanager_disconnected();
            s_state = ESP32_WIFIMANAGER_STATE_IDLE;
            break;

        case ESP32_WIFIMANAGER_STATE_CONNECTION_FAILED:
            ets_printf(ESP32_WIFIMANAGER_TAG" : ESP32_WIFIMANAGER_STATE_CONNECTION_FAILED\n");
            s_esp32_wifimanager_connection_failed();
            s_state = ESP32_WIFIMANAGER_STATE_IDLE;
            break;
        
        case ESP32_WIFIMANAGER_STATE_IDLE:
            //DO NOTHING
            break;
        
        default:
            break;
    }
}

static void s_esp32_wifimanager_intialize(void)
{
    //INTIALIZE ESP32 WIFIMANAGER MODULE

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
                            s_esp32_wifimanager_led_toggle_cb,
                            NULL);

    //SET UP WIFI CONNECT CHECK TIMER
    ESP32_TIMER_Initialize(TIMER_GROUP0,
                            TIMER1,
                            true,
                            TIMER_DIRECTION_UP,
                            true,
                            2);
    ESP32_TIMER_SetInterruptCb(TIMER_GROUP0,
                            TIMER1,
                            ESP32_TIMER_MS_TO_CNT_VALUE(ESP32_WIFIMANAGER_WIFI_CONNECT_CHECK_MS, 2),
                            s_esp32_wifimanager_wifi_connect_check_cb,
                            (void*)&s_station_config);
    
    //START LED FLASHING TIMER
    ESP32_TIMER_Start(TIMER_GROUP0, TIMER0);

    //START WIFI CONNECT CHECK TIMER
    ESP32_TIMER_Start(TIMER_GROUP0, TIMER1);

    //INITIALIZE ESP32 WIFI STACK
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&config);
    esp_wifi_set_mode(WIFI_MODE_STA);

    //REGISTER WIFI EVENT HANDLER
    esp_event_loop_init(s_esp32_wifimanager_wifi_evt_handler, NULL);
  
    switch(s_esp32_wifimanager_credential_src)
    {
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_GPIO:
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_INTERNAL:
            //USED INTERNALLY SAVED WIFI CREDENTIALS
            //SET AUTOCONNECT STORAGE TO FLASH
            esp_wifi_set_storage(WIFI_STORAGE_FLASH);
            //SET WIFI AUTOCONNECT TO TRUE
            esp_wifi_set_auto_connect(true);
            esp_wifi_get_config(WIFI_IF_STA, &s_station_config);
            break;
        
        case ESP32_WIFIMANAGER_CREDENTIAL_SRC_HARDCODED:
            //USE HARDCODED SUPPLIED WIFI CREDENTIALS
            //SET WIFI AUTOCONNECT TO FALSE
            esp_wifi_set_auto_connect(false);
            strcpy((char * restrict)s_station_config.sta.ssid, 
                s_esp32_wifimanager_ssid_hardcoded_details.ssid_name);
            strcpy((char * restrict)s_station_config.sta.password,
                s_esp32_wifimanager_ssid_hardcoded_details.ssid_pwd);
            s_station_config.sta.bssid_set = false;
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
    esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t*)&s_station_config);

    if(s_debug_on)
    {
        ets_printf(ESP32_WIFIMANAGER_TAG" : Initialized (%u, %u) led @ pin %u\n", s_esp32_wifimanager_credential_src,
                                                                                    s_esp32_wifimanager_config_mode,
                                                                                    s_esp32_wifimanager_gpio_led);
    }
}

static bool s_esp32_wifimanager_connecting(void)
{
    //CONNECT TO WIFI

    //CHECK FOR TRIGGER
    //IF TRIGGER = GPIO DO CHECK RIGHT NOW
    //IF TRIGGER = NOCONNECTION, LET IT PROCEED TO CONNECT TO WIFI
    if(s_esp32_wifimanager_credential_src == ESP32_WIFIMANAGER_CREDENTIAL_SRC_GPIO)
    {
        //CHECK IF GPIO ACTIVATED
        ets_printf(ESP32_WIFIMANAGER_TAG" : Checking trigger gpio level\n");
        uint8_t retval;
        if(ESP32_GPIO_GetValue(s_esp32_wifimanager_gpio_trigger_pin, &retval) == ESP_OK && 
            retval == s_esp32_wifimanager_gpio_trigger_type)
        {
            //START CONFIGURATION PROCESS
            //RETURN FALSE TO GO INTO FAILED STATE AND START CONFIGURATION
            //MANUALLY START WIFI AS WIFI IS NOT STARTED YET 
            //AND SMARTCONFIG NEEDS IT TO BE STARTED
            ets_printf(ESP32_WIFIMANAGER_TAG" : gpio triggered !!\n");
            esp_wifi_start();
            return false;
        }
    }

    //CHECK FOR CONNECTION ATTEMPTS
    if(s_wifi_attempt_count >= ESP32_WIFIMANAGER_WIFI_RETRY_COUNT)
    {
        //MAX ATTEMPT REACHED
        ets_printf(ESP32_WIFIMANAGER_TAG" : Max connection attempts reached !\n");
        return false;
    }

    ets_printf(ESP32_WIFIMANAGER_TAG" : Wifi connect attempt #%u\n", s_wifi_attempt_count);
    s_wifi_attempt_count++;

    //START WIFI
    esp_wifi_start();

    //CONNECT TO WIFI
    esp_wifi_connect();

    return true;
}

static void s_esp32_wifimanager_connected(void)
{
    //WIFI CONNECTION OK

    //STOP ALL TIMERS
    ESP32_TIMER_Stop(TIMER_GROUP0, TIMER0);
    ESP32_TIMER_Stop(TIMER_GROUP0, TIMER1);
    //TURN LED ON
    ESP32_GPIO_SetValue(s_esp32_wifimanager_gpio_led, true);
    //CALL USER CB
    if(s_esp32_wifimanager_wifi_connected_user_cb != NULL)
    {
        (*s_esp32_wifimanager_wifi_connected_user_cb)(NULL, true);
    }
}

static void s_esp32_wifimanager_disconnected(void)
{
    //WIFI DISCONNECTED

}

static void s_esp32_wifimanager_connection_failed(void)
{
    //WIFI CONNECTION FAILED

    //STOP ALL TIMERS
    ESP32_TIMER_Stop(TIMER_GROUP0, TIMER0);
    ESP32_TIMER_Stop(TIMER_GROUP0, TIMER1);
    
    //TURN LED OFF
    ESP32_GPIO_SetValue(s_esp32_wifimanager_gpio_led, false);
    
    //CALL USER CB
    if(s_esp32_wifimanager_wifi_connected_user_cb != NULL)
    {
        (*s_esp32_wifimanager_wifi_connected_user_cb)(NULL, false);
    }

    //START CONFIGURATION PROCES
    switch(s_esp32_wifimanager_config_mode)
    {
        case ESP32_WIFIMANAGER_CONFIG_SMARTCONFIG:
            //START SMARTCONFIG
            esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
            esp_smartconfig_start(s_esp32_wifimanager_smartconfig_cb);
            break;
        
        case ESP32_WIFIMANAGER_CONFIG_WEBCONFIG:
            ets_printf(ESP32_WIFIMANAGER_TAG" : Config Mode = WEBCONFIG\n");
            break;
        
        case ESP32_WIFIMANAGER_CONFIG_BLE:
            ets_printf(ESP32_WIFIMANAGER_TAG" : Config Mode = BLE\n");
            break;
        
        default:
            ets_printf(ESP32_WIFIMANAGER_TAG" : Config Mode = INVALID\n");
            break;
    }
}

static void s_esp32_wifimanager_led_toggle_cb(void* pArg)
{
    //LED TOGGLE TIMER CB FUNCTION

    ESP32_GPIO_SetDebug(false);
    s_esp32_wifimanager_led_status = !s_esp32_wifimanager_led_status;
    ESP32_GPIO_SetValue(s_esp32_wifimanager_gpio_led, s_esp32_wifimanager_led_status);
    ESP32_GPIO_SetDebug(true);
}

static void s_esp32_wifimanager_wifi_connect_check_cb(void* pArg)
{
    //WIFI CONNECTED CHECK CB

    if(s_wifi_connected)
    {
        //DO NOTHING
        //STATE MACHINE WILL TAKE CARE OF IT      
        return;
    }

    //WIFI NOT CONNECTED
    //SET STATE TO ESP32_WIFIMANAGER_STATE_CONNECTING
    s_state = ESP32_WIFIMANAGER_STATE_CONNECTING;
}

static esp_err_t s_esp32_wifimanager_wifi_evt_handler(void* ctx, system_event_t* evt)
{
    //WIFI EVENT HANDLER

    switch(evt->event_id)
    {
        case SYSTEM_EVENT_WIFI_READY:
            ets_printf(ESP32_WIFIMANAGER_TAG" : EVT_WIFI_READY\n");
            break;
        
        case SYSTEM_EVENT_SCAN_DONE:
            ets_printf(ESP32_WIFIMANAGER_TAG" : EVT_SCAN_DONE\n");
            break;
        
        case SYSTEM_EVENT_STA_START:
            ets_printf(ESP32_WIFIMANAGER_TAG" : EVT_STA_START\n");
            break;

        case SYSTEM_EVENT_STA_CONNECTED:
            ets_printf(ESP32_WIFIMANAGER_TAG" : EVT_STA_CONNECTED\n");
            ets_printf(ESP32_WIFIMANAGER_TAG" : SSID : %s\n", 
                                            (evt->event_info).connected.ssid);
            break;
        
        case SYSTEM_EVENT_STA_DISCONNECTED:
            //CAN HAPPEN IF THE ESP DISCONNECTS FROM AN ALREADY CONNECTED AP
            //OR IF IT WAS NOT ABLE TO CONNECT TO IT IN THE FIRST PLACE (ONLY IF
            //SEND THE WRONG PASSWORD TO EXITING SSID)
            //IT COULD HAPPEN THAT STA_DISCONNECT IS NEVER CALLED SO WE NEED
            //THE WIFI CONNECTED TIMER ALSO
            ets_printf(ESP32_WIFIMANAGER_TAG" : EVT_STA_DISCONNECTED\n");
            s_state = ESP32_WIFIMANAGER_STATE_DISCONNECTED;
            break;
        
        case SYSTEM_EVENT_STA_GOT_IP:
            ets_printf(ESP32_WIFIMANAGER_TAG" : EVT_STA_GOT_IP\n");
            ets_printf(ESP32_WIFIMANAGER_TAG" : IP : %u.%u.%u.%u\n",
                                        ((evt->event_info).got_ip.ip_info.ip.addr & 0x000000FF),
                                        ((evt->event_info).got_ip.ip_info.ip.addr & 0x0000FF00) >> 8,
                                        ((evt->event_info).got_ip.ip_info.ip.addr & 0x00FF0000) >> 16,
                                        ((evt->event_info).got_ip.ip_info.ip.addr & 0xFF000000) >> 24);
            s_state = ESP32_WIFIMANAGER_STATE_CONNECTED;
            break;
        
        default:
            ets_printf(ESP32_WIFIMANAGER_TAG" : EVT_UNKNOWN\n");
            break;
    }
    return ESP_OK;
}

static void s_esp32_wifimanager_smartconfig_cb(smartconfig_status_t status, void *pdata)
{
    //ESP32 SMARTCOFIG EVENT CB FUNCTION

    switch(status)
    {
        case SC_STATUS_WAIT:
            ets_printf(ESP32_WIFIMANAGER_TAG" : SMARTCONFIG: SC_STATUS_WAIT\n");
            break;
        
        case SC_STATUS_FIND_CHANNEL:
            ets_printf(ESP32_WIFIMANAGER_TAG" : SMARTCONFIG: SC_STATUS_FIND_CHANNEL\n");
            break;
        
        case SC_STATUS_GETTING_SSID_PSWD:
            ets_printf(ESP32_WIFIMANAGER_TAG" : SMARTCONFIG: SC_STATUS_GETTING_SSID_PSWD\n");
            break;

        case SC_STATUS_LINK:
            ets_printf(ESP32_WIFIMANAGER_TAG" : SMARTCONFIG: SC_STATUS_LINK\n");
            wifi_config_t *wifi_config = pdata;
            ets_printf(ESP32_WIFIMANAGER_TAG" : SMARTCONFIG: SSID = %s\n", wifi_config->sta.ssid);
            ets_printf(ESP32_WIFIMANAGER_TAG" : SMARTCONFIG: PASSWORD = %s\n", wifi_config->sta.password);
            esp_wifi_disconnect();
            esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config);
            esp_wifi_connect();
            break;

        case SC_STATUS_LINK_OVER:
            if(pdata != NULL)
            {
                uint8_t ip[4] = {0};
                memcpy(ip, (uint8_t* )pdata, 4);
                ets_printf(ESP32_WIFIMANAGER_TAG" : SMARTCONFIG: IP = %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
            }
            break;
    }
}