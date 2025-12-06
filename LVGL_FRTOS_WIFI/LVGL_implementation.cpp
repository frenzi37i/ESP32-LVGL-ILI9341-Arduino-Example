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
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "esp_heap_caps.h"

// SETTINGS 
#define LOG_LVGL_ENABLED 0
#define ROTATION 3 // can be 0,1,2,3

// LVGL rotation and touch calibration data according TFT_eSPI rotation
// TO gain calibration data, upload and run the Generic>Touch_calibrate example from TFT_eSPI library
#if ROTATION == 0
#define TFT_ROTATION LV_DISPLAY_ROTATION_0
uint16_t calData[5] = { 770, 2671, 419, 3059, 2 };
#endif
#if ROTATION == 1
#define TFT_ROTATION LV_DISPLAY_ROTATION_90
uint16_t calData[5] = { 394, 3129, 731, 2734, 1 };
#endif
#if ROTATION == 2
#define TFT_ROTATION LV_DISPLAY_ROTATION_180
uint16_t calData[5] = { 848, 2576, 440, 3026, 4 };
#endif
#if ROTATION == 3
#define TFT_ROTATION LV_DISPLAY_ROTATION_270
uint16_t calData[5] = { 355, 3237, 645, 2794, 7 };
#endif

// Set ILI9341 screen resolution without considering the rotation. 
// Must match the resolution considering rotation = 0
#define TFT_HOR_RES 240
#define TFT_VER_RES 320

//LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
//uint32_t draw_buf[DRAW_BUF_SIZE / 4];
uint32_t *draw_buf = nullptr;


TFT_eSPI tft = TFT_eSPI(); 


// LOG function on serial
// Remember to init Serial if willing to use this 
#if LOG_LVGL_ENABLED != 0
void my_print(lv_log_level_t level, const char *buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}
#endif


// DISPLAY DRIVER
// LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  //Copy `px map` to the `area`

    // TFT_eSPI implmentation
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();
  
  // Call it to tell LVGL you are ready
  lv_display_flush_ready(disp);
}


// TOUCHSCREEN DRIVER 
// Called periodically to poll touchscreen touch event
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  uint16_t x, y;
  bool pressed = tft.getTouch(&x, &y);
  if (!pressed) {
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  data->state = LV_INDEV_STATE_PRESSED;
  // Remember to comment lvgl/src/indev/lv_indev.c from line 693 to 703
  // to have proper touch screen coordinates 
  data->point.x = x;
  data->point.y = y;
}

// LVGL tick source (using millis())
static uint32_t my_tick(void) {
  return millis();
}

// MAIN LVGL SETUP
void setup_lvgl(void){
  // Setup the TFT_eSPI, preparing all the hardware
  // and init lvgl, assigning output and input devices 
  // Call this function in the setup before running lv_timer_handler() in the main loop

  // Init tft display and load touchscreen calibration data
  tft.init();
  tft.setRotation(ROTATION); 
  tft.setTouch(calData);

  // Init lvgl
  lv_init();

  // Set lvgl tick source
  lv_tick_set_cb(my_tick);

  // register print function for debugging 
#if LOG_LVGL_ENABLED != 0
  lv_log_register_print_cb(my_print);
#endif

  // heap allocation for draw buffer
  draw_buf = (uint32_t*) heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_8BIT);

  // Setup the display in LVGL
  lv_display_t *disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
  //lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf),LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_buffers(disp, draw_buf, NULL, DRAW_BUF_SIZE ,LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, my_disp_flush); // Callback to draw on screen 
  lv_display_set_rotation(disp, TFT_ROTATION);  // Set internal lvgl rotation
  lv_display_set_default(disp);
    
  // Setup the input device driver
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); // Touchpad should have POINTER type
  lv_indev_set_read_cb(indev, my_touchpad_read); // Callback for the touch read

}