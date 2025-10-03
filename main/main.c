#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include "driver/i2c.h"
#include "esp_log.h"

# include "driver/gpio.h"

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

// Variáveis para controle das interrupções
static volatile bool start_pressed = false;
static volatile bool stop_pressed = false;

static void gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t) arg;
    if (gpio_num == START_INT) {
        start_pressed = true;
        // Usar ESP_EARLY_LOGI para logs dentro da ISR
        ESP_EARLY_LOGI("ISR", "START_INT (GPIO %d) triggered!", (int)gpio_num);
    } else if (gpio_num == STOP_INT) {
        stop_pressed = true;
        ESP_EARLY_LOGI("ISR", "STOP_INT (GPIO %d) triggered!", (int)gpio_num);
    } else {
        ESP_EARLY_LOGE("ISR", "ISR chamada com GPIO desconhecido: %d", (int)gpio_num);
    }
}

void app_main(void)
{
    static uint32_t collection_start_time = 0;
    static uint32_t last_lcd_update = 0;

    wifi_init_ap("ECG-ESP32", "12345678", 6, 4, false);
    
    ESP_ERROR_CHECK(config_ports());
    
    // Registrar handlers das interrupções
    ESP_LOGI("MAIN", "Registrando handlers das interrupções...");
    ESP_ERROR_CHECK(gpio_isr_handler_add(START_INT, gpio_isr_handler, (void*) START_INT));
    ESP_ERROR_CHECK(gpio_isr_handler_add(STOP_INT, gpio_isr_handler, (void*) STOP_INT));
    ESP_LOGI("MAIN", "Handlers das interrupções registrados com sucesso");
    
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
        .sda_io_num = ADS_SDA_PORT,
        .scl_io_num = ADS_SCL_PORT,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };

    ESP_ERROR_CHECK(i2c_param_config(0, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(0, I2C_MODE_MASTER, 0, 0, 0));
    
    // Imagem inicial do LCD
    lcd_config();
    boot_image();
    //vTaskDelay(pdMS_TO_TICKS(2000));
    //lcd_display_welcome();
    //vTaskDelay(pdMS_TO_TICKS(2000));
    //lcd_display_network();
    //vTaskDelay(pdMS_TO_TICKS(2000));
    //lcd_display_server();
    //vTaskDelay(pdMS_TO_TICKS(2000));
    //lcd_display_collecting(444);
    //vTaskDelay(pdMS_TO_TICKS(2000));
    //lcd_display_stopped(444, get_current_filename());
    //vTaskDelay(pdMS_TO_TICKS(2000));

    esp_err_t ecg_ret = ecg_config();
    if (ecg_ret != ESP_OK) {
        ESP_LOGE("MAIN", "Falha na configuração do ECG: %s", esp_err_to_name(ecg_ret));
    } else {
        ESP_LOGI("MAIN", "ECG configurado com sucesso");
    }

    httpd_handle_t server = start_webserver();
    web_register_sd_api(server);
    mdns_app_start("ecg");                      // URL -> http://ecg.local

    web_register_sd_api(server);

    xTaskCreatePinnedToCore(sd_task, "sd_task", 8192, NULL, 2, NULL, APP_CPU_NUM);
    xTaskCreatePinnedToCore(ecg_task, "ecg_task", 4096, NULL, 4, NULL, APP_CPU_NUM);
    
    vTaskDelay(pdMS_TO_TICKS(3000));

    //lcd_display_welcome();
    
    while (1)
    {
        // Verificar interrupções (remover depois se não precisar)
        if (start_pressed) {
            start_pressed = false;  // Reset da flag
            ESP_LOGI("MAIN", "Botão START foi pressionado!");
            // Aqui você adicionará a lógica para iniciar a coleta
        }
        
        if (stop_pressed) {
            stop_pressed = false;   // Reset da flag
            ESP_LOGI("MAIN", "Botão STOP foi pressionado!");
            // Aqui você adicionará a lógica para parar a coleta
        }
        
        vTaskDelay(10);

        /*
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

        lcd_display_stopped(132, get_current_filename());
        vTaskDelay(pdMS_TO_TICKS(5000));
        */
    }
}