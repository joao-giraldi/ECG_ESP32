#include "microSD.h"
#include "freertos/semphr.h"

sdmmc_card_t* sd_card;
const char mount_point[] = MOUNT_POINT;

// Variável global para armazenar o nome do arquivo atual
static char current_filename[64] = "";

// Mutex para proteger acesso à variável current_filename
static SemaphoreHandle_t filename_mutex = NULL;

// Variáveis estáticas para controle do arquivo
static FILE *current_file = NULL;
static uint8_t write_count = 0;

extern QueueHandle_t ecg_buffer_queue;
extern TaskHandle_t ecg_handler;

esp_err_t sd_config(void) { 
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
        .allocation_unit_size = 16 * 1024,
        .format_if_mount_failed = false,
        .disk_status_check_enable = true
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

    ESP_ERROR_CHECK(spi_bus_initialize(sd_host.slot, &bus_conf, SDSPI_DEFAULT_DMA));

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PORT;
    slot_config.host_id = sd_host.slot;

    ESP_LOGI("ESP", "Montando sistema de arquivos");

    ESP_ERROR_CHECK(esp_vfs_fat_sdspi_mount(mount_point, &sd_host, &slot_config, &mount_config, &sd_card));

    ESP_LOGI("SD", "Sistema de arquivos montado");

    //sdmmc_card_print_info(stdout, sd_card);

    ESP_LOGI("SD", "Cartão SD inicializado com sucesso");
    return ESP_OK;
}

void write_file(const char *path, int16_t *data) {
    ESP_LOGI("SD", "Abrindo arquivo para APPEND BINÁRIO");

    if(current_file == NULL) {
        current_file = fopen(path, "ab");
        if(current_file == NULL) {
            ESP_LOGE("SD", "Falha ao abrir o arquivo");
            return;
        }
    }

    fflush(current_file);

    size_t written = fwrite(data, sizeof(uint16_t), ECG_BUFFER_SIZE, current_file);
    
    if(written != ECG_BUFFER_SIZE) {
        ESP_LOGE("SD", "Erro na escrita: %d/%d", written, ECG_BUFFER_SIZE);
    }

    fflush(current_file);
        
    write_count++;

    if(write_count % 2 == 0) {
        fclose(current_file);
        current_file = NULL;  // Será reaberto na próxima escrita
        write_count = 0;
        ESP_LOGI("SD", "Arquivo fechado e reaberto para segurança");
    }
}

void finalize_current_file(void) {
    if(current_file != NULL) {
        ESP_LOGI("SD", "Finalizando arquivo atual - forçando flush e fechamento");
        fflush(current_file);
        fclose(current_file);
        current_file = NULL;
        write_count = 0;
        ESP_LOGI("SD", "Arquivo finalizado com sucesso");
    } else {
        ESP_LOGI("SD", "Nenhum arquivo aberto para finalizar");
    }

    uint8_t file_number = get_next_file_number();
    char new_filename[64];
    snprintf(new_filename, sizeof(new_filename), MOUNT_POINT"/ECG_%d.BIN", file_number);
    
    // Atualizar o nome do arquivo global (thread-safe)
    if (filename_mutex != NULL) {
        if (xSemaphoreTake(filename_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            strncpy(current_filename, new_filename, sizeof(current_filename) - 1);
            current_filename[sizeof(current_filename) - 1] = '\0';
            xSemaphoreGive(filename_mutex);
            ESP_LOGI("SD", "Arquivo avançado para: %s", new_filename);
        } else {
            ESP_LOGE("SD", "Timeout ao tentar atualizar nome do arquivo");
        }
    } else {
        ESP_LOGE("SD", "Mutex não inicializado");
    }
}

uint8_t get_next_file_number(void) {
    static uint8_t last_used = 0;
    const char* counter_file = MOUNT_POINT"/COUNTER.TXT";
    FILE* f = NULL;
    
    // Tenta ler o último número usado
    f = fopen(counter_file, "r");
    if(f != NULL) {
        int temp_value = 0;
        if(fscanf(f, "%d", &temp_value) == 1) {
            last_used = (uint8_t)temp_value;
        } else {
            ESP_LOGW("SD", "Erro ao ler conteúdo do arquivo contador");
            last_used = 0;
        }
        fclose(f);
        ESP_LOGI("SD", "Último número usado lido do arquivo: %d", last_used);
    } else {
        ESP_LOGI("SD", "Arquivo contador não existe, iniciando do 1");
        last_used = 0;
    }
    
    // Incrementar para o próximo número (circular: 1-50)
    last_used++;
    if(last_used > MAX_FILES) {
        last_used = 1;
    }

    ESP_LOGI("SD", "Tentando salvar novo número: %d", last_used);
    
    f = fopen(counter_file, "w");
    if(f != NULL) {
        if(fprintf(f, "%d\n", last_used) > 0) {
            fflush(f);
            fclose(f);
            ESP_LOGI("SD", "Novo número salvo com sucesso: %d", last_used);
        } else {
            fclose(f);
            ESP_LOGE("SD", "Erro ao escrever no arquivo contador");
        }
    } else {
        ESP_LOGE("SD", "Erro ao abrir arquivo contador para escrita: %s (errno: %d)", strerror(errno), errno);
    }
    
    ESP_LOGI("SD", "Próximo arquivo (circular): ecg_%d.bin", last_used);
    return last_used;
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
    snprintf(filename, sizeof(filename), MOUNT_POINT"/ECG_%d.BIN", file_number);
    
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
        // Usar timeout de 100ms em vez de portMAX_DELAY para permitir paradas mais rápidas
        if(xQueueReceive(ecg_buffer_queue, temp, pdMS_TO_TICKS(100)) == pdTRUE) {
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
                // Usar nome de arquivo atual (thread-safe)
                const char* current_file = get_current_filename();
                write_file(current_file, temp);
            } else {
                ESP_LOGE("SD", "Buffer com dados inválidos descartado");
            }
        }
    }
}