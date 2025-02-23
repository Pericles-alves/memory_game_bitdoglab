#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "limits.h"

#define JOY_X_PIN 26  // GPIO26 (ADC0)
#define JOY_Y_PIN 27  // GPIO27 (ADC1)
// Define thresholds for directions, and the dead_zones
#define ADC_MAX_RES         4096  // ADC 12 bit resolution value
#define MAX_NUM_DIVISIONS   40    // Number of possible positions for the joystick
#define DEADZONE            7     // Joystick dead zone
#define CENTER_THRESHOLD    ((2048*MAX_NUM_DIVISIONS)/ADC_MAX_RES)  // ADC midpoint (adjustable)

#define TIMER_PERIOD_MS      100

typedef enum {
    CENTER,
    UP, DOWN, LEFT, RIGHT,
    LEFT_UP, LEFT_DOWN, RIGHT_UP, RIGHT_DOWN
} JoystickDirection;

volatile JoystickDirection last_direction = CENTER;
TaskHandle_t joystick_task_handle = NULL;
TimerHandle_t joystick_timer = NULL;

void joystick_timer_callback(TimerHandle_t xTimer) {
    // Read joystick ADC values
    uint16_t x_value = adc_read();  // Read X-axis
    x_value = (x_value*MAX_NUM_DIVISIONS/ADC_MAX_RES);
    adc_select_input(1);
    uint16_t y_value = adc_read();  // Read Y-axis
    y_value = (y_value*MAX_NUM_DIVISIONS/ADC_MAX_RES);
    adc_select_input(0); // Reset back to X-axis
    JoystickDirection new_direction = CENTER;

    if (x_value > CENTER_THRESHOLD + DEADZONE) {
        if (y_value > CENTER_THRESHOLD + DEADZONE)
            new_direction = RIGHT_UP;
        else if (y_value < CENTER_THRESHOLD - DEADZONE)
            new_direction = RIGHT_DOWN;
        else
            new_direction = RIGHT;
    } 
    else if (x_value < CENTER_THRESHOLD - DEADZONE) {
        if (y_value > CENTER_THRESHOLD + DEADZONE)
            new_direction = LEFT_UP;
        else if (y_value < CENTER_THRESHOLD - DEADZONE)
            new_direction = LEFT_DOWN;
        else
            new_direction = LEFT;
    } 
    else {
        if (y_value > CENTER_THRESHOLD + DEADZONE)
            new_direction = UP;
        else if (y_value < CENTER_THRESHOLD - DEADZONE)
            new_direction = DOWN;
    }

    if (new_direction != last_direction) {
        last_direction = new_direction;
        printf("New direction: %d\n", new_direction);
        xTaskNotify(joystick_task_handle, new_direction, eSetValueWithOverwrite);
    }
}

void joystick_task(void *pvParameters) {
    uint32_t notification_value;
    while (1) {
        if (xTaskNotifyWait(0, ULONG_MAX, &notification_value, portMAX_DELAY) == pdTRUE) {
            printf("Handling joystick direction: %d\n", notification_value);
            // Perform action based on joystick movement
        }
    }
}

void joystick_init(){
    adc_init();
    adc_gpio_init(JOY_X_PIN);
    adc_gpio_init(JOY_Y_PIN);
}

int main() {
    stdio_init_all();
    
    joystick_init();
    xTaskCreate(joystick_task, "JoystickTask", 256, NULL, 1, &joystick_task_handle);
    joystick_timer = xTimerCreate("JoystickTimer", pdMS_TO_TICKS(TIMER_PERIOD_MS), pdTRUE, NULL, joystick_timer_callback);

    if (joystick_timer != NULL) {
        xTimerStart(joystick_timer, 0);
    }

    vTaskStartScheduler();

    while (1) {
        tight_loop_contents();
    }
}