#include "components/oled_ssd1306/ssd1306_i2c.h"

#define JOYSTICK_BTN 22

volatile bool btn_pressed = false;

void btn_pressed_callback(uint gpio, uint32_t events){
    btn_pressed = true;
    gpio_set_irq_enabled(JOYSTICK_BTN, GPIO_IRQ_EDGE_FALL, true);
}

int main() {
    stdio_init_all();
    SSD1306_init();

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

    
    return 0;
}
