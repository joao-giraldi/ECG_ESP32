#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "ecg.h"
#include "microSD.h"
#include "ports.h"

#include "wifi.h"
#include "web_server.h"

#ifndef APP_CPU_NUM
#define APP_CPU_NUM     PRO_CPU_NUM
#endif

extern sdmmc_card_t* sd_card;

void app_main(void) {
    printf("=== ECG Monitor com ADS1115 ===\n");
    wifi_init_ap("ECG-ESP32", "12345678", 6, 4, false);
    start_webserver();  

    ESP_ERROR_CHECK(config_ports());
    ESP_ERROR_CHECK(ecg_config());

    sd_config();
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    xTaskCreatePinnedToCore(ecg_task, "ecg_task", 4096, NULL, 4, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(sd_task, "sd_task", 8192, NULL, 2, NULL, APP_CPU_NUM);

    // printf("Tasks criadas com sucesso!");
}