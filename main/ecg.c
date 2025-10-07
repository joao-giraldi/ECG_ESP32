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

gptimer_handle_t ecg_timer = NULL;
SemaphoreHandle_t ecg_sample_semaphore = NULL;

// Callback do timer para amostragem precisa
static bool IRAM_ATTR ecg_timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    BaseType_t high_task_awoken = pdFALSE;
    xSemaphoreGiveFromISR(ecg_sample_semaphore, &high_task_awoken);
    return high_task_awoken == pdTRUE;
}

esp_err_t ecg_config(void) {
    
    ESP_LOGI("ECG", "Configurando ADS1115");
    
    // Configurar dispositivo ADS1115
    ads_device = ads1115_config(ADS_I2C_PORT, 0x48);            // Porta I2C 0, endereço 0x48
    
    // Configurar parâmetros do ADS1115 para ECG
    ads1115_set_mux(&ads_device, ADS1115_MUX_0_1);              // Diferencial entre pinos 0 e 1
    ads1115_set_pga(&ads_device, ADS1115_FSR_2_048);            // Ganho = +-2.048V
    ads1115_set_mode(&ads_device, ADS1115_MODE_CONTINUOUS);     // Modo contínuo
    ads1115_set_sps(&ads_device, ADS1115_SPS_128);              // SPS MENOR = melhor filtro interno (128 SPS)
    ads1115_set_max_ticks(&ads_device, pdMS_TO_TICKS(50));      // Timeout maior para estabilidade
    
    // Teste de leitura inicial
    int16_t test_value = ads1115_get_raw(&ads_device);
    if (test_value == 0) {
        ESP_LOGE("ECG", "Falha na comunicação com ADS1115");
        return ESP_FAIL;
    }

    ESP_LOGI("ECG", "ADS1115 configurado com sucesso!");    
    // Criar semáforo para sincronização de amostragem
    ecg_sample_semaphore = xSemaphoreCreateBinary();
    if (ecg_sample_semaphore == NULL) {
        ESP_LOGE("ECG", "Falha ao criar semáforo de amostragem");
        return ESP_FAIL;
    }

    // Configurar timer para controle preciso de amostragem
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, resolução de 1us
    };
    
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &ecg_timer));
    
    gptimer_event_callbacks_t cbs = {
        .on_alarm = ecg_timer_callback,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(ecg_timer, &cbs, NULL));
    
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = ECG_TIMER_PERIOD_US,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(ecg_timer, &alarm_config));

    ESP_LOGI("ECG", "ADS1115 e timer configurados com sucesso!");    
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
        // Aguardar sinal do timer
        if(xSemaphoreTake(ecg_sample_semaphore, portMAX_DELAY) == pdTRUE) {
            
            // Verificar se os eletrodos estão conectados
            if(gpio_get_level(ECG_LO_MENOS) == 1 || gpio_get_level(ECG_LO_MAIS) == 1) {
                continue;
            }
                
            // Aquisição
            current_buffer[current_idx] = ecg_measure();
            //printf("%d\n", current_buffer[current_idx]);
            current_idx++;

            if(current_idx >= ECG_BUFFER_SIZE) {
                if (ecg_buffer_queue != NULL) {
                    if(xQueueSend(ecg_buffer_queue, &current_buffer, pdMS_TO_TICKS(50)) != pdTRUE) {
                        ESP_LOGE("ECG", "Erro no envio da fila");
                    } else {
                        ESP_LOGI("ECG", "Buffer enviado");
                    }
                } else {
                    ESP_LOGW("ECG", "Fila ECG não inicializada");
                }
                current_idx = 0;
                printf("\n--- BUFFER COMPLETO ---\n");  // Separador visual
            }
        }
    }
}

void finalize_ecg_collection(void) {
    ESP_LOGI("ECG", "Finalizando coleta ECG...");

    if (ecg_timer != NULL) {
        gptimer_stop(ecg_timer);
        gptimer_disable(ecg_timer);
    }
    
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