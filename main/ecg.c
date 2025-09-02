#include "ecg.h"

i2c_dev_t device;
float gain_val;
extern QueueHandle_t ecg_buffer_queue;

#define ECG_MAX_VOLTAGE     2.048f
#define ECG_RESOLUTION      65535

// A DISCUTIR...
float raw_to_voltage(uint16_t raw) {
    return (raw * ECG_MAX_VOLTAGE) / ECG_RESOLUTION;
}

esp_err_t ecg_config(void) {
    // Inicializar I2C
    ESP_ERROR_CHECK(i2cdev_init());

    // Configurar ADS1115 para ECG
    gain_val = ads111x_gain_values[ADS_GAIN];

    ESP_ERROR_CHECK(ads111x_init_desc(&device, ADS111X_ADDR_GND, ADS_I2C_PORT, ADS_SDA_PORT, ADS_SCL_PORT));
    ESP_ERROR_CHECK(ads111x_set_mode(&device, ADS111X_MODE_CONTINUOUS));
    ESP_ERROR_CHECK(ads111x_set_data_rate(&device, ADS111X_DATA_RATE_64));
    ESP_ERROR_CHECK(ads111x_set_input_mux(&device, ADS111X_MUX_0_1));
    ESP_ERROR_CHECK(ads111x_set_gain(&device, ADS_GAIN));

    return ESP_OK;
}

int16_t ecg_measure(void) {
    int16_t raw = 0;
    
    if(ads111x_get_value(&device, &raw) == ESP_OK) {
        return raw;
    } else {
        ESP_LOGE("ADC", "ERRO NA LEITURA DO ADC.");
        return 0;
    }
}

void ecg_task(void *pvParameters) {
    uint16_t idx = 0;
    int16_t buffer[ECG_BUFFER_SIZE];
    while(1) {
        // Verificar leads-off (eletrodos desconectados)
        if(gpio_get_level(ECG_LO_MENOS) == 1 || gpio_get_level(ECG_LO_MAIS) == 1) {
            printf("# LEADS OFF!\n");
            vTaskDelay(pdMS_TO_TICKS(500));
        } else {
            buffer[idx] = ecg_measure();
            printf("%d\n",buffer[idx]);
            idx++;

            if(idx == ECG_BUFFER_SIZE) {
                xQueueSend(ecg_buffer_queue, &buffer, pdTICKS_TO_MS(100));
                printf("%d\n",buffer[ECG_BUFFER_SIZE-1]);
                idx = 0;
                vTaskDelay(pdTICKS_TO_MS(5));
            }
            vTaskDelay(pdMS_TO_TICKS((int)ECG_DELAY_MS));
        }
    }
}