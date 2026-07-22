#include "oled.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include <stdio.h>
#include <string.h>

static i2c_inst_t *oled_i2c = NULL;
static bool oled_ready = false;
static char line1_buf[21];
static char line2_buf[21];

// ============================================
// Core functions
// ============================================

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

void oled_show_message(const char *line1, const char *line2) {
    if (!oled_ready) return;
    ssd1306_clear();
    
    if (line1) {
        ssd1306_draw_text(0, 0, line1);
    }
    
    if (line2) {
        ssd1306_draw_text(0, 16, line2);
    }
    
    ssd1306_update();
}

void oled_clear(void) {
    if (!oled_ready) return;
    ssd1306_clear();
    ssd1306_update();
}

// ============================================
// usbliter8 specific status messages
// ============================================

void oled_show_booting(void) {
    oled_show_message("usbliter8", "Booting...");
}

void oled_show_waiting_dfu(void) {
    oled_show_message("WAITING DFU", "Connect device");
}

void oled_show_device_found(uint16_t vid, uint16_t pid) {
    snprintf(line1_buf, sizeof(line1_buf), "VID:0x%04X", vid);
    snprintf(line2_buf, sizeof(line2_buf), "PID:0x%04X", pid);
    oled_show_message(line1_buf, line2_buf);
}

void oled_show_exploiting(void) {
    oled_show_message("EXPLOITING...", "Please wait");
}

void oled_show_exploit_progress(uint8_t percent) {
    if (!oled_ready) return;
    if (percent > 100) percent = 100;
    
    snprintf(line1_buf, sizeof(line1_buf), "EXPLOIT %d%%", percent);
    
    // Progress bar (16 characters wide)
    char progress[17] = {0};
    uint8_t bars = (percent * 16) / 100;
    for (uint8_t i = 0; i < bars && i < 16; i++) {
        progress[i] = '=';
    }
    if (percent < 100 && bars < 16) {
        progress[bars] = '>';
    }
    progress[16] = '\0';
    
    snprintf(line2_buf, sizeof(line2_buf), "[%s]", progress);
    oled_show_message(line1_buf, line2_buf);
}

void oled_show_stage(const char *stage) {
    snprintf(line1_buf, sizeof(line1_buf), "STAGE: %s", stage);
    oled_show_message(line1_buf, "Exploiting...");
}

void oled_show_pwnd_success(void) {
    oled_show_message("✓ PWND!", "Exploit successful");
}

void oled_show_done(void) {
    oled_show_message("✓ DONE", "Process complete");
}

void oled_show_unsupported(void) {
    oled_show_message("UNSUPPORTED", "Device not supported");
}

void oled_show_already_pwned(void) {
    oled_show_message("ALREADY PWND", "Device is pwned");
}

// ============================================
// Both versions of oled_show_error
// ============================================

// Version with parameter - for custom error messages
void oled_show_error(const char *error_msg) {
    if (error_msg) {
        snprintf(line1_buf, sizeof(line1_buf), "✗ ERROR");
        snprintf(line2_buf, sizeof(line2_buf), "%s", error_msg);
        oled_show_message(line1_buf, line2_buf);
    } else {
        oled_show_message("✗ ERROR", "Check connection");
    }
}

// Version without parameter - legacy support
void oled_show_error(void) {
    oled_show_message("✗ ERROR", "Check connection");
}

void oled_show_reboot_needed(void) {
    oled_show_message("REBOOT NEEDED", "Restart Pico");
}

void oled_show_led_sync(const char *state) {
    if (!state) return;
    
    if (strcmp(state, "BOOTING") == 0) {
        oled_show_booting();
    } else if (strcmp(state, "IDLE") == 0) {
        oled_show_waiting_dfu();
    } else if (strcmp(state, "EXPLOIT") == 0) {
        oled_show_exploiting();
    } else if (strcmp(state, "SUCCESS") == 0) {
        oled_show_pwnd_success();
    } else if (strcmp(state, "FAILED") == 0) {
        oled_show_error("Exploit failed");
    } else {
        oled_show_message("STATE", state);
    }
}

void oled_show_device_info(const char *info) {
    oled_show_message("DEVICE INFO", info);
}

// ============================================
// Additional useful functions
// ============================================

void oled_show_waiting_for_device(void) {
    oled_show_message("SEARCHING", "For DFU device");
}

void oled_show_device_detected(void) {
    oled_show_message("DEVICE FOUND", "Checking support...");
}

void oled_show_unsupported_device(uint16_t vid, uint16_t pid) {
    snprintf(line1_buf, sizeof(line1_buf), "UNSUPPORTED");
    snprintf(line2_buf, sizeof(line2_buf), "VID:0x%04X PID:0x%04X", vid, pid);
    oled_show_message(line1_buf, line2_buf);
}

void oled_show_exploit_stage(uint8_t stage_num, const char *stage_name) {
    snprintf(line1_buf, sizeof(line1_buf), "STAGE %d/5", stage_num);
    snprintf(line2_buf, sizeof(line2_buf), "%s", stage_name);
    oled_show_message(line1_buf, line2_buf);
}

void oled_show_success_with_info(const char *info) {
    oled_show_message("✓ SUCCESS", info);
}

void oled_show_failure_with_info(const char *info) {
    oled_show_message("✗ FAILED", info);
}

void oled_show_reset(void) {
    oled_show_message("RESET", "Bus reset");
}

void oled_show_ready(void) {
    oled_show_message("READY", "Waiting for DFU");
}

// ============================================
// Legacy compatibility functions
// ============================================

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
    oled_show_booting();  // Reuse new function
}

void oled_show_running(void) {
    oled_show_exploiting();  // Reuse new function
}

void oled_show_unsupported(void) {
    oled_show_unsupported();
}

void oled_show_already_pwned(void) {
    oled_show_already_pwned();
}
