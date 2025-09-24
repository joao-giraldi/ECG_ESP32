#include "lcd.h"

esp_err_t lcd_config(void) {
    init_ssd1306();
    ESP_ERROR_CHECK(i2c_ssd1306_buffer_clear(&i2c_ssd1306));
    return ESP_OK;
}

void lcd_display_welcome(void) {
    i2c_ssd1306_buffer_clear(&i2c_ssd1306);
    ssd1306_print_str(24, 5, "ECG Monitor", false);
    ssd1306_print_str(25, 35, "Bem-vindo!", false);
    ssd1306_print_str(15, 45, "Pressione o", false);
    ssd1306_print_str(5, 55, "botao de coleta", false);
    ssd1306_display();
}

void lcd_display_collecting(uint32_t elapsed_time) {
    char buffer[32];

    i2c_ssd1306_buffer_clear(&i2c_ssd1306);
    ssd1306_print_str(35, 5, "COLETANDO", false);

    uint32_t minutes = elapsed_time / 60;
    uint32_t seconds = elapsed_time % 60;
    snprintf(buffer, sizeof(buffer), "Tempo: %02lu:%02lu", minutes, seconds);
    ssd1306_print_str(5, 35, buffer, false);

    ssd1306_print_str(10, 50, "Pressione STOP", false);
    ssd1306_display();
}

void lcd_display_stopped(uint32_t total_time) {

}

void lcd_display_network_ready(void) {
    i2c_ssd1306_buffer_clear(&i2c_ssd1306);
    ssd1306_print_str(45, 5, "PRONTO", false);
    ssd1306_print_str(2, 35, "Rede: Ativa", false);
    ssd1306_print_str(2, 45, "http://ecg.local", false);
    ssd1306_display();
}