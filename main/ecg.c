#include "ecg.h"

i2c_dev_t device;
float gain_val;

esp_err_t ecg_config(void) {
    // Inicializar I2C
    ESP_ERROR_CHECK(i2cdev_init());

    // Configurar ADS1115 para ECG
    gain_val = ads111x_gain_values[ADS_GAIN];

    ESP_ERROR_CHECK(ads111x_init_desc(&device, ADS111X_ADDR_GND, ADS_I2C_PORT, ADS_SDA_PORT, ADS_SCL_PORT));
    ESP_ERROR_CHECK(ads111x_set_mode(&device, ADS111X_MODE_CONTINUOUS));
    ESP_ERROR_CHECK(ads111x_set_data_rate(&device, ADS111X_DATA_RATE_475));
    ESP_ERROR_CHECK(ads111x_set_input_mux(&device, ADS111X_MUX_0_1));
    ESP_ERROR_CHECK(ads111x_set_gain(&device, ADS_GAIN));

    return ESP_OK;
}

void ecg_measure(void) {
    int16_t raw = 0;
    
    if(ads111x_get_value(&device, &raw) == ESP_OK) {
        // Saída principal (usar para plotagem)
        printf("%d\n", raw);        
    } else {
        printf("# ADC Error\n");
    }
}

void ecg_task(void *pvParameters) {
    while(1) {
        // Verificar leads-off (eletrodos desconectados)
        if(gpio_get_level(ECG_LO_MENOS) == 1 || gpio_get_level(ECG_LO_MAIS) == 1) {
            printf("# LEADS OFF!\n");
            vTaskDelay(pdMS_TO_TICKS(500));
        } else {
            ecg_measure();
            vTaskDelay(pdMS_TO_TICKS((int)ECG_DELAY_MS));
        }
    }
}