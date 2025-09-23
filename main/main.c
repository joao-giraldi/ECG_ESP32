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
#include "ssd1306.h"

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

extern i2c_ssd1306_handle_t i2c_ssd1306;

void app_main(void)
{
    printf("=== ECG Monitor com ADS1115 ===\n");
    wifi_init_ap("ECG-ESP32", "12345678", 6, 4, false);

    ESP_ERROR_CHECK(config_ports());
    // ESP_ERROR_CHECK(ecg_config());
    // sd_config();
    
    init_ssd1306();     // Inicia o painel LCD

    httpd_handle_t server = start_webserver();
    web_register_sd_api(server);
    mdns_app_start("ecg"); // -> http://ecg.local

    web_register_sd_api(server);

    // vTaskDelay(pdMS_TO_TICKS(1000));

    // xTaskCreatePinnedToCore(ecg_task, "ecg_task", 4096, NULL, 4, NULL, APP_CPU_NUM);
    // xTaskCreatePinnedToCore(sd_task, "sd_task", 8192, NULL, 2, NULL, APP_CPU_NUM);

    // printf("Tasks criadas com sucesso!");

    while(1)
    {   
        i2c_ssd1306_buffer_clear(&i2c_ssd1306);
    
        ssd1306_print_str(18, 0, "Hello World!", false);
        ssd1306_print_str(18, 17, "SSD1306 OLED", false);
        ssd1306_print_str(28, 27, "with ESP32", false);
        ssd1306_print_str(38, 37, "ESP-IDF", false);
        ssd1306_print_str(28, 47, "Embedded C", false);

        ssd1306_display();
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        
        // Limpar buffer antes da próxima tela
        i2c_ssd1306_buffer_clear(&i2c_ssd1306);
        
        ssd1306_print_str(18, 0, "Hello World!", false);
        ssd1306_print_str(18, 17, "SSD1306 OLED", false);
        ssd1306_print_str(28, 27, "with ESP32", false);
        ssd1306_print_str(38, 37, "ESP-IDF", false);
        ssd1306_print_str(28, 47, "MUITO FODA", false);
        
        ssd1306_display();
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }

    while (1)
    {
        vTaskDelay(10);
    }
}