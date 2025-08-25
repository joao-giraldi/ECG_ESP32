#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ads111x.h>
#include <string.h>
#include <sys/unistd.h> 

#include "ecg.h"

#define SDA_PORT        21
#define SCL_PORT        22
#define I2C_PORT        0
#define GAIN            ADS111X_GAIN_2V048  // Menor gain para ECG (mV range)

// Configurações para ECG
#define ECG_SAMPLE_RATE     100.0           // 100 Hz sample rate
#define ECG_DELAY_MS        (1000.0/ECG_SAMPLE_RATE)


#ifndef APP_CPU_NUM
#define APP_CPU_NUM     PRO_CPU_NUM
#endif

#define ECG_BUFFER_SIZE     512

static uint16_t ecg_buf_idx = 0;
static uint16_t buffer[ECG_BUFFER_SIZE];
static uint16_t ecg_value;

// Variáveis globais
static i2c_dev_t device;
static float gain_val;

// Função de medição otimizada para ECG
static uint16_t measure_ecg(void) {
    int16_t raw = 0;
    
    if(ads111x_get_value(&device, &raw) == ESP_OK) {
        // Saída principal (usar para plotagem)
        printf("%d\n", raw);
        return raw;       
    } else {
        printf("# ADC Error\n");
        return 0;
    }
}

// Ex.: append_bin("/leitura/leitura_1.bin", buffer, 512 * sizeof(uint16_t));
void append_bin(const char *path, const void *buf, size_t bytes)
{
    FILE *f = fopen(path, "ab");
    if (!f)
    {
        printf("ERRO AO ABRIR O ARQUIVO!!!!!!!!!!!!!!!!!!!!\n");
        return;
    };

    fwrite(buf, 1, bytes, f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
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
            ecg_value = measure_ecg();

            if(ecg_buf_idx == ECG_BUFFER_SIZE - 1)
            {
                ecg_buf_idx = 0;
                append_bin("/sdcard/leitura/leitura_1.bin", buffer, 512 * sizeof(uint16_t));
                buffer[ecg_buf_idx] = ecg_value;
                ecg_buf_idx++;
            }
            else {
                buffer[ecg_buf_idx] = ecg_value;
                ecg_buf_idx++;
            }
            vTaskDelay(pdMS_TO_TICKS((int)ECG_DELAY_MS));  // para 100Hz
        }
    }
}