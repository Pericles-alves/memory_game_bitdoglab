#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Define ADC channels
#define JOY_X_PIN 26  // GPIO26 (ADC0)
#define JOY_Y_PIN 27  // GPIO27 (ADC1)

// Define thresholds for directions, and the dead zones
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

// Joystick direction as last detected
volatile JoystickDirection last_direction = CENTER;

// Function to read joystick ADC values
JoystickDirection read_joystick() {
    // Read joystick ADC values
    uint16_t x_value = adc_read();  // Read X-axis
    x_value = (x_value * MAX_NUM_DIVISIONS / ADC_MAX_RES);
    adc_select_input(1);
    uint16_t y_value = adc_read();  // Read Y-axis
    y_value = (y_value * MAX_NUM_DIVISIONS / ADC_MAX_RES);
    adc_select_input(0);  // Reset back to X-axis

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

    return new_direction;
}

void displayArrowIndex(uint i) {
    npSetLEDs(arrow_positions[i], arrow_size, arrow_colors[i]);
}

void init_buttons() {
    // No need to initialize physical buttons for joystick-based input
}

// Function to generate a random sequence
void generate_sequence(int sequence[], int length) {
    for (int i = 0; i < length; i++) {
        sequence[i] = rand() % NUM_BUTTONS;  // Random joystick direction
    }
}

// Function to play the sequence (light up the arrows for joystick directions)
void play_sequence(int sequence[], int length) {
    for (uint i = 0; i < length; i++) {
        printf("Joystick direction %d\n", sequence[i]);  // Simulate LED indicator or serial print
        displayArrowIndex(sequence[i]);
        sleep_ms(500);
        npClear();
        npWrite();
        sleep_ms(500);
    }
}

// Function to get player input and check if it matches the sequence
bool check_player_input(int sequence[], int length) {
    for (int i = 0; i < length; i++) {
        JoystickDirection joystick_input = read_joystick();  // Read the joystick input

        // Wait until the player makes a move
        while (joystick_input == CENTER) {
            joystick_input = read_joystick();  // Read joystick input
            sleep_ms(50);  // debounce delay
        }

        if (joystick_input != sequence[i]) {
            return false;  // Incorrect input
        }
    }
    return true;  // Correct input
}

int main() {
    stdio_init_all();
    srand(time(NULL));  // Initialize random seed

    int sequence[SEQUENCE_LENGTH];
    int sequence_length = 10;  // Start with a sequence length of 1

    // Initialize joystick ADC pins
    adc_init();
    adc_gpio_init(JOY_X_PIN);
    adc_gpio_init(JOY_Y_PIN);
    adc_select_input(0);

    // Initialize NeoPixel LED matrix
    npInit(LED_PIN);
    npClear();

    while (1) {
        printf("Sequence length: %d\n", sequence_length);

        // Generate and play the sequence
        generate_sequence(sequence, sequence_length);
        play_sequence(sequence, sequence_length);

        // Wait for user input and check if it matches the sequence
        if (check_player_input(sequence, sequence_length)) {
            printf("Correct!\n");
            sequence_length++;  // Increase the sequence length
        } else {
            printf("Incorrect! Game over.\n");
            sequence_length = 1;  // Reset the game
        }

        sleep_ms(1000);  // Delay between rounds
    }
}
