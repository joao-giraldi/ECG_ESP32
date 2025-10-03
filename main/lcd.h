#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_panel_vendor.h"

#include "ports.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ    (400 * 1000)
#define EXAMPLE_PIN_NUM_SDA           ADS_SDA_PORT
#define EXAMPLE_PIN_NUM_SCL           ADS_SCL_PORT
#define EXAMPLE_PIN_NUM_RST           -1
#define EXAMPLE_I2C_HW_ADDR           0x3C

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              128
#define EXAMPLE_LCD_V_RES              64

// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

typedef enum {
    SYSTEM_IDLE,
    SYSTEM_COLLECTING,
    SYSTEM_STOPPED
} system_state_t;

void boot_image(void);
void example_lvgl_demo_ui(lv_disp_t *disp);
bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);

void lcd_config(void);
void lcd_display_welcome(void);
void lcd_display_collecting(uint32_t elapsed_time);
void lcd_display_stopped(uint32_t total_time, const char* filename);
void lcd_display_network(void);
void lcd_display_server(void);

#ifdef __cplusplus
}
#endif