#include "microSD.h"
#include "freertos/semphr.h"

sdmmc_card_t* sd_card;
const char mount_point[] = MOUNT_POINT;

// Variável global para armazenar o nome do arquivo atual
static char current_filename[64] = "";

// Mutex para proteger acesso à variável current_filename
static SemaphoreHandle_t filename_mutex = NULL;

extern QueueHandle_t ecg_buffer_queue;
extern TaskHandle_t ecg_handler;

void write_file(const char *path, int16_t *data) {
    ESP_LOGI("SD", "Abrindo arquivo para APPEND BINÁRIO");
    static FILE *f = NULL;
    static uint8_t write_count = 0;

    if(f == NULL) {
        f = fopen(path, "ab");
        if(f == NULL) {
            ESP_LOGE("SD", "Falha ao abrir o arquivo");
            return;
        }
    }

    fflush(f);

    size_t written = fwrite(data, sizeof(uint16_t), ECG_BUFFER_SIZE, f);
    
    if(written != ECG_BUFFER_SIZE) {
        ESP_LOGE("SD", "Erro na escrita: %d/%d", written, ECG_BUFFER_SIZE);
    }

    fflush(f);
        
    write_count++;

    if(write_count % 2 == 0) {
        fclose(f);
        f = NULL;  // Será reaberto na próxima escrita
        write_count = 0;
        ESP_LOGI("SD", "Arquivo fechado e reaberto para segurança");
    }
}

// GEPETO LEU
void read_file(const char *path) {
    ESP_LOGI("SD", "Abrindo arquivo para LEITURA BINÁRIA");
    FILE *f = fopen(path, "rb");
    if(f == NULL) {
        ESP_LOGE("SD", "Falha ao abrir o arquivo");
        return;
    }
    
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    int total_samples = file_size / sizeof(uint16_t);
    ESP_LOGI("SD", "Arquivo tem %d bytes, %d amostras ECG", (int)file_size, total_samples);

    uint16_t buffer[ECG_BUFFER_SIZE];
    int samples_read = 0;
    
    while(samples_read < total_samples) {
        int samples_to_read = (total_samples - samples_read) > ECG_BUFFER_SIZE ? 
                              ECG_BUFFER_SIZE : (total_samples - samples_read);
        
        size_t read_count = fread(buffer, sizeof(uint16_t), samples_to_read, f);
        
        if(read_count > 0) {
            ESP_LOGI("SD", "Lidas %d amostras:", (int)read_count);
            
            // Imprimir as amostras (ou processar como necessário)
            for(int i = 0; i < read_count; i++) {
                printf("Amostra %d: %u\n", samples_read + i, buffer[i]);
            }
            
            samples_read += read_count;
        } else {
            ESP_LOGE("SD", "Erro na leitura ou fim do arquivo");
            break;
        }
    }
    
    fclose(f);
    ESP_LOGI("SD", "Leitura do arquivo concluída");
}

void sd_config(void) {

    // NOTA: A fila ECG agora é criada no main.c para evitar problemas de sincronização
    
    // Inicializar mutex para proteção do nome do arquivo
    if (filename_mutex == NULL) {
        filename_mutex = xSemaphoreCreateMutex();
        if (filename_mutex == NULL) {
            ESP_LOGE("SD", "Falha ao criar mutex para nome do arquivo");
            return;
        }
        ESP_LOGI("SD", "Mutex para nome do arquivo criado com sucesso");
    }
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    ESP_LOGI("SD", "Inicializando SD Card");

    sdmmc_host_t sd_host = SDSPI_HOST_DEFAULT();
    sd_host.max_freq_khz = 400;

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

    ESP_LOGI("SD", "Cartão SD inicializado com sucesso");
}

uint8_t get_next_file_number(void) {
    char test_filename[64];
    uint8_t file_num = 1;
    FILE *test_file;
    
    // Procurar o próximo número disponível
    while(file_num < 100) {  // Limite de segurança
        snprintf(test_filename, sizeof(test_filename), MOUNT_POINT"/ecg_%d.bin", file_num);
        
        test_file = fopen(test_filename, "r");
        if(test_file == NULL) {
            // Arquivo não existe, usar este número
            ESP_LOGI("SD", "Próximo arquivo disponível: ecg_%d.bin", file_num);
            return file_num;
        } else {
            fclose(test_file);
            file_num++;
        }
    }
    
    ESP_LOGW("SD", "Muitos arquivos! Usando ecg_1.bin (sobrescrever)");
    return 1;
}

const char* get_current_filename(void) {
    static char local_filename[64];
    
    // Se o mutex não foi inicializado, retornar string vazia
    if (filename_mutex == NULL) {
        ESP_LOGW("SD", "Mutex não inicializado, retornando nome vazio");
        return "";
    }
    
    // Adquirir mutex com timeout de 100ms
    if (xSemaphoreTake(filename_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Copiar o nome do arquivo para variável local thread-safe
        strncpy(local_filename, current_filename, sizeof(local_filename) - 1);
        local_filename[sizeof(local_filename) - 1] = '\0';
        
        // Liberar mutex
        xSemaphoreGive(filename_mutex);
        
        return local_filename;
    } else {
        ESP_LOGE("SD", "Timeout ao tentar acessar nome do arquivo");
        return "timeout_error";
    }
}

void sd_task(void *pvParameters) {
    ESP_LOGI("SD", "INICIANDO TASK SD");

    int16_t temp[ECG_BUFFER_SIZE];
    uint8_t file_number = get_next_file_number();
    char filename[64];
    snprintf(filename, sizeof(filename), MOUNT_POINT"/ecg_%d.bin", file_number);
    
    // Armazenar o nome do arquivo na variável global (thread-safe)
    if (filename_mutex != NULL) {
        if (xSemaphoreTake(filename_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            strncpy(current_filename, filename, sizeof(current_filename) - 1);
            current_filename[sizeof(current_filename) - 1] = '\0';  // Garantir terminação null
            xSemaphoreGive(filename_mutex);
            ESP_LOGI("SD", "Nome do arquivo atualizado com sucesso: %s", filename);
        } else {
            ESP_LOGE("SD", "Timeout ao tentar definir nome do arquivo");
        }
    } else {
        ESP_LOGE("SD", "Mutex não inicializado, não foi possível definir nome do arquivo");
    }
    
    ESP_LOGI("SD", "Gravando em: %s", filename);
    
    while(1) {
        if(xQueueReceive(ecg_buffer_queue, temp, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI("SD", "DADOS RECEBIDOS DA FILA");
            
            // Verificar se os valores estão em uma faixa razoável antes de gravar
            bool valid_data = true;
            for(int i = 0; i < ECG_BUFFER_SIZE; i++) {
                if(temp[i] < -5000 || temp[i] > 32000) {
                    ESP_LOGW("SD", "Valor suspeito detectado: %d", temp[i]);
                    valid_data = false;
                    break;
                }
            }
            
            if(valid_data) {
                write_file(filename, temp);
            } else {
                ESP_LOGE("SD", "Buffer com dados inválidos descartado");
            }
        }
    }
}