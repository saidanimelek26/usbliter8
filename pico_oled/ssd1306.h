#ifndef SSD1306_H
#define SSD1306_H

#include <stdbool.h>
#include <stdint.h>
#include "hardware/i2c.h"

bool ssd1306_init_i2c(i2c_inst_t *i2c, uint8_t address);
void ssd1306_clear(void);
void ssd1306_draw_text(int x, int y, const char *text);
void ssd1306_update(void);

#endif // SSD1306_H
