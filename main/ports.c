#include "ports.h"

esp_err_t config_ports(void) {
    // Configurar GPIOs para leads-off detection
    ESP_ERROR_CHECK(gpio_set_direction(ECG_LO_MENOS, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_direction(ECG_LO_MAIS, GPIO_MODE_INPUT));
    
    // I2C pins
    ESP_ERROR_CHECK(gpio_set_direction(ADS_SDA_PORT, GPIO_MODE_INPUT_OUTPUT_OD));  // SDA
    ESP_ERROR_CHECK(gpio_set_direction(ADS_SCL_PORT, GPIO_MODE_OUTPUT_OD));        // SCL

    //Pinos do microSD
    ESP_ERROR_CHECK(gpio_set_direction(SD_CS_PORT, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(SD_SCK_PORT, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(SD_MOSI_PORT, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(SD_MISO_PORT, GPIO_MODE_INPUT));

    return ESP_OK;
}