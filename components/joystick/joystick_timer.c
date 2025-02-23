#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/timer.h"

// Define ADC channels
#define JOY_X_PIN 26  // GPIO26 (ADC0)
#define JOY_Y_PIN 27  // GPIO27 (ADC1)

// Define thresholds for directions, and the dead_zones
#define ADC_MAX_RES         4096  // ADC 12 bit resolution value
#define MAX_NUM_DIVISIONS   40    // Number of possible positions for the joystick
#define DEADZONE            7     // Joystick dead zone
#define CENTER_THRESHOLD    ((2048*MAX_NUM_DIVISIONS)/ADC_MAX_RES)  // ADC midpoint (adjustable)

// Timer interval in microseconds
#define TIMER_INTERVAL_US 100000

// Possible joystick directions
typedef enum {
    CENTER,
    UP, DOWN, LEFT, RIGHT,
    LEFT_UP, LEFT_DOWN, RIGHT_UP, RIGHT_DOWN
} JoystickDirection;

// Last detected direction
volatile JoystickDirection last_direction = CENTER;

// Timer callback function
bool joystick_timer_callback(struct repeating_timer *t) {
    // Read joystick ADC values
    uint16_t x_value = adc_read();  // Read X-axis
    x_value = (x_value*MAX_NUM_DIVISIONS/ADC_MAX_RES);
    adc_select_input(1);
    uint16_t y_value = adc_read();  // Read Y-axis
    y_value = (y_value*MAX_NUM_DIVISIONS/ADC_MAX_RES);
    adc_select_input(0); // Reset back to X-axis

    // Determine direction based on ADC values
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

    // If direction changed, trigger an action
    if (new_direction != last_direction) {
        last_direction = new_direction;
        printf("New direction: %d\n", new_direction);  // Replace with your action
    }

    return true; // Continue repeating the timer
}

int main() {
    stdio_init_all();
    
    // Initialize ADC
    adc_init();
    adc_gpio_init(JOY_X_PIN);
    adc_gpio_init(JOY_Y_PIN);
    adc_select_input(0);

    // Initialize a repeating timer
    struct repeating_timer timer;
    add_repeating_timer_us(-TIMER_INTERVAL_US, joystick_timer_callback, NULL, &timer);

    // Main loop (can be empty if timer handles everything)
    while (1) {
        tight_loop_contents();
    }

    return 0;
}
