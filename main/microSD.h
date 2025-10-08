#pragma once

#include <driver/sdspi_host.h>
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include "diskio_impl.h"
#include "diskio_sdmmc.h"
#include "ff.h"
#include <string.h>
#include <stdlib.h>

#include "ports.h"
#include "ecg.h"
#include "lcd.h"
#include "sys_error.h"

#define MOUNT_POINT "/sdcard"
#define MAX_FILES   50

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sd_config(void);
void sd_task(void *pvParameters);
void write_file(const char *path, int16_t *data);

uint8_t get_next_file_number(void);
const char* get_current_filename(void);
void finalize_current_file(void);

#ifdef __cplusplus
}
#endif