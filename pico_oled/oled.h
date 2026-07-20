#ifndef OLED_H
#define OLED_H

#include <stdbool.h>
#include "hardware/i2c.h"

void oled_init_i2c(i2c_inst_t *i2c, uint gpio_sda, uint gpio_scl);
void oled_show_status(bool connected);
void oled_show_connecting(void);

#endif // OLED_H
