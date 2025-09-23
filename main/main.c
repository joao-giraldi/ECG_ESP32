#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "ports.h"

#include "ecg.h"
#include "microSD.h"
#include "wifi.h"
#include "web_server.h"
#include "ssd1306.h"

#ifndef APP_CPU_NUM
#define APP_CPU_NUM     PRO_CPU_NUM
#endif

extern sdmmc_card_t* sd_card;

void app_main(void) {
    printf("=== ECG Monitor com ADS1115 ===\n");
    wifi_init_ap("ECG-ESP32", "12345678", 6, 4, false);
    start_webserver();  

    ESP_ERROR_CHECK(config_ports());
    //ESP_ERROR_CHECK(ecg_config());

    //sd_config();

    //ESP_ERROR_CHECK(lcd_config());

    init_ssd1306();
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    //xTaskCreatePinnedToCore(ecg_task, "ecg_task", 4096, NULL, 4, NULL, APP_CPU_NUM);
    //xTaskCreatePinnedToCore(sd_task, "sd_task", 8192, NULL, 2, NULL, APP_CPU_NUM);

    // printf("Tasks criadas com sucesso!");

    while(1)
    {   
        ssd1306_print_str(18, 0, "Hello World!", false);
        ssd1306_print_str(18, 17, "SSD1306 OLED", false);
        ssd1306_print_str(28, 27, "with ESP32", false);
        ssd1306_print_str(38, 37, "ESP-IDF", false);
        ssd1306_print_str(28, 47, "Embedded C", false);

        ssd1306_display();
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }

    while (1)
    {
        vTaskDelay(10);
    }
    
}