#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ports.h"

esp_err_t lcd_config(void);

#ifdef __cplusplus
}
#endif