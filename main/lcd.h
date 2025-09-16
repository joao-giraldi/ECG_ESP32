#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/lock.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "i2cdev.h"
#include "esp_lcd_sh1107.h"

#include "ports.h"

esp_err_t lcd_config(void);

#ifdef __cplusplus
}
#endif