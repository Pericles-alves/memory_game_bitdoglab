#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "oled_ssd1306/ssd1306_i2c.h"
#include "hardware/pwm.h"

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

#define JOYSTICK_BTN 22
#define BUZZER_PIN 21
#define BUZZER_FREQUENCY 100

volatile bool btn_pressed = false;

#define NUM_BUTTONS 9
#define SEQUENCE_LENGTH 10

// GPIO pins for the buttons (modify according to your wiring)
const uint button_pins[NUM_BUTTONS] = {2, 3, 4, 5, 6, 7, 8, 9};

// Define ADC channels
#define JOY_X_PIN 26  // GPIO26 (ADC0)
#define JOY_Y_PIN 27  // GPIO27 (ADC1)

// Define thresholds for directions, and the dead zones
#define ADC_MAX_RES         4096  // ADC 12 bit resolution value
#define MAX_NUM_DIVISIONS   40    // Number of possible positions for the joystick
#define DEADZONE            7     // Joystick dead zone
#define CENTER_THRESHOLD    ((2048*MAX_NUM_DIVISIONS)/ADC_MAX_RES)  // ADC midpoint (adjustable)

#define LED_COUNT 25
#define LED_PIN 7
#define DIM_FACTOR 0.5  // Dimming factor (range 0 to 1)

uint8_t arrow_colors[8][3] = {
  {0x08, 0x00, 0x00}, // Color 1
  {0x09, 0x08, 0x12}, // Color 2
  {0x08, 0x12, 0x06}, // Color 3
  {0x06, 0x11, 0x00}, // Color 4
  {0x08, 0x00, 0x06}, // Color 5
  {0x06, 0x10, 0x08}, // Color 6
  {0x12, 0x06, 0x08}, // Color 7
  {0x00, 0x06, 0x08}  // Color 8
};

/*
matrix:
uint8_t arrow_colors[8][3] = {
  {0x08, 0x00, 0x00}, // Color 1
  {0x09, 0x08, 0x12}, // Color 2
  {0x08, 0x12, 0x06}, // Color 3
  {0x06, 0x11, 0x00}, // Color 4
  {0x08, 0x00, 0x06}, // Color 5
  {0x06, 0x10, 0x08}, // Color 6
  {0x12, 0x06, 0x08}, // Color 7
  {0x00, 0x06, 0x08}  // Color 8
};

uint8_t arrow_colors[8][3] = {
  {255 - 0x08, 255 - 0x00, 255 - 0x00}, // Color 1
  {255 - 0x09, 255 - 0x08, 255 - 0x12}, // Color 2
  {255 - 0x08, 255 - 0x12, 255 - 0x06}, // Color 3
  {255 - 0x06, 255 - 0x11, 255 - 0x00}, // Color 4
  {255 - 0x08, 255 - 0x00, 255 - 0x06}, // Color 5
  {255 - 0x06, 255 - 0x10, 255 - 0x08}, // Color 6
  {255 - 0x12, 255 - 0x06, 255 - 0x08}, // Color 7
  {255 - 0x00, 255 - 0x06, 255 - 0x08}  // Color 8
};

*/

uint arrow_positions[8][9] = {
  {14,13,12,11,10,8,2,18,22}, // right
  {14,6,2,16,22,13,12,11,10}, // left
  {2,8,6,10,14,7,12,17,22},   // down
  {2,7,10,12,14,16,17,18,22}, // up
  {4,3,2,5,14,6,12,18,20},    // left_down
  {0,1,2,9,10,8,12,16,24},    // right down
  {0,8,12,14,15,16,24,23,22}, // left_up
  {4,6,10,12,18,19,20,21,22} // right up
};
uint num_of_arrows = 8;
uint arrow_size = 9;

// Possible joystick directions
typedef enum {
  CENTER=0, RIGHT=1, LEFT=2, DOWN=3, UP=4, LEFT_DOWN=5, RIGHT_DOWN=6, LEFT_UP=7, RIGHT_UP=8
} JoystickDirection;

// Joystick direction as last detected
volatile JoystickDirection last_direction = CENTER;
bool joystick_moved = false;  // Tracks if joystick has been moved

