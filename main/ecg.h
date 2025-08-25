#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "ads111x.h"

#include "ports.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADS_GAIN            ADS111X_GAIN_2V048
#define ECG_SAMPLE_RATE     100           // 500 Hz sample rate
#define ECG_DELAY_MS        (1000.0/ECG_SAMPLE_RATE)

esp_err_t ecg_config(void);
void ecg_task(void *pvParameters);
void ecg_measure(void);

#ifdef __cplusplus
}
#endif