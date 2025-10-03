#pragma once

#include <driver/sdspi_host.h>
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include "freertos/queue.h"

#include "dirent.h"
#include "sys/stat.h"

#include "ports.h"
#include "ecg.h"

#define MOUNT_POINT "/sdcard"

#ifdef __cplusplus
extern "C" {
#endif

void sd_config(void);
void sd_task(void *pvParameters);
void write_file(const char *path, int16_t *data);
void read_file(const char *path);

uint8_t get_next_file_number(void);
const char* get_current_filename(void);

#ifdef __cplusplus
}
#endif