// Definição de pixel GRB
struct pixel_t {
  uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;


uint notes_buzzer[8]= {261, 293, 329, 349, 392, 440, 493, 523};

// Inicializa o PWM no pino do buzzer
void pwm_init_buzzer(uint pin) {
  gpio_set_function(pin, GPIO_FUNC_PWM);
  uint slice_num = pwm_gpio_to_slice_num(pin);
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, 4.0f); // Ajusta divisor de clock
  pwm_init(slice_num, &config, true);
  pwm_set_gpio_level(pin, 0); // Desliga o PWM inicialmente
}

// Toca uma nota com a frequência e duração especificadas
void play_tone(uint pin, uint frequency, uint duration_ms) {
  uint slice_num = pwm_gpio_to_slice_num(pin);
  uint32_t clock_freq = clock_get_hz(clk_sys);
  uint32_t top = clock_freq / frequency - 1;

  pwm_set_wrap(slice_num, top);
  pwm_set_gpio_level(pin, top / 2.5); // 50% de duty cycle

  sleep_ms(duration_ms);

  pwm_set_gpio_level(pin, 0); // Desliga o som após a duração
  sleep_ms(50); // Pausa entre notas
}

// Function to read joystick ADC values
JoystickDirection read_joystick() {
  adc_select_input(1);
  uint16_t x_value = adc_read();  // Read X-axis
  x_value = (x_value*MAX_NUM_DIVISIONS/ADC_MAX_RES);
  adc_select_input(0); // Reset back to X-axis
  uint16_t y_value = adc_read();  // Read Y-axis
  y_value = (y_value*MAX_NUM_DIVISIONS/ADC_MAX_RES);

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

/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInit(uint pin) {

  // Cria programa PIO.
  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;

  // Toma posse de uma máquina PIO.
  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0) {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
  }

  // Inicia programa na máquina PIO obtida.
  ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

  // Limpa buffer de pixels.
  for (uint i = 0; i < LED_COUNT; ++i) {
    leds[i].R = 0;
    leds[i].G = 0;
    leds[i].B = 0;
  }
}

/**
 * Atribui uma cor RGB a um LED.
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}
/**
 * Limpa o buffer de pixels.
 */
void npClear() {
  for (uint i = 0; i < LED_COUNT; ++i)
    npSetLED(i, 0, 0, 0);
}


/**
 * Escreve os dados do buffer nos LEDs.
 */
