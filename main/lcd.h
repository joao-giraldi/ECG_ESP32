#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"

#include "ports.h"
#include "ssd1306.h"

extern i2c_ssd1306_handle_t i2c_ssd1306;

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYSTEM_IDLE,
    SYSTEM_COLLECTING,
    SYSTEM_STOPPED
} system_state_t;

esp_err_t lcd_config(void);
void lcd_display_welcome(void);
void lcd_display_collecting(uint32_t elapsed_time);
void lcd_display_stopped(uint32_t total_time);
void lcd_display_network_ready(void);

#ifdef __cplusplus
}
#endif