#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include "driver/i2c.h"
#include "esp_log.h"
# include "driver/gpio.h"
#include <stdatomic.h>  

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

#define DEBOUNCE_TIME_MS    200    // 200ms de debounce

typedef volatile enum {
    SYSTEM_BOOT_STATE,
    SYSTEM_COLLECTING_STATE,
    SYSTEM_WEB_STATE
} system_state_t;

// Variável de estado thread-safe
static volatile _Atomic system_state_t sys_state = SYSTEM_BOOT_STATE;

QueueHandle_t ecg_buffer_queue = NULL;

// Handles das tasks para controle (suspender/resumir)
static TaskHandle_t sd_task_handle = NULL;
static TaskHandle_t ecg_task_handle = NULL;

// Tempos para controle de debounce na ISR
static volatile uint32_t last_start_time = 0;
static volatile uint32_t last_stop_time = 0;

static void gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t) arg;
    uint32_t current_time = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
    
    if (gpio_num == START_INT_PORT) {
        // Verificar debounce para START
        if (current_time - last_start_time > DEBOUNCE_TIME_MS) {
            // Operação atômica - thread-safe
            atomic_store(&sys_state, SYSTEM_COLLECTING_STATE);
            last_start_time = current_time;
            ESP_EARLY_LOGI("ISR", "START_INT (GPIO %d) - Estado: COLLECTING", (int)gpio_num);
        } else {
            ESP_EARLY_LOGD("ISR", "START bounce ignorado (time diff: %d ms)", 
                           (int)(current_time - last_start_time));
        }
    } else if (gpio_num == STOP_INT_PORT) {
        // Verificar debounce para STOP
        if (current_time - last_stop_time > DEBOUNCE_TIME_MS) {
            // Operação atômica - thread-safe
            atomic_store(&sys_state, SYSTEM_WEB_STATE);
            last_stop_time = current_time;
            ESP_EARLY_LOGI("ISR", "STOP_INT (GPIO %d) - Estado: WEB", (int)gpio_num);
        } else {
            ESP_EARLY_LOGD("ISR", "STOP bounce ignorado (time diff: %d ms)", 
                           (int)(current_time - last_stop_time));
        }
    }
}

void app_main(void)
{
    static uint32_t collection_start_time = 0;
    static uint32_t last_lcd_update = 0;
    uint32_t current_time = 0;

    wifi_init_ap("ECG-ESP32", "12345678", 6, 4, false);
    
    ESP_ERROR_CHECK(config_ports());
    
    // Registrar handlers das interrupções
    ESP_LOGI("MAIN", "Registrando handlers das interrupções...");
    ESP_ERROR_CHECK(gpio_isr_handler_add(START_INT_PORT, gpio_isr_handler, (void*) START_INT_PORT));
    ESP_ERROR_CHECK(gpio_isr_handler_add(STOP_INT_PORT, gpio_isr_handler, (void*) STOP_INT_PORT));
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
    
    // Configura e mostra logo do sistema
    lcd_config();
    boot_image();

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

    // Criar tasks com handles para controle
    xTaskCreatePinnedToCore(sd_task, "sd_task", 8192, NULL, 2, &sd_task_handle, APP_CPU_NUM);
    xTaskCreatePinnedToCore(ecg_task, "ecg_task", 4096, NULL, 4, &ecg_task_handle, APP_CPU_NUM);

    vTaskSuspend(ecg_task_handle);

    //vTaskStartScheduler();

    ESP_LOGI("MAIN", "Tasks criadas e suspensas - aguardando comando para iniciar");
    
    // Variável para detectar mudanças de estado
    static system_state_t last_state = SYSTEM_COLLECTING_STATE;
    
    // Variáveis para rotação de telas no estado WEB
    static uint32_t last_screen_change = 0;
    static uint8_t current_screen = 0; // 0=stopped, 1=network, 2=server
    
    // Variável para armazenar o tempo total da última coleta (congelado)
    static uint32_t final_collection_time = 0;
    
    // Tempo para estabilizar sistema e mostrar a logo
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    while (1)
    {
        // Leitura atômica do estado atual
        system_state_t current_state = atomic_load(&sys_state);
        
        // Detectar mudanças de estado
        if (current_state != last_state) {
            ESP_LOGI("MAIN", "Mudança de estado: %d -> %d", last_state, current_state);
            
            switch (current_state) {
                case SYSTEM_BOOT_STATE:
                    ESP_LOGI("MAIN", "Estado: BOOT");
                    lcd_display_welcome();
                    break;
                    
                case SYSTEM_COLLECTING_STATE:
                    ESP_LOGI("MAIN", "Estado: COLLECTING - Ativando tasks de coleta");
                    collection_start_time = 0;
                    last_lcd_update = 0;
                    // Ativar task de coleta
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    vTaskResume(ecg_task_handle);
                    lcd_display_collecting(0);
                    break;
                    
                case SYSTEM_WEB_STATE:
                    ESP_LOGI("MAIN", "Estado: WEB - Suspendendo tasks de coleta");
                    // Suspender task de coleta
                    vTaskSuspend(ecg_task_handle);
                    // Calcular e CONGELAR o tempo total de coleta
                    final_collection_time = (collection_start_time == 0) ? 0 : 
                        ((xTaskGetTickCount() * portTICK_PERIOD_MS / 1000) - collection_start_time);
                    // Resetar para começar com tela STOPPED
                    last_screen_change = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;
                    current_screen = 0; // Começar com STOPPED
                    // Mostrar primeira tela imediatamente
                    lcd_display_stopped(final_collection_time, get_current_filename());
                    ESP_LOGI("MAIN", "Mostrando tela inicial: STOPPED (tempo FINAL: %d s)", final_collection_time);
                    break;
            }
            
            last_state = current_state;
        } else {
            switch (current_state) {
                case SYSTEM_COLLECTING_STATE:
                    current_time = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;

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
                    break;
                case SYSTEM_WEB_STATE:
                    // Rotação entre telas: stopped -> network -> server -> stopped...
                    uint32_t current_time_web = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;
                    
                    // Inicializar timer se for a primeira vez
                    if (last_screen_change == 0) {
                        last_screen_change = current_time_web;
                    }
                    
                    // Trocar tela a cada 3 segundos
                    if (current_time_web - last_screen_change >= 3) {
                        current_screen = (current_screen + 1) % 3;
                        last_screen_change = current_time_web;
                        
                        ESP_LOGD("MAIN", "Rotação de tela: %d", current_screen);
                        
                        switch (current_screen) {
                            case 0:
                                ESP_LOGD("MAIN", "Mostrando tela: STOPPED (tempo congelado: %d s)", final_collection_time);
                                // Usar o tempo CONGELADO da coleta
                                lcd_display_stopped(final_collection_time, get_current_filename());
                                break;
                            case 1:
                                ESP_LOGD("MAIN", "Mostrando tela: NETWORK");
                                lcd_display_network();
                                break;
                            case 2:
                                ESP_LOGD("MAIN", "Mostrando tela: SERVER");
                                lcd_display_server();
                                break;
                        }
                    }
                    break;
                default:
                    break;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}