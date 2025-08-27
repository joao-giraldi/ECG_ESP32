#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "ads111x.h"

#include "esp_log.h"

#include "ports.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADS_GAIN            ADS111X_GAIN_2V048
#define ECG_SAMPLE_RATE     100           // 100 Hz sample rate
#define ECG_DELAY_MS        (1000.0/ECG_SAMPLE_RATE)

#define ECG_BUFFER_SIZE     512

esp_err_t ecg_config(void);
void ecg_task(void *pvParameters);
uint16_t ecg_measure(void);

// A DISCUTIR...
float raw_to_voltage(uint16_t raw);

#ifdef __cplusplus
}
#endif