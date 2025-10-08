#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"

#include "ports.h"
#include "sys_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ECG_SAMPLE_RATE     100                          // 150Hz -> 100Hz
#define ECG_DELAY_MS        (1000.0/ECG_SAMPLE_RATE)
#define ECG_TIMER_PERIOD_US (1000000/ECG_SAMPLE_RATE)   // Timer period in microseconds

#define ECG_BUFFER_SIZE     1024

// Variáveis externas para sincronização
extern SemaphoreHandle_t ecg_sample_semaphore;

esp_err_t ecg_config(void);
void ecg_task(void *pvParameters);
int16_t ecg_measure(void);
void finalize_ecg_collection(void);

#ifdef __cplusplus
}
#endif