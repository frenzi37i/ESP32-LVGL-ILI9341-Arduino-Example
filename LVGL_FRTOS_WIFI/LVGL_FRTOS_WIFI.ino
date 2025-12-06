/*
LVGL ESP32 IMPLEMENTATION EXAMPLE

Implementing the "button multiple events" example

Using: 
> LVGL  v. 9.4.0
> TFT_eSPI v. 2.5.43
> arduino ide 
> ILI9341 tft + touchscreen + ESP32 Mini1 

>>> COMPILE WITH ARDUINO IDE FOR WEMOS D1 MINI ESP32 <<<

LVGL internal spi manager is quite bad and not working. 
Is not possible to properly calibrate touch screen (already well implemented into TFT_eSPI)
It's messy to create a duplicated external TFT_eSPI only for the touchscreen driver. 
So, creating a single, external TFT_eSPI method and creating my own driver for touch and tft.

Hardware setup according to TFT_eSPI configuration (see below).
Touch screen SPI using the same pins of th tft (in parallel), except for the CS. 
No pullup / pulldown needed. 
_______________________________________________________________________________________________________-
Libraries configuration: 

---------------------------
|   TFT_eSPI v. 2.5.43 configuration
---------------------------
TFT_eSPI configuration found in: C:\Users\__USER_NAME__\Documents\Arduino\libraries\TFT_eSPI\User_Setup.h

Settings: 
#define ILI9341_DRIVER
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
//#define TFT_RST   4  // Reset pin (could connect to RST pin)
#define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST
#define TOUCH_CS 17     // Chip select pin (T_CS) of touch screen
#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

---------------------------
|   lvgl v.9.4.0 configuration
---------------------------
lvgl template configuration file can be found on C:\Users\__USER_NAME__\Documents\Arduino\libraries\lvgl\lv_conf_template.h
copy it to C:\Users\__USER_NAME__\Documents\Arduino\libraries\ and rename it lv_conf.h
enable it by setting #if 1 at the beginning of the file (line 15)
other modification to standard file: None
avoiding using lvgl internal tft_espi and ili9341 driver. 

Additionally, lvgl is doing strange things with touch screen coordinates when touch coordinates are 
passed with rotations != 0. The touch coordinates are perfectly managed already by the TFT_eSPI library, 
so is needed to comment or delete this section in lvgl/src/indev/lv_indev.c from line 693 to 703: 
  
  // if(disp->rotation == LV_DISPLAY_ROTATION_180 || disp->rotation == LV_DISPLAY_ROTATION_270) {
  //     data->point.x = disp->hor_res - data->point.x - 1;
  //     data->point.y = disp->ver_res - data->point.y - 1;
  // }
  // if(disp->rotation == LV_DISPLAY_ROTATION_90 || disp->rotation == LV_DISPLAY_ROTATION_270) {
  //     int32_t tmp = data->point.y;
  //     data->point.y = data->point.x;
  //     data->point.x = disp->ver_res - tmp - 1;
  // }

*/

#include "LVGL_implementation.h"
#include <WiFi.h>

// EventGroup bits
#define WIFI_CONNECTED_BIT   (1 << 0)
#define WIFI_FAIL_BIT        (1 << 1)

// Queue commands
typedef enum {
    WIFI_CMD_CONNECT,
    WIFI_CMD_DISCONNECT
} wifi_cmd_t;

char wifi_ip_str[32] = "0.0.0.0";
int wifi_rssi = 0;
SemaphoreHandle_t wifiDataMutex;

EventGroupHandle_t wifiEventGroup;
QueueHandle_t wifiCmdQueue;

// LVGL objects
lv_obj_t *label_status;
lv_obj_t *btn_connect;
lv_obj_t *btn_disconnect;
lv_obj_t *label_ip;
lv_obj_t *label_rssi;

// Callbacks from the buttons to add command to the queue
static void btn_connect_event_cb(lv_event_t * e) {
    wifi_cmd_t cmd = WIFI_CMD_CONNECT;
    xQueueSend(wifiCmdQueue, &cmd, 0);
}

static void btn_disconnect_event_cb(lv_event_t * e) {
    wifi_cmd_t cmd = WIFI_CMD_DISCONNECT;
    xQueueSend(wifiCmdQueue, &cmd, 0);
}


void guiTask(void *pvParameters) {

    // Label stato WiFi
    label_status = lv_label_create(lv_scr_act());
    lv_label_set_text(label_status, "WiFi: Idle");
    lv_obj_align(label_status, LV_ALIGN_TOP_MID, 0, 20);

    // Label IP
    label_ip = lv_label_create(lv_scr_act());
    lv_label_set_text(label_ip, "IP: ---");
    lv_obj_align(label_ip, LV_ALIGN_TOP_MID, 0, 50);

    // Label RSSI
    label_rssi = lv_label_create(lv_scr_act());
    lv_label_set_text(label_rssi, "Signal: --- dBm");
    lv_obj_align(label_rssi, LV_ALIGN_TOP_MID, 0, 80);

    // Connect button
    btn_connect = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn_connect, 120, 50);
    lv_obj_align(btn_connect, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_add_event_cb(btn_connect, btn_connect_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label1 = lv_label_create(btn_connect);
    lv_label_set_text(label1, "Connect");
    lv_obj_center(label1);

    // Disconnect button
    btn_disconnect = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn_disconnect, 120, 50);
    lv_obj_align(btn_disconnect, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_add_event_cb(btn_disconnect, btn_disconnect_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label2 = lv_label_create(btn_disconnect);
    lv_label_set_text(label2, "Disconnect");
    lv_obj_center(label2);

    while (1) {
        lv_timer_handler();

        EventBits_t bits = xEventGroupGetBits(wifiEventGroup);

        if (bits & WIFI_CONNECTED_BIT)
            lv_label_set_text(label_status, "WiFi: Connected");
        else if (bits & WIFI_FAIL_BIT)
            lv_label_set_text(label_status, "WiFi: Failed");
        else
            lv_label_set_text(label_status, "WiFi: Disconnected");

        // Aggiornamento IP + RSSI (protetti da mutex)
        if (xSemaphoreTake(wifiDataMutex, 5)) {
            // IP
            lv_label_set_text_fmt(label_ip, "IP: %s", wifi_ip_str);

            // RSSI
            lv_label_set_text_fmt(label_rssi, "Signal: %d dBm", wifi_rssi);

            xSemaphoreGive(wifiDataMutex);
        }

        //vTaskDelay(pdMS_TO_TICKS(200));
    }
}

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PWD";

void wifiTask(void *pvParameters) {
    wifi_cmd_t cmd;

    while (1) {
        if (xQueueReceive(wifiCmdQueue, &cmd, portMAX_DELAY)) {

            if (cmd == WIFI_CMD_CONNECT) {
                WiFi.mode(WIFI_STA);
                WiFi.disconnect(true);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                WiFi.begin(ssid, password);

                xEventGroupClearBits(wifiEventGroup,
                                     WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

                int retry = 0;
                const int max_retries = 15;

                while (WiFi.status() != WL_CONNECTED && retry < max_retries) {
                    retry++;
                    vTaskDelay(pdMS_TO_TICKS(500));
                }

                if (WiFi.status() == WL_CONNECTED) {
                    xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);

                    // Aggiorna dati IP + RSSI
                    if (xSemaphoreTake(wifiDataMutex, portMAX_DELAY)) {
                        snprintf(wifi_ip_str, sizeof(wifi_ip_str),
                                 "%s", WiFi.localIP().toString().c_str());
                        wifi_rssi = WiFi.RSSI();
                        xSemaphoreGive(wifiDataMutex);
                    }

                } else {
                    xEventGroupSetBits(wifiEventGroup, WIFI_FAIL_BIT);
                }
            }

            else if (cmd == WIFI_CMD_DISCONNECT) {
                WiFi.disconnect(true);

                xEventGroupClearBits(wifiEventGroup, WIFI_CONNECTED_BIT);

                // Reset dati
                if (xSemaphoreTake(wifiDataMutex, portMAX_DELAY)) {
                    strcpy(wifi_ip_str, "0.0.0.0");
                    wifi_rssi = 0;
                    xSemaphoreGive(wifiDataMutex);
                }
            }
        }
    }
}

void setup() {
  // Setup lvgl
  setup_lvgl(); 
  
  wifiEventGroup = xEventGroupCreate();
  wifiCmdQueue   = xQueueCreate(5, sizeof(wifi_cmd_t));
  wifiDataMutex  = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(guiTask,  "guiTask",  7000, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(wifiTask, "wifiTask", 5000, NULL, 1, NULL, 0);
}

void loop() {
  1;    
}
