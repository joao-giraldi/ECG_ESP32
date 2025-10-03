#include "lcd.h"

static const char *TAG = "LCD";

#define I2C_HOST  0

bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_t * disp = (lv_disp_t *)user_ctx;
    lvgl_port_flush_ready(disp);
    return false;
}

void example_lvgl_demo_ui(lv_disp_t *disp)
{
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR); /* Circular scroll */
    lv_label_set_text(label, "INNOVATE YOURSELF - ASHISH SAINI");

    /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
    lv_obj_set_width(label, disp->driver->hor_res);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 30);
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

}

void lcd_display_stopped(uint32_t total_time) {

}

void lcd_display_network_ready(void) {

}