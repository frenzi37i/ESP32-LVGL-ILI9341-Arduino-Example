# LVGL + ESP32 + TFT_eSPI (ILI9341)
### Minimal Working Example — Custom Display & Touch Drivers

This project demonstrates a clean and stable setup for running **LVGL 9.4.0** on an **ESP32 Mini1** with an **ILI9341 TFT display + resistive touchscreen**, using **TFT_eSPI** as the **only** SPI manager.

LVGL's built-in ILI9341/TFT_eSPI and touch drivers are intentionally not used due to:
- Unreliable SPI handling  
- Impossible / incorrect touchscreen calibration  
- Rotation conflicts  

A custom display driver and input driver are implemented instead.

**The minimal implementation is in /LVGL_example.**

**An implementation with WiFi and FreeRtos is also provided in /LVGL_FRTOS_WIFI (needed to setup your wifi SSID and Password)**

## 📌 Environment

| Component | Version / Notes |
|----------|------------------|
| **Board** | Wemos D1 Mini ESP32 |
| **Display** | ILI9341 + resistive touch |
| **Libraries** | LVGL 9.4.0, TFT_eSPI 2.5.43 |
| **IDE** | Arduino IDE |
| **Compilation Target** | *Wemos D1 Mini ESP32* |

## 📡 Hardware Wiring

The TFT and touch controller share the SPI bus (MISO / MOSI / SCLK).  
Only **CS pins differ**.

```
ESP32 Mini1   →   TFT / Touch
-----------------------------------
19  (MISO)    →   MISO
23  (MOSI)    →   MOSI
18  (SCLK)    →   SCLK

15  (TFT_CS)  →   TFT CS
2   (TFT_DC)  →   TFT DC
RST (shared)  →   TFT_RST (-1)

17  (TOUCH_CS) → Touch CS
```

➡️ No pull-ups or pull-downs required.

## ⚙️ TFT_eSPI Configuration

Edit:

```
Documents/Arduino/libraries/TFT_eSPI/User_Setup.h
```

Use these settings:

```c
#define ILI9341_DRIVER

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18

#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1   // uses ESP32 reset

#define TOUCH_CS 17

#define SPI_FREQUENCY       27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
```

## ⚙️ LVGL Configuration

1. Copy lvgl template configuration file from:
```
C:/Users/USER_NAME/Documents/Arduino/libraries/lvgl/lv_conf_template.h
```
to: (renaming it) 
```
C:/Users/USER_NAME/Documents/Arduino/libraries/lv_conf.h
```
   
2. Enable with:
   ```c
   #if 1
   ```

No additional changes are required **except disabling LVGL’s incorrect touch rotation logic**.

## 🛠 Required Patch: Disable LVGL Touch Rotation

TFT_eSPI already provides correct coordinates with rotation applied.  
LVGL applies **additional transformations**, causing misaligned touch input.  
These must be removed.

Edit:

```
C:/Users/USER_NAME/Documents/Arduino/libraries/lvgl/src/indev/lv_indev.c
```

Comment out the block around lines **693–703**:

```c
// if(disp->rotation == LV_DISPLAY_ROTATION_180 || disp->rotation == LV_DISPLAY_ROTATION_270) {
//     data->point.x = disp->hor_res - data->point.x - 1;
//     data->point.y = disp->ver_res - data->point.y - 1;
// }
// if(disp->rotation == LV_DISPLAY_ROTATION_90 || disp->rotation == LV_DISPLAY_ROTATION_270) {
//     int32_t tmp = data->point.y;
//     data->point.y = data->point.x;
//     data->point.x = disp->ver_res - tmp - 1;
// }
```

After this fix:
- Touchscreen calibration works  
- Coordinates match LVGL rotation  
- No flipping / mirroring issues  

## ✅ Summary

This project provides:

- Stable LVGL rendering on ESP32  
- Accurate, rotation-correct touchscreen input  
- A single, clean TFT_eSPI instance for both display and touch  
- No dependency on LVGL’s broken internal drivers  
