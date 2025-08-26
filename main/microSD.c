#include "microSD.h"

sdmmc_card_t* sd_card;
const char mount_point[] = MOUNT_POINT;

void write_file(const char *path, char *data) {
    ESP_LOGI("SD", "Abrindo arquivo");
    FILE *f = fopen(path, "w");
    if(f == NULL) {
        ESP_LOGE("SD", "Falha ao abrir o arquivo");
        return;
    }
    fprintf(f, data);
    fclose(f);
    ESP_LOGI("SD", "Arquivo escrito");
}

void read_file(const char *path) {
    ESP_LOGI("SD", "Abrindo arquivo");
    FILE *f = fopen(path, "r");
    if(f == NULL) {
        ESP_LOGE("SD", "Falha ao abrir o arquivo");
        return;
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);

    char *pos = strchr(line, '\n');
    if(pos) {
        *pos = '\0';
    }
    ESP_LOGI("SD", "Lido do arquivo: %s", line);
}

void sd_config(void) {

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    ESP_LOGI("SD", "Inicializando SD Card");

    sdmmc_host_t sd_host = SDSPI_HOST_DEFAULT();
    sd_host.max_freq_khz = 5000;

    spi_bus_config_t bus_conf = {
        .mosi_io_num = SD_MOSI_PORT,
        .miso_io_num = SD_MISO_PORT,
        .sclk_io_num = SD_SCK_PORT,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000
    };

    if(spi_bus_initialize(sd_host.slot, &bus_conf, SDSPI_DEFAULT_DMA) != ESP_OK) {
        ESP_LOGE("SD", "Falha ao inicializar o SD.");
        return;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PORT;
    slot_config.host_id = sd_host.slot;

    ESP_LOGI("ESP", "Montando sistema de arquivos");

    if(esp_vfs_fat_sdspi_mount(mount_point, &sd_host, &slot_config, &mount_config, &sd_card) != ESP_OK) {
        ESP_LOGE("SD", "Falha ao montar o sistema de arquivos.");
    }

    ESP_LOGI("SD", "Filesystem mounted");

    sdmmc_card_print_info(stdout, sd_card);

    /*
    sdspi_dev_handle_t sd_handle;
    if(sdspi_host_init_device(&slot_config, &sd_handle) != ESP_OK) {
        ESP_LOGE("SD", "Falha ao inicializar dispositivo SD SPI:");
        spi_bus_free(sd_host.slot);
        return;
    }

    sd_host.slot = sd_handle;

    sd_card = malloc(sizeof(sdmmc_card_t));
    if(sd_card == NULL) {
        ESP_LOGE("SD", "Falha ao alocar memória para sd_card");
        sdspi_host_deinit();
        spi_bus_free(sd_host.slot);
        return;
    }

    if(sdmmc_card_init(&sd_host, sd_card) != ESP_OK) {
        ESP_LOGE("SD", "Falha ao inicializar cartão SD");
        free(sd_card);
        sdspi_host_deinit();
        spi_bus_free(sd_host.slot);
        return;
    }

    ESP_LOGI("SD", "Cartão SD inicializado com sucesso");
    */
}
