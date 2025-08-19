#include "ecg.h"

adc_continuous_handle_t adc = NULL;

void adc_config(void) {
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = 64,
    };

    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &adc));

    adc_digi_pattern_config_t adc_pattern = {
        .atten = ADC_ATTEN_DB_12,
        .channel = ADC_CHANNEL_0,
        .unit = ADC_UNIT_1,
        .bit_width = ADC_BITWIDTH_12
    };

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SOC_ADC_SAMPLE_FREQ_THRES_LOW,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .pattern_num = 1,
        .adc_pattern = &adc_pattern
    };

    ESP_ERROR_CHECK(adc_continuous_config(adc, &dig_cfg));
    
    // Iniciar o ADC uma vez
    ESP_ERROR_CHECK(adc_continuous_start(adc));
}

void adc_read(void) {
    uint8_t result[READ_LEN];
    uint32_t ret_num = 0;

    while(1) {
        if(gpio_get_level(18) == 1 || gpio_get_level(19) == 1) {
            printf("!\n");
        }
        else {
            esp_err_t ret = adc_continuous_read(adc, result, READ_LEN, &ret_num, 100); // Timeout aumentado para 100ms
            
            if(ret == ESP_OK) {
                for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                    adc_digi_output_data_t *p = (adc_digi_output_data_t*)&result[i];
                    printf("%d\n", p->type1.data);
                }
            }
            else if(ret == ESP_ERR_TIMEOUT) {
                // Timeout é normal quando não há dados suficientes
                printf("ADC timeout\n");
            }
            else {
                printf("Erro na leitura ADC: %s\n", esp_err_to_name(ret));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Delay de 10ms
    }
}

void adc_deinit(void) {
    if (adc != NULL) {
        ESP_ERROR_CHECK(adc_continuous_stop(adc));
        ESP_ERROR_CHECK(adc_continuous_deinit(adc));
        adc = NULL;
    }
}