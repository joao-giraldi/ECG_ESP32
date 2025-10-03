#include "lcd.h"
#include "wifi.h"

static const char *TAG = "LCD";

#define I2C_HOST  0

bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_t * disp = (lv_disp_t *)user_ctx;
    lvgl_port_flush_ready(disp);
    return false;
}

void lcd_config(void) {
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = EXAMPLE_I2C_HW_ADDR,
        .control_phase_bytes = 1,               // According to SSD1306 datasheet
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,   // According to SSD1306 datasheet
        .lcd_param_bits = EXAMPLE_LCD_CMD_BITS, // According to SSD1306 datasheet
        .dc_bit_offset = 6,                     // According to SSD1306 datasheet
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES,
        .double_buffer = true,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        }
    };
    lv_disp_t * disp = lvgl_port_add_disp(&disp_cfg);

    /* Register done callback for IO */
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };
    esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, disp);

    /* Rotation of the screen */
    lv_disp_set_rotation(disp, LV_DISP_ROT_180);

    ESP_LOGI(TAG, "Display LVGL Scroll Text");
    //example_lvgl_demo_ui(disp);
}

void boot_image(void){
    ESP_LOGI(TAG, "Imagem de Boot");

    LV_IMG_DECLARE(logo_coracao);
    
    // Obter o display ativo
    lv_disp_t * disp = lv_disp_get_default();
    if (disp == NULL) {
        ESP_LOGE(TAG, "Display não encontrado");
        return;
    }

    // Obter a tela ativa
    lv_obj_t * scr = lv_disp_get_scr_act(disp);

    // Limpar a tela
    lv_obj_clean(scr);

    // Configuração da imagem
    lv_obj_t * img = lv_img_create(scr);
    lv_img_set_src(img, &logo_coracao);
    
    // Vamos redimensionar usando zoom
    lv_obj_align(img, LV_ALIGN_CENTER, 0, -10);

    // Adicionar título abaixo da imagem
    lv_obj_t * title = lv_label_create(scr);
    lv_label_set_text(title, "ECG Monitor");
    lv_obj_align(title, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);

    // Forçar atualização da tela
    lv_refr_now(disp);
    
    ESP_LOGI(TAG, "Boot image displayed successfully");
}

