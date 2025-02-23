/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

int main() {
    stdio_init_all();
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(26);
    adc_gpio_init(27);

    while (1) {
        adc_select_input(0);
        uint adc_y_raw = adc_read();
        adc_select_input(1);
        uint adc_x_raw = adc_read();

        const uint bar_width = 40;
        const uint adc_max = (1 << 12) - 1;
        uint bar_x_pos = adc_x_raw * bar_width/ adc_max;
        uint bar_y_pos = adc_y_raw * bar_width/ adc_max;
        printf("\r[X,Y] = [%d,%d]\n", bar_x_pos, bar_y_pos);
        sleep_ms(50);

    }
}