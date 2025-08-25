#pragma once

#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

// MACROS PARA AS PORTS UTILIZADAS

#define ADS_I2C_PORT        0
#define ECG_LO_MENOS        18
#define ECG_LO_MAIS         19
#define ADS_SDA_PORT        21
#define ADS_SCL_PORT        22

void config_ports(void);

#ifdef __cplusplus
}
#endif