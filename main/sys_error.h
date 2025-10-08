#pragma once

#include "lcd.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

void err_handler(const char* err_point);

#ifdef __cplusplus
}
#endif