void lcd_display_welcome(void) {
    ESP_LOGI(TAG, "Exibindo tela de boas-vindas");

    // Obter o display ativo
    lv_disp_t * disp = lv_disp_get_default();
    if (disp == NULL) {
        ESP_LOGE(TAG, "Display não encontrado");
        return;
    }

    // Obter a tela ativa
    lv_obj_t * scr = lv_disp_get_scr_act(disp);
    if (scr == NULL) {
        ESP_LOGE(TAG, "Tela não encontrada");
        return;
    }

    // Limpar a tela
    lv_obj_clean(scr);

    // Instrução para o usuário - usando um único label com quebra de linha
    lv_obj_t * instruction = lv_label_create(scr);
    if (instruction != NULL) {
        lv_label_set_text(instruction, "Pressione o botao de coleta");
        lv_obj_set_style_text_align(instruction, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(instruction, LV_ALIGN_CENTER, 0, 0);
        
        // Configurar para permitir múltiplas linhas
        lv_label_set_long_mode(instruction, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(instruction, 120);
    }

    // Forçar atualização da tela
    lv_refr_now(disp);
    
    ESP_LOGI(TAG, "Tela de boas-vindas exibida com sucesso");
}

void lcd_display_collecting(uint32_t elapsed_time) {
    // Obter o display ativo
    lv_disp_t * disp = lv_disp_get_default();
    if (disp == NULL) {
        ESP_LOGE(TAG, "Display não encontrado");
        return;
    }

    // Obter a tela ativa
    lv_obj_t * scr = lv_disp_get_scr_act(disp);
    if (scr == NULL) {
        ESP_LOGE(TAG, "Tela não encontrada");
        return;
    }

    // Limpar a tela
    lv_obj_clean(scr);

    uint32_t minutes = elapsed_time / 60;
    uint32_t seconds = elapsed_time % 60;

    // Título - COLETANDO
    lv_obj_t * title = lv_label_create(scr);
    if (title != NULL) {
        lv_label_set_text(title, "COLETANDO. . .");
        lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);
    }

    // Exibição do tempo formatado
    lv_obj_t * time_display = lv_label_create(scr);
    if (time_display != NULL) {
        // Criar string formatada para o tempo
        char time_str[32];
        snprintf(time_str, sizeof(time_str), "Tempo: %02lu:%02lu", (unsigned long)minutes, (unsigned long)seconds);
        
        lv_label_set_text(time_display, time_str);
        lv_obj_set_style_text_align(time_display, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(time_display, LV_ALIGN_CENTER, 0, 0);
    }
    
    // Forçar atualização da tela
    lv_refr_now(disp);
}

void lcd_display_stopped(uint32_t total_time, const char* filename) {
    ESP_LOGI(TAG, "Exibindo informações de coleta finalizada");

    // Obter o display ativo
    lv_disp_t * disp = lv_disp_get_default();
    if (disp == NULL) {
        ESP_LOGE(TAG, "Display não encontrado");
        return;
    }

    // Obter a tela ativa
    lv_obj_t * scr = lv_disp_get_scr_act(disp);
    if (scr == NULL) {
        ESP_LOGE(TAG, "Tela não encontrada");
        return;
    }

    // Limpar a tela
    lv_obj_clean(scr);

    // Calcular minutos e segundos do tempo total
    uint32_t minutes = total_time / 60;
    uint32_t seconds = total_time % 60;

    // Título - COLETA FINALIZADA
    lv_obj_t * title = lv_label_create(scr);
    if (title != NULL) {
        lv_label_set_text(title, "CONCLUIDO");
        lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);
    }

    // Exibição do tempo total formatado
    lv_obj_t * time_display = lv_label_create(scr);
    if (time_display != NULL) {
        char time_str[32];
        snprintf(time_str, sizeof(time_str), "Tempo: %02lu:%02lu", (unsigned long)minutes, (unsigned long)seconds);
        
        lv_label_set_text(time_display, time_str);
        lv_obj_set_style_text_align(time_display, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(time_display, LV_ALIGN_CENTER, 0, 0);
    }

    // Exibição do nome do arquivo
    lv_obj_t * file_display = lv_label_create(scr);
    if (file_display != NULL) {
        char file_str[64];
        char clean_filename[32];
        
        // Extrair apenas o nome do arquivo (sem o caminho completo)
        const char* file_name = strrchr(filename, '/');
        if (file_name) {
            file_name++;  // Pular o '/'
        } else {
            file_name = filename;  // Usar o nome completo se não houver '/'
        }
        
        // Remover a extensão .bin
        strncpy(clean_filename, file_name, sizeof(clean_filename) - 1);
        clean_filename[sizeof(clean_filename) - 1] = '\0';
        
        // Deixar apenas o nome do arquivo
        char* dot_pos = strrchr(clean_filename, '.');
        if (dot_pos != NULL) {
            *dot_pos = '\0';
        }
        
        snprintf(file_str, sizeof(file_str), "Arquivo: %s", clean_filename);
        
        lv_label_set_text(file_display, file_str);
        lv_obj_set_style_text_align(file_display, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(file_display, LV_ALIGN_CENTER, 0, 15);
    }

    // Forçar atualização da tela
    lv_refr_now(disp);
    
    ESP_LOGI(TAG, "Tela de coleta finalizada exibida");
}

void lcd_display_network(void) {
    ESP_LOGI(TAG, "Exibindo informações de rede WiFi");

    // Obter o display ativo
    lv_disp_t * disp = lv_disp_get_default();
    if (disp == NULL) {
        ESP_LOGE(TAG, "Display não encontrado");
        return;
    }

    // Obter a tela ativa
    lv_obj_t * scr = lv_disp_get_scr_act(disp);
    if (scr == NULL) {
        ESP_LOGE(TAG, "Tela não encontrada");
        return;
    }

    // Limpar a tela
    lv_obj_clean(scr);

    lv_obj_t * title = lv_label_create(scr);
    if (title != NULL) {
        lv_label_set_text(title, "REDE WIFI");
        lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);
    }

    lv_obj_t * wifi_name = lv_label_create(scr);
    if (wifi_name != NULL) {
        lv_label_set_text(wifi_name, "Nome: " WIFI_SSID);
        lv_obj_set_style_text_align(wifi_name, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(wifi_name, LV_ALIGN_CENTER, 0, 0);
    }

    lv_obj_t * wifi_pass = lv_label_create(scr);
    if (wifi_pass != NULL) {
        lv_label_set_text(wifi_pass, "Senha: " WIFI_PASS);
        lv_obj_set_style_text_align(wifi_pass, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(wifi_pass, LV_ALIGN_CENTER, 0, 15);
    }

    // Forçar atualização da tela
    lv_refr_now(disp);
    
    ESP_LOGI(TAG, "Informações de rede exibidas");
}

void lcd_display_server(void) {
    ESP_LOGI(TAG, "Exibindo informações do servidor web");

    // Obter o display ativo
    lv_disp_t * disp = lv_disp_get_default();
    if (disp == NULL) {
        ESP_LOGE(TAG, "Display não encontrado");
        return;
    }

    // Obter a tela ativa
    lv_obj_t * scr = lv_disp_get_scr_act(disp);
    if (scr == NULL) {
        ESP_LOGE(TAG, "Tela não encontrada");
        return;
    }

    // Limpar a tela
    lv_obj_clean(scr);

    lv_obj_t * title = lv_label_create(scr);
    if (title != NULL) {
        lv_label_set_text(title, "SITE");
        lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);
    }

    lv_obj_t * access = lv_label_create(scr);
    if (access != NULL) {
        lv_label_set_text(access, "Acesse:");
        lv_obj_set_style_text_align(access, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(access, LV_ALIGN_CENTER, 0, 0);
    }

    lv_obj_t * url = lv_label_create(scr);
    if (url != NULL) {
        lv_label_set_text(url, "http://ecg.local");
        lv_obj_set_style_text_align(url, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(url, LV_ALIGN_CENTER, 0, 15);
    }

    // Forçar atualização da tela
    lv_refr_now(disp);
    
    ESP_LOGI(TAG, "Informações do servidor exibidas");
}
