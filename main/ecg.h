#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_adc/adc_continuous.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"

#ifdef __cplusplus
extern "C" {
#endif

#define READ_LEN 32

void adc_config(void);
void adc_read(void);
void adc_deinit(void);

#ifdef __cplusplus
}
#endif