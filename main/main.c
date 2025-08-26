#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "ecg.h"
#include "microSD.h"
#include "ports.h"

#ifndef APP_CPU_NUM
#define APP_CPU_NUM     PRO_CPU_NUM
#endif

#define ECG_BUFFER_SIZE     512

//static uint16_t ecg_buf_idx = 0;
//static uint16_t buffer[ECG_BUFFER_SIZE];
//static uint16_t ecg_value;

// Variáveis globais
i2c_dev_t device;
float gain_val;

extern sdmmc_card_t* sd_card;

void app_main(void) {
    printf("=== ECG Monitor com ADS1115 ===\n");
    
    config_ports();
    ESP_ERROR_CHECK(ecg_config());

    sd_config();
    
    vTaskDelay(pdMS_TO_TICKS(1000));

    //TESTE SD
    const char *file_hello = MOUNT_POINT"/hello.txt";
    char data[64];
    snprintf(data, 64, "%s %s!\n", "Hello", sd_card->cid.name);
    write_file(file_hello, data);
    vTaskDelay(1000);
    read_file(file_hello);
    vTaskDelay(1000);
    
    //xTaskCreatePinnedToCore(ecg_task, "ecg_task", 4096, NULL, 3, NULL, APP_CPU_NUM);

    printf("Tasks criadas com sucesso!");
}