#include "oled.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"

static i2c_inst_t *oled_i2c = NULL;
static bool oled_ready = false;

void oled_init_i2c(i2c_inst_t *i2c, uint gpio_sda, uint gpio_scl) {
    const uint baud = 100000;
    i2c_init(i2c, baud);
    gpio_set_function(gpio_sda, GPIO_FUNC_I2C);
    gpio_set_function(gpio_scl, GPIO_FUNC_I2C);
    gpio_pull_up(gpio_sda);
    gpio_pull_up(gpio_scl);

    oled_i2c = i2c;
    if (!ssd1306_init_i2c(oled_i2c, 0x3C)) {
        oled_ready = false;
        return;
    }
    ssd1306_clear();
    ssd1306_update();
    oled_ready = true;
}

void oled_show_status(bool connected) {
    if (!oled_ready) return;
    ssd1306_clear();
    ssd1306_draw_text(0, 0, "Phone:");
    if (connected) {
        ssd1306_draw_text(0, 16, "Connected");
    } else {
        ssd1306_draw_text(0, 16, "Disconnected");
    }
    ssd1306_update();
}

void oled_show_connecting(void) {
    if (!oled_ready) return;
    ssd1306_clear();
    ssd1306_draw_text(0, 0, "Waiting for phone");
    ssd1306_draw_text(0, 16, "Connecting...");
    ssd1306_update();
}
