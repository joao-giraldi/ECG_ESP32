#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include "driver/i2c.h"
#include "esp_log.h"

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

QueueHandle_t ecg_buffer_queue = NULL;

void app_main(void)
{
    static uint32_t collection_start_time = 0;
    static uint32_t last_lcd_update = 0;

    printf("=== ECG Monitor com ADS1115 ===\n");
    wifi_init_ap("ECG-ESP32", "12345678", 6, 4, false);
    
    ESP_ERROR_CHECK(config_ports());
    
    // Criar a fila ECG logo no início para evitar problemas de sincronização
    if (ecg_buffer_queue == NULL) {
        ecg_buffer_queue = xQueueCreate(3, sizeof(int16_t) * ECG_BUFFER_SIZE);
        if (ecg_buffer_queue == NULL) {
            ESP_LOGE("MAIN", "Falha ao criar fila ECG");
        } else {
            ESP_LOGI("MAIN", "Fila ECG criada com sucesso");
        }
    }
    
    sd_config();
    
    // Inicializar I2C uma única vez para todos os dispositivos
    ESP_LOGI("MAIN", "Inicializando barramento I2C...");
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = ADS_SDA_PORT,  // GPIO_NUM_21
        .scl_io_num = ADS_SCL_PORT,  // GPIO_NUM_22
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,  // 400kHz
    };
    ESP_ERROR_CHECK(i2c_param_config(0, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(0, I2C_MODE_MASTER, 0, 0, 0));
    
    // Fazer scan do barramento I2C
    ESP_LOGI("MAIN", "Fazendo scan do barramento I2C...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | 0, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(0, cmd, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            ESP_LOGI("MAIN", "Dispositivo I2C encontrado no endereço: 0x%02X", addr);
        }
    }
    
    ESP_LOGW("HAHA", "ANTES");
    test_lcd();  // LCD usa o driver I2C já inicializado
    
    // Aguardar estabilização do LVGL e suas tasks internas
    ESP_LOGI("MAIN", "Aguardando estabilização do LVGL...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGW("HAHA", "DEPOIS");
    
    esp_err_t ecg_ret = ecg_config();
    if (ecg_ret != ESP_OK) {
        ESP_LOGE("MAIN", "Falha na configuração do ECG: %s", esp_err_to_name(ecg_ret));
        ESP_LOGE("MAIN", "Continuando sem funcionalidade ECG...");
    } else {
        ESP_LOGI("MAIN", "ECG configurado com sucesso");
    }

    httpd_handle_t server = start_webserver();
    web_register_sd_api(server);
    mdns_app_start("ecg"); // -> http://ecg.local

    web_register_sd_api(server);

    vTaskDelay(pdMS_TO_TICKS(1000));

    xTaskCreatePinnedToCore(ecg_task, "ecg_task", 4096, NULL, 4, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(sd_task, "sd_task", 8192, NULL, 2, NULL, APP_CPU_NUM);

    // printf("Tasks criadas com sucesso!");
    
    while (1)
    {
        vTaskDelay(10);
    }
}