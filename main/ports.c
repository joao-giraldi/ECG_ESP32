#include "ports.h"

void config_ports(void) {
    // Configurar GPIOs para leads-off detection
    gpio_set_direction(ECG_LO_MENOS, GPIO_MODE_INPUT);
    gpio_set_direction(ECG_LO_MAIS, GPIO_MODE_INPUT);
    
    // I2C pins
    gpio_set_direction(ADS_SDA_PORT, GPIO_MODE_INPUT_OUTPUT_OD);  // SDA
    gpio_set_direction(ADS_SCL_PORT, GPIO_MODE_OUTPUT_OD);        // SCL
}