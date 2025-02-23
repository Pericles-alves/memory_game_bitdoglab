#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

// Definição do número de LEDs e pino.
#define LED_COUNT 25
#define LED_PIN 7
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
uint8_t arrow_colors[8][3] = {
  {0x19, 0x00, 0x00}, // Color 1
  {0x1F, 0x18, 0x33}, // Color 2
  {0x18, 0x33, 0x10}, // Color 3
  {0x13, 0x32, 0x00}, // Color 4
  {0x19, 0x00, 0x13}, // Color 5
  {0x13, 0x31, 0x19}, // Color 6
  {0x33, 0x10, 0x19}, // Color 7
  {0x00, 0x10, 0x19}  // Color 8
};
*/

uint8_t red_color[3] = {0x10,0x00,0x00};
uint8_t green_color[3] = {0x00,0x10,0x00};
uint8_t blue_color[3] = {0x00,0x00,0x10};


uint arrow_positions[8][9] = {
  {2,7,10,12,14,16,17,18,22}, // up
  {4,6,10,12,18,19,20,21,22}, // right up
  {14,13,12,11,10,8,2,18,22}, // right
  {0,1,2,9,10,8,12,16,24},    // right down
  {2,8,6,10,14,7,12,17,22},   // down
  {4,3,2,5,14,6,12,18,20},    // left_down
  {14,6,2,16,22,13,12,11,10}, // left
  {0,8,12,14,15,16,24,23,22}  // left_up
};
uint num_of_arrows = 8;
uint arrow_size = 9;

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


int main() {

  // Inicializa entradas e saídas.
  stdio_init_all();

  // Inicializa matriz de LEDs NeoPixel.
  npInit(LED_PIN);
  npClear();

  for (size_t i = 0; i < num_of_arrows; i++)
  {
    npSetLEDs(arrow_positions[i], arrow_size, arrow_colors[i]);
    sleep_ms(1000);
  }


  // Não faz mais nada. Loop infinito.
  while (true) {
    sleep_ms(1000);
  }
}
