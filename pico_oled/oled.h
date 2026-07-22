#ifndef OLED_H
#define OLED_H

#include <stdbool.h>
#include <stdint.h>
#include "hardware/i2c.h"

// Core functions
void oled_init_i2c(i2c_inst_t *i2c, uint gpio_sda, uint gpio_scl);
void oled_show_message(const char *line1, const char *line2);
void oled_clear(void);

// usbliter8 specific status messages
void oled_show_booting(void);
void oled_show_waiting_dfu(void);
void oled_show_device_found(uint16_t vid, uint16_t pid);
void oled_show_exploiting(void);
void oled_show_exploit_progress(uint8_t percent);
void oled_show_stage(const char *stage);
void oled_show_pwnd_success(void);
void oled_show_done(void);
void oled_show_unsupported(void);
void oled_show_already_pwned(void);

// Error functions with DIFFERENT names
void oled_show_error_msg(const char *error_msg);  // With parameter - FOR NEW CODE
void oled_show_error(void);                        // Without parameter - FOR LEGACY

void oled_show_reboot_needed(void);
void oled_show_led_sync(const char *state);
void oled_show_device_info(const char *info);

// Additional useful functions
void oled_show_waiting_for_device(void);
void oled_show_device_detected(void);
void oled_show_unsupported_device(uint16_t vid, uint16_t pid);
void oled_show_exploit_stage(uint8_t stage_num, const char *stage_name);
void oled_show_success_with_info(const char *info);
void oled_show_failure_with_info(const char *info);
void oled_show_reset(void);
void oled_show_ready(void);

// Legacy compatibility functions
void oled_show_status(bool connected);
void oled_show_connecting(void);
void oled_show_running(void);
void oled_show_unsupported(void);
void oled_show_already_pwned(void);

#endif // OLED_H
