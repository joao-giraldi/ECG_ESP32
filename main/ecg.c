#include "ecg.h"
#include "ads1115.h"

ads1115_t ads_device;
extern QueueHandle_t ecg_buffer_queue;

#define ECG_MAX_VOLTAGE     2.048f
#define ECG_RESOLUTION      32767  // 15-bit signed (2^15 - 1)

esp_err_t ecg_config(void) {
    
    ESP_LOGI("ECG", "Configurando ADS1115");
    
    // Configurar dispositivo ADS1115
    ads_device = ads1115_config(ADS_I2C_PORT, 0x48);            // Porta I2C 0, endereço 0x48
    
    // Configurar parâmetros do ADS1115 para ECG
    ads1115_set_mux(&ads_device, ADS1115_MUX_0_1);              // Diferencial entre pinos 0 e 1
    ads1115_set_pga(&ads_device, ADS1115_FSR_2_048);            // Fundo de escala ±2.048V
    ads1115_set_mode(&ads_device, ADS1115_MODE_CONTINUOUS);     // Modo contínuo
    ads1115_set_sps(&ads_device, ADS1115_SPS_64);               // 64 samples por segundo
    ads1115_set_max_ticks(&ads_device, pdMS_TO_TICKS(1000));    // Timeout de 1 segundo
    
    // Teste de leitura inicial
    ESP_LOGI("ECG", "Testando leitura do ADS1115...");
    int16_t test_value = ads1115_get_raw(&ads_device);
    if (test_value == 0) {
        ESP_LOGE("ECG", "Falha na comunicação com ADS1115");
        return ESP_FAIL;
    }
    
    double voltage = ads1115_get_voltage(&ads_device);
    ESP_LOGI("ECG", "Teste inicial - Raw: %d, Voltage: %.3f V", test_value, voltage);
    ESP_LOGI("ECG", "ADS1115 configurado com sucesso!");
    
    return ESP_OK;
}

int16_t ecg_measure(void) {
    int16_t raw = ads1115_get_raw(&ads_device);
    if (raw == 0) {
        ESP_LOGW("ECG", "Leitura retornou 0 - possível erro de comunicação");
    }
    return raw;
}

void ecg_task(void *pvParameters) {
    uint16_t idx = 0;
    int16_t buffer[ECG_BUFFER_SIZE];
    
    ESP_LOGI("ECG", "Iniciando task de aquisição ECG");
    
    while(1) {
        // Verificar leads-off (eletrodos desconectados)
        if(gpio_get_level(ECG_LO_MENOS) == 1 || gpio_get_level(ECG_LO_MAIS) == 1) {
            printf("# LEADS OFF!\n");
            vTaskDelay(pdMS_TO_TICKS(500));
        } else {
            buffer[idx] = ecg_measure();
            printf("%d\n", buffer[idx]);
            idx++;

            if(idx == ECG_BUFFER_SIZE) {
                // Verificar se a fila existe antes de usar
                if (ecg_buffer_queue != NULL) {
                    xQueueSend(ecg_buffer_queue, &buffer, pdTICKS_TO_MS(100));
                } else {
                    ESP_LOGW("ECG", "Fila ECG não inicializada, descartando dados");
                }
                printf("%d\n", buffer[ECG_BUFFER_SIZE-1]);
                idx = 0;
                vTaskDelay(pdTICKS_TO_MS(5));
            }
            vTaskDelay(pdMS_TO_TICKS((int)ECG_DELAY_MS));
        }
    }
}