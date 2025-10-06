#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "ports.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ECG_SAMPLE_RATE     100                         // 100 Hz sample rate
#define ECG_DELAY_MS        (1000.0/ECG_SAMPLE_RATE)

#define ECG_BUFFER_SIZE     512

esp_err_t ecg_config(void);
void ecg_task(void *pvParameters);
int16_t ecg_measure(void);
void finalize_ecg_collection(void);

#ifdef __cplusplus
}
#endif