#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ads111x.h>
#include <string.h>
#include <math.h>

#include "ecg.h"

#define SDA_PORT        21
#define SCL_PORT        22
#define I2C_PORT        0
#define GAIN            ADS111X_GAIN_2V048  // Menor gain para ECG (mV range)

// Configurações para ECG
#define ECG_SAMPLE_RATE     100.0           // 100 Hz sample rate
#define ECG_DELAY_MS        (1000.0/ECG_SAMPLE_RATE)

// Filtros para ECG
#define LOWPASS_CUTOFF      40.0    // Remove ruído > 40Hz  
#define HIGHPASS_CUTOFF     0.5     // Remove DC drift < 0.5Hz
#define NOTCH_FREQ          60.0    // Remove 60Hz (rede elétrica)

#ifndef APP_CPU_NUM
#define APP_CPU_NUM     PRO_CPU_NUM
#endif

// Variáveis globais
static i2c_dev_t device;
static float gain_val;

// Função de medição otimizada para ECG
static void measure_ecg(void) {
    int16_t raw = 0;
    
    if(ads111x_get_value(&device, &raw) == ESP_OK) {
        // Saída principal (usar para plotagem)
        printf("%d\n", raw);        
    } else {
        printf("# ADC Error\n");
    }
}

void config_ports(void) {
    // Configurar GPIOs para leads-off detection
    gpio_set_direction(18, GPIO_MODE_INPUT);
    gpio_set_direction(19, GPIO_MODE_INPUT);
    
    // I2C pins
    gpio_set_direction(21, GPIO_MODE_INPUT_OUTPUT_OD);  // SDA
    gpio_set_direction(22, GPIO_MODE_OUTPUT_OD);        // SCL
}

void app_main(void) {
    printf("=== ECG Monitor com ADS1115 ===\n");
    
    config_ports();

    // Inicializar I2C
    ESP_ERROR_CHECK(i2cdev_init());

    // Configurar ADS1115 para ECG
    gain_val = ads111x_gain_values[GAIN];

    ESP_ERROR_CHECK(ads111x_init_desc(&device, ADS111X_ADDR_GND, I2C_PORT, SDA_PORT, SCL_PORT));
    ESP_ERROR_CHECK(ads111x_set_mode(&device, ADS111X_MODE_CONTINUOUS));
    ESP_ERROR_CHECK(ads111x_set_data_rate(&device, ADS111X_DATA_RATE_475));  // 475 SPS para ECG
    ESP_ERROR_CHECK(ads111x_set_input_mux(&device, ADS111X_MUX_0_1));       // Diferencial A0-A1
    ESP_ERROR_CHECK(ads111x_set_gain(&device, GAIN));

    // Delay para estabilizar
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Loop principal de aquisição
    while(1) {
        // Verificar leads-off (eletrodos desconectados)
        if(gpio_get_level(18) == 1 || gpio_get_level(19) == 1) {
            printf("# LEADS OFF!\n");
            vTaskDelay(pdMS_TO_TICKS(500));
        } else {
            measure_ecg();
            vTaskDelay(pdMS_TO_TICKS((int)ECG_DELAY_MS));  // para 100Hz
        }
    }
}