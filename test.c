#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_BUTTONS 8
#define SEQUENCE_LENGTH 10

// GPIO pins for the buttons (modify according to your wiring)
const uint button_pins[NUM_BUTTONS] = {2, 3, 4, 5, 6, 7, 8, 9};

// Function to initialize the GPIO pins for buttons
void init_buttons() {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        gpio_init(button_pins[i]);
        gpio_set_dir(button_pins[i], GPIO_IN);
        gpio_pull_up(button_pins[i]);
    }
}

// Function to read the button states (returns the button index or -1 if no button pressed)
int read_button() {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (!gpio_get(button_pins[i])) {  // Button pressed (active low)
            return i;
        }
    }
    return -1;  // No button pressed
}

// Function to generate a random sequence
void generate_sequence(int sequence[], int length) {
    for (int i = 0; i < length; i++) {
        sequence[i] = rand() % NUM_BUTTONS;  // Random button press
    }
}

// Function to play the sequence (light up the buttons)
void play_sequence(int sequence[], int length) {
    for (int i = 0; i < length; i++) {
        printf("Button %d\n", sequence[i] + 1);  // Simulate LED indicator or serial print
        sleep_ms(500);
    }
}

// Function to get player input and check if it matches the sequence
bool check_player_input(int sequence[], int length) {
    for (int i = 0; i < length; i++) {
        int button_pressed = -1;
        while (button_pressed == -1) {
            button_pressed = read_button();
            sleep_ms(50);  // debounce delay
        }

        if (button_pressed != sequence[i]) {
            return false;  // Incorrect input
        }
    }
    return true;  // Correct input
}

int main() {
    stdio_init_all();
    srand(time(NULL));  // Initialize random seed

    init_buttons();

    int sequence[SEQUENCE_LENGTH];
    int sequence_length = 1;  // Start with a sequence length of 1

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

    return 0;
}
