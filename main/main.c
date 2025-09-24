#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "ports.h"

#include "ecg.h"
#include "microSD.h"
#include "wifi.h"
#include "web_server.h"
#include "mdns_app.h"
#include "lcd.h"

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

void app_main(void)
{
    static uint32_t collection_start_time = 0;
    static uint32_t last_lcd_update = 0;

    printf("=== ECG Monitor com ADS1115 ===\n");
    wifi_init_ap("ECG-ESP32", "12345678", 6, 4, false);

    ESP_ERROR_CHECK(config_ports());
    // ESP_ERROR_CHECK(ecg_config());
    sd_config();
    
    //init_ssd1306();     // Inicia o painel LCD
    ESP_ERROR_CHECK(lcd_config());

    httpd_handle_t server = start_webserver();
    web_register_sd_api(server);
    mdns_app_start("ecg"); // -> http://ecg.local

    web_register_sd_api(server);

    // vTaskDelay(pdMS_TO_TICKS(1000));

    // xTaskCreatePinnedToCore(ecg_task, "ecg_task", 4096, NULL, 4, NULL, APP_CPU_NUM);
    // xTaskCreatePinnedToCore(sd_task, "sd_task", 8192, NULL, 2, NULL, APP_CPU_NUM);

    // printf("Tasks criadas com sucesso!");

    lcd_display_welcome();
    ESP_LOGI("OI", "Welcome foi");
    vTaskDelay(300);
    ESP_LOGI("OI", "Network");
    lcd_display_network_ready();
    
    while (1)
    {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;

        if(collection_start_time == 0) {
            collection_start_time = current_time;
            last_lcd_update = current_time;

            lcd_display_collecting(0);
        }

        if(current_time - last_lcd_update >= 1) {
            uint32_t elapsed = current_time - collection_start_time;
            lcd_display_collecting(elapsed);
            last_lcd_update = current_time;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
        //vTaskDelay(10);
    }
}