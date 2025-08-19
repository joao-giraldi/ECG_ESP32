#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "spi_flash_mmap.h"
#include "esp_adc/adc_continuous.h"
#include "driver/gptimer.h"
#include "freertos/queue.h"

#include "ecg.h"

void config_ports(void) {
    gpio_set_direction(18, GPIO_MODE_INPUT);
    gpio_set_direction(19, GPIO_MODE_INPUT);
}

void app_main(void) {
    config_ports();

    adc_config();

    while(1) {
        adc_read();
    }
}

/*
# define TIMER_INTR_SEL TIMER_INTR_LEVEL
# define TIMER_GROUP TIMER_GROUP_0
# define TIMER_DIVIDER 80
# define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER)
# define TIMER_FINE_ADJ (0*(TIMER_BASE_CLK/TIMER_DIVIDER) /1000000)
# define TIMER_INTERVAL0_SEC (0.01)

QueueHandle_t queue;

void IRAM_ATTR timer_group0_isr(void *p) {
    int timer_idx = (int) p;
    int value;
    esp_err_t r = ESP_OK;
    uint32_t intr_status = TIMERGO.int_st_timer.val;
    BaseType_t priorityFlag = pdFALSE;

    if((intr_status & BIT(timer_idx)) && timer_idx == TIMER_0) {
        TIMERGO.hw_timer[timer_idx].update = 1;
        TIMERGO.hw_timer[timer_idx].config.alarm_en = 1;

        if(gpio_get_level(19) == 1 || gpio_get_level(18) == 1) {
            value = 0;
        } else {
            r = adc2_get_raw(ADC_CHANNEL_0, ADC_BITWIDTH_12, &value); //FAZER O CONTÍNUO
        }

        if(r == ESP_OK) xQueueSendFromISR(queue, &value, &priorityFlag);
    }
}

void timer0_init() {
    int timer_group = TIMER_GROUP_0;
    int timer_idx = TIMER_0;
    timer_config_t config;

    config.alarm_en = 1;
    config.auto_reload = 1;
    config.counter_dir = TIMER_COUNT_UP;
    config.divider = TIMER_DIVIDER;
    config.intr_type = TIMER_INTR_SEL;
    config.counter_en = TIMER_PAUSE;
    timer_init (timer_group ,timer_idx ,&config);
    timer_pause(timer_group, timer_idx);
    timer_set_counter_value(timer_group , timer_idx, 0x00000000ULL);
    timer_set_alarm_value(timer_group, timer_idx, (TIMER_INTERVAL0_SEC * TIMER_SCALE) - TIMER_FINE_ADJ);
    timer_enable_intr(timer_group, timer_idx);
    timer_isr_register(timer_group, timer_idx, timer_group0_isr, (void*) timer_idx, ESP_INTR_FLAG_IRAM, NULL );
    timer_start(timer_group , timer_idx);
}

void task_heart(void *pvParameter) {
    int value = 0;
    for(;;) {
        if(xQueueReceive(queue, &value, (TickType_t)(1000 / portTICK_PERIOD_MS)) == pdTRUE) {
            printf("%d\n", value );
        }
    }
}

void app_main(void) {
    queue = xQueueCreate(10, sizeof(int));

    gpio_set_direction(4, GPIO_MODE_OUTPUT);
    gpio_set_direction(19, GPIO_MODE_INPUT);
    gpio_set_direction(18, GPIO_MODE_INPUT);

    gpio_set_level(4, 0);

    adc2_config_channel_atten(ADC2_CHANNEL_0, ADC_ATTEN_11db);

    timer0_init();

    xTaskCreate(&task_heart, "transmitter", 2048, NULL, 5, NULL);
}
*/