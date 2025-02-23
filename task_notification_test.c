
#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED_PIN  12
#define BTN_PIN  22

TaskHandle_t blink_task_handle = NULL;
TaskHandle_t btn_task_handle = NULL;

void blink_task(void *pvParameters) {
    while (1) {
        gpio_put(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_put(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void btn_isr_callback(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(btn_task_handle, &xHigherPriorityTaskWoken);
    gpio_set_irq_enabled(BTN_PIN, GPIO_IRQ_EDGE_FALL, false); // Disable interruption after first press
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void btn_task(void *pvParameters) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        printf("Button pressed! Stopping blink task.\n");
        vTaskDelete(blink_task_handle);
        printf("Program terminated.\n");
        vTaskDelete(NULL);
    }
}

int main() {
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);
    
    xTaskCreate(blink_task, "BlinkTask", 256, NULL, 1, &blink_task_handle);
    xTaskCreate(btn_task, "BtnTask", 256, NULL, 1, &btn_task_handle);
    
    gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_FALL, true, &btn_isr_callback);
    
    vTaskStartScheduler();
    while (1);
}
