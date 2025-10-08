#include "sys_error.h"

void err_handler(const char* err_point) {
    ESP_EARLY_LOGE("ERROR", "Erro crítico detectado: %c", err_point);

    portDISABLE_INTERRUPTS();

    extern TaskHandle_t ecg_task_handle;
    extern TaskHandle_t sd_task_handle;
    
    if (ecg_task_handle != NULL) {
        vTaskSuspend(ecg_task_handle);
    }
    if (sd_task_handle != NULL) {
        vTaskSuspend(sd_task_handle);
    }

    lcd_display_error(err_point);
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();
}