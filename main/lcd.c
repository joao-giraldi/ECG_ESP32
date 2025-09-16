#include "lcd.h"

extern i2c_dev_t device;
esp_lcd_panel_io_handle_t io_handle = NULL;

esp_err_t lcd_config(void) {
    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_IO_I2C_SH1107_CONFIG();

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle)config->i2c.port, &io_config, &io_handle));

    esp_lcd_panel_handle_t lcd_panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_sh1107(io_handle, &panel_config, &lcd_panel_handle));
    ESP_ERROR_CHECK(panel_sh1107_reset(lcd_panel_handle));
    ESP_ERROR_CHECK(panel_sh1107_init(lcd_panel_handle));
    ESP_ERROR_CHECK(panel_sh1107_disp_on_off(lcd_panel_handle, true));

    return ESP_OK;
}