void npWrite() {
  // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
  for (uint i = 0; i < LED_COUNT; ++i) {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
  sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}
void npSetLEDs(uint led_ids[], uint size_of_led_ids, uint8_t leds_rgb_color[]){
  npClear();
  for (size_t i = 0; i < size_of_led_ids; i++)
  {
    uint index = led_ids[i];
    leds[index].R = leds_rgb_color[0];
    leds[index].G = leds_rgb_color[1];
    leds[index].B = leds_rgb_color[2];
  }
  npWrite();
}

void displayArrowIndex(uint i){
  npSetLEDs(arrow_positions[i], arrow_size, arrow_colors[i]);
}


//function to get a joystick move only when it returns to center first
JoystickDirection get_joystick_move() {
  JoystickDirection move = read_joystick();
  
  if (move == CENTER) {
      joystick_moved = false; // Reset the flag when the joystick is centered
      return CENTER;
  }

  if (!joystick_moved && move != CENTER) {
      joystick_moved = true; // Mark joystick as moved
      return move; // Return the detected move
  }
  return CENTER; // Ignore repeated moves until joystick resets
}


// Function to generate a random sequence
void generate_sequence(int sequence[], int length) {
  for (int i = 0; i < length; i++) {
      sequence[i] = rand() % NUM_BUTTONS;  // Random button press
      while (sequence[i] == 0 || sequence[i] == 9){
        sequence[i] = rand() % NUM_BUTTONS;  // Random button press
      }
      printf("random number: %d\n", sequence[i]);
  }
}

int generate_random_number(void){
    int rnd_num = rand() % NUM_BUTTONS;  // Random button press
    while (rnd_num == 0 || rnd_num == 9){
      rnd_num = rand() % NUM_BUTTONS;  // Random button press
    }
    printf("random number: %d\n", rnd_num);
    return rnd_num;
}

// Function to play the sequence (light up the buttons)
void play_sequence(int sequence[], int length) {
  for (uint i = 0; i < length; i++) {
      printf("Button %d\n", sequence[i]);  // Simulate LED indicator or serial print
      displayArrowIndex(sequence[i]-1);
      play_tone(BUZZER_PIN, notes_buzzer[sequence[i]-1], 150);
      sleep_ms(350);
      npClear();
      npWrite();
      sleep_ms(500);
  }
}


// Function to get player input and check if it matches the sequence
bool check_player_input(int sequence[], int length) {
  for (int i = 0; i < length; i++) {
      JoystickDirection joystick_input = get_joystick_move();
      

      while (joystick_input == CENTER) {
          joystick_input = get_joystick_move();
          sleep_ms(50);
      }

      int index = joystick_input - 1;

      displayArrowIndex(index);
      play_tone(BUZZER_PIN, notes_buzzer[index], 150);
      //sleep_ms(150);
      npClear();
      npWrite();

      printf("read joystick value: %d, sequence value %d, index %d\n", joystick_input, sequence[i], index);
      if (joystick_input != sequence[i]) {
          return false;
      }
  }
  npClear();
  npWrite();
  sleep_ms(250);  // debounce delay
  return true;
}

void btn_pressed_callback(uint gpio, uint32_t events){
  btn_pressed = true;
  gpio_set_irq_enabled(JOYSTICK_BTN, GPIO_IRQ_EDGE_FALL, true);
}

int main() {

  stdio_init_all();
  
  SSD1306_init();
  pwm_init_buzzer(BUZZER_PIN);
  gpio_init(JOYSTICK_BTN);
  gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
  gpio_pull_up(JOYSTICK_BTN);
  gpio_set_irq_enabled_with_callback(JOYSTICK_BTN, GPIO_IRQ_EDGE_FALL, true, &btn_pressed_callback);

  // Initialize render area for entire frame (SSD1306_WIDTH pixels by SSD1306_NUM_PAGES pages)
  struct render_area frame_area = {
      start_col: 0,
      end_col : SSD1306_WIDTH - 1,
      start_page : 0,
      end_page : SSD1306_NUM_PAGES - 1
      };

  calc_render_area_buflen(&frame_area);

  // zero the entire display
  uint8_t buf[SSD1306_BUF_LEN];
  memset(buf, 0, SSD1306_BUF_LEN);
  render(buf, &frame_area);
  
  char *text[] = {
      "             ",
      " Memory Game ",
      "             ",
      "Press and hold",
      " Joystick to ",
      "    START    ",
      "             ",
  };

  int y = 0;
  for (uint i = 0 ;i < count_of(text); i++) {
      WriteString(buf, 5, y, text[i]);
      y+=8;
  }
  render(buf, &frame_area);


  while(btn_pressed == false){
      sleep_ms(500);
      SSD1306_send_cmd(SSD1306_SET_INV_DISP);
      sleep_ms(500);
      SSD1306_send_cmd(SSD1306_SET_NORM_DISP);
  }
  memset(buf, 0, SSD1306_BUF_LEN);
  render(buf, &frame_area);
  SSD1306_send_cmd(SSD1306_SET_INV_DISP);


  srand(time(NULL));  // Initialize random seed
  // Initialize ADC
  adc_init();
  adc_gpio_init(JOY_X_PIN);
  adc_gpio_init(JOY_Y_PIN);
  //init_buttons();

  int sequence[SEQUENCE_LENGTH];
  int sequence_length = 1;  // Start with a sequence length of 1
  int already_generated = sequence_length;
  // Inicializa matriz de LEDs NeoPixel.
  npInit(LED_PIN);
  npClear();
  generate_sequence(sequence, sequence_length);
  // Aqui, você desenha nos LEDs.
  while (1) {
    printf("Sequence length: %d\n", sequence_length);


    play_sequence(sequence, sequence_length);
    

    // Wait for user input and check if it matches the sequence
    if (check_player_input(sequence, sequence_length)) {
        printf("Correct!\n");
        sequence_length++;  // Increase the sequence length
        sequence[sequence_length-1] = generate_random_number();
    } else {
        printf("Incorrect! Game over.\n");
        sequence_length = 1;  // Reset the game
    }
    npClear();
    npWrite();
    sleep_ms(3000);  // Delay between rounds
  }

}
