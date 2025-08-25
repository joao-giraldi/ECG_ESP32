#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "ecg.h"
#include "ports.h"

#ifndef APP_CPU_NUM
#define APP_CPU_NUM     PRO_CPU_NUM
#endif

// Variáveis globais
i2c_dev_t device;
float gain_val;

void app_main(void) {
    printf("=== ECG Monitor com ADS1115 ===\n");
    
    config_ports();
    ESP_ERROR_CHECK(ecg_config());

    vTaskDelay(pdMS_TO_TICKS(1000));

    xTaskCreatePinnedToCore(ecg_task, "ecg_task", 4096, NULL, 3, NULL, APP_CPU_NUM);

    printf("Tasks criadas com sucesso!");
}