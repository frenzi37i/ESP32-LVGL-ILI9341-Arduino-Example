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
#include "GUI.h"

void setup() {
  // Setup lvgl and add a button to it
  setup_lvgl(); 
  lv_example_event_button();
}

void loop() {
  // Run lvgl (must be a non blocking loop)
  // Using delay to have the wanted refresh rate
  lv_timer_handler(); 
  delay(5);          
}
