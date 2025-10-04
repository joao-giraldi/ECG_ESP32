#include "ports.h"

esp_err_t config_ports(void) {
    // Configurar GPIOs para leads-off detection
    ESP_ERROR_CHECK(gpio_set_direction(ECG_LO_MENOS, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_direction(ECG_LO_MAIS, GPIO_MODE_INPUT));
    
    // I2C pins
    ESP_ERROR_CHECK(gpio_set_direction(ADS_SDA_PORT, GPIO_MODE_INPUT_OUTPUT_OD));
    ESP_ERROR_CHECK(gpio_set_direction(ADS_SCL_PORT, GPIO_MODE_OUTPUT_OD));

    //Pinos do microSD
    ESP_ERROR_CHECK(gpio_set_direction(SD_CS_PORT, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(SD_SCK_PORT, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(SD_MOSI_PORT, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(SD_MISO_PORT, GPIO_MODE_INPUT));

    // Configurar pinos de interrupção individualmente para melhor diagnóstico
    ESP_LOGI("PORTS", "Configurando START_INT_PORT (GPIO %d)...", START_INT_PORT);
    gpio_config_t start_config = {
        .pin_bit_mask = (1ULL << START_INT_PORT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    ESP_ERROR_CHECK(gpio_config(&start_config));
    
    ESP_LOGI("PORTS", "Configurando STOP_INT_PORT (GPIO %d)...", STOP_INT_PORT);
    gpio_config_t stop_config = {
        .pin_bit_mask = (1ULL << STOP_INT_PORT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    ESP_ERROR_CHECK(gpio_config(&stop_config));
    
    // Instalar o serviço de ISR do GPIO
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    
    // Verificar se os pinos estão lendo corretamente
    int start_level = gpio_get_level(START_INT_PORT);
    int stop_level = gpio_get_level(STOP_INT_PORT);
    ESP_LOGI("PORTS", "Níveis iniciais - START_INT: %d, STOP_INT: %d", start_level, stop_level);
    
    ESP_LOGI("PORTS", "Pinos de interrupção configurados: START_INT=%d, STOP_INT=%d", START_INT_PORT, STOP_INT_PORT);

    return ESP_OK;
}