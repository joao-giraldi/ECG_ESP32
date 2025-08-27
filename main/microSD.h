#pragma once

#include <driver/sdspi_host.h>
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include "freertos/queue.h"

#include "ports.h"
#include "ecg.h"

#define MOUNT_POINT "/sdcard"

#ifdef __cplusplus
extern "C" {
#endif

void sd_config(void);
void sd_task(void *pvParameters);
void write_file(const char *path, uint16_t *data);
void read_file(const char *path);

#ifdef __cplusplus
}
#endif