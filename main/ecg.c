#include "ecg.h"
#include "ads1115.h"
#include <string.h>

#define ECG_MAX_VOLTAGE     2.048f
#define ECG_RESOLUTION      32767  // 15-bit signed (2^15 - 1)

ads1115_t ads_device;
extern QueueHandle_t ecg_buffer_queue;

// Variáveis para controle do buffer do ECG
static uint16_t current_idx = 0;
static int16_t current_buffer[ECG_BUFFER_SIZE];

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
    int16_t test_value = ads1115_get_raw(&ads_device);
    if (test_value == 0) {
        ESP_LOGE("ECG", "Falha na comunicação com ADS1115");
        return ESP_FAIL;
    }

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
    ESP_LOGI("ECG", "Iniciando task de aquisição ECG");
    
    // Resetar variáveis ao iniciar
    current_idx = 0;
    memset(current_buffer, 0, sizeof(current_buffer));
    
    while(1) {
        // Verificar se os eletrodos estão conectados
        if(gpio_get_level(ECG_LO_MENOS) == 1 || gpio_get_level(ECG_LO_MAIS) == 1) {
            ESP_LOGW("ECG", "Eletrodos desconectados");
            vTaskDelay(pdMS_TO_TICKS(500));
        } else {
            current_buffer[current_idx] = ecg_measure();
            printf("%d\n", current_buffer[current_idx]);
            current_idx++;

            if(current_idx >= ECG_BUFFER_SIZE) {
                if (ecg_buffer_queue != NULL) {
                    // Tentar enviar por até 1 segundo
                    if(xQueueSend(ecg_buffer_queue, &current_buffer, pdMS_TO_TICKS(1000)) != pdTRUE) {
                        ESP_LOGW("ECG", "Fila cheia, descartando buffer");
                    } else {
                        ESP_LOGI("ECG", "Buffer enviado para SD (512 amostras)");
                    }
                } else {
                    ESP_LOGW("ECG", "Fila ECG não inicializada, descartando dados");
                }
                current_idx = 0;
                vTaskDelay(pdMS_TO_TICKS(5));
            }
            vTaskDelay(pdMS_TO_TICKS((int)ECG_DELAY_MS));
        }
    }
}

void finalize_ecg_collection(void) {
    ESP_LOGI("ECG", "Finalizando coleta ECG...");
    
    if(current_idx > 0) {
        ESP_LOGI("ECG", "Enviando buffer parcial (%d amostras)", current_idx);
        
        // Preencher resto do buffer com zeros para manter tamanho consistente
        memset(&current_buffer[current_idx], 0, (ECG_BUFFER_SIZE - current_idx) * sizeof(int16_t));
        
        if (ecg_buffer_queue != NULL) {
            if(xQueueSend(ecg_buffer_queue, &current_buffer, pdMS_TO_TICKS(2000)) != pdTRUE) {
                ESP_LOGW("ECG", "Não foi possível enviar buffer parcial - fila cheia");
            } else {
                ESP_LOGI("ECG", "Buffer parcial enviado com sucesso");
            }
        } else {
            ESP_LOGW("ECG", "Fila ECG não disponível para buffer parcial");
        }
    } else {
        ESP_LOGI("ECG", "Nenhum dado parcial para enviar");
    }
    
    // Resetar índice para próxima coleta
    current_idx = 0;
    ESP_LOGI("ECG", "Finalização ECG concluída");
}