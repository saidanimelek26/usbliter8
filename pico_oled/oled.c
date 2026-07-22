#include "oled.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static i2c_inst_t *oled_i2c = NULL;
static bool oled_ready = false;
static char line1_buf[21];
static char line2_buf[21];

// ============================================
// Core functions with improved initialization
// ============================================

void oled_init_i2c(i2c_inst_t *i2c, uint gpio_sda, uint gpio_scl) {
    // Initialize I2C for SH1106 display
    const uint baud = 400000;  // Standard I2C speed
    i2c_init(i2c, baud);
    gpio_set_function(gpio_sda, GPIO_FUNC_I2C);
    gpio_set_function(gpio_scl, GPIO_FUNC_I2C);
    gpio_pull_up(gpio_sda);
    gpio_pull_up(gpio_scl);

    oled_i2c = i2c;
    
    // Try multiple times to initialize SH1106
    bool init_success = false;
    for (int attempt = 0; attempt < 3; attempt++) {
        if (ssd1306_init_i2c(oled_i2c, 0x3C)) {
            init_success = true;
            break;
        }
        sleep_ms(50);
    }
    
    if (!init_success) {
        oled_ready = false;
        printf("OLED initialization FAILED\n");
        return;
    }
    
    // Clear display and update
    ssd1306_clear();
    ssd1306_update();
    sleep_ms(100);
    
    oled_ready = true;
    printf("OLED initialized successfully\n");
    
    // Test display with initialization message
    oled_show_message("usbliter8", "Ready");
    sleep_ms(500);
}

void oled_show_message(const char *line1, const char *line2) {
    if (!oled_ready) {
        return;
    }
    
    ssd1306_clear();
    
    // Convert to uppercase for better compatibility
    char line1_upper[21] = {0};
    char line2_upper[21] = {0};
    
    // Draw line 1 at y=0
    if (line1) {
        int i = 0;
        for (const char *p = line1; *p && i < 20; p++, i++) {
            line1_upper[i] = toupper((unsigned char)*p);
        }
        line1_upper[i] = '\0';
        ssd1306_draw_text(0, 0, line1_upper);
    }
    
    // Draw line 2 at y=16 (2nd page)
    if (line2) {
        int i = 0;
        for (const char *p = line2; *p && i < 20; p++, i++) {
            line2_upper[i] = toupper((unsigned char)*p);
        }
        line2_upper[i] = '\0';
        ssd1306_draw_text(0, 16, line2_upper);
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
    oled_show_message("EXPLOITING", "Please wait");
}

void oled_show_exploit_progress(uint8_t percent) {
    if (!oled_ready) return;
    if (percent > 100) percent = 100;
    
    snprintf(line1_buf, sizeof(line1_buf), "PROGRESS %3d%%", percent);
    
    // Progress bar (12 characters wide for display)
    char progress[13] = {0};
    uint8_t bars = (percent * 12) / 100;
    for (uint8_t i = 0; i < bars && i < 12; i++) {
        progress[i] = '=';
    }
    if (percent < 100 && bars < 12) {
        progress[bars] = '>';
    }
    progress[12] = '\0';
    
    snprintf(line2_buf, sizeof(line2_buf), "[%s]", progress);
    oled_show_message(line1_buf, line2_buf);
}

void oled_show_stage(const char *stage) {
    if (!stage) return;
    snprintf(line1_buf, sizeof(line1_buf), "STAGE: %s", stage);
    oled_show_message(line1_buf, "Running...");
}

void oled_show_pwnd_success(void) {
    oled_show_message("SUCCESS!", "DEVICE PWND!");
}

void oled_show_done(void) {
    oled_show_message("COMPLETE", "Ready for next");
}

// ============================================
// Error functions
// ============================================

void oled_show_error_msg(const char *error_msg) {
    if (!oled_ready) return;
    
    if (error_msg) {
        snprintf(line1_buf, sizeof(line1_buf), "ERROR!");
        snprintf(line2_buf, sizeof(line2_buf), "%s", error_msg);
        oled_show_message(line1_buf, line2_buf);
    } else {
        oled_show_message("ERROR!", "Check connection");
    }
}

void oled_show_error(void) {
    oled_show_message("ERROR!", "Check setup");
}

// ============================================
// Status functions
// ============================================

void oled_show_unsupported(void) {
    oled_show_message("UNSUPPORTED", "Not supported");
}

void oled_show_already_pwned(void) {
    oled_show_message("ALREADY PWND", "Reboot needed");
}

void oled_show_reboot_needed(void) {
    oled_show_message("REBOOT NEEDED", "Restart device");
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
        oled_show_error_msg("Exploit failed");
    } else {
        oled_show_message("STATE", state);
    }
}

void oled_show_device_info(const char *info) {
    if (!info) return;
    oled_show_message("DEVICE INFO", info);
}

// ============================================
// Additional useful functions
// ============================================

void oled_show_waiting_for_device(void) {
    oled_show_message("SEARCHING", "For DFU...");
}

void oled_show_device_detected(void) {
    oled_show_message("DEVICE FOUND", "Checking...");
}

void oled_show_unsupported_device(uint16_t vid, uint16_t pid) {
    snprintf(line1_buf, sizeof(line1_buf), "VID:0x%04X", vid);
    snprintf(line2_buf, sizeof(line2_buf), "PID:0x%04X", pid);
    oled_show_message(line1_buf, line2_buf);
}

void oled_show_exploit_stage(uint8_t stage_num, const char *stage_name) {
    if (!stage_name) return;
    snprintf(line1_buf, sizeof(line1_buf), "STAGE %d/5", stage_num);
    snprintf(line2_buf, sizeof(line2_buf), "%s", stage_name);
    oled_show_message(line1_buf, line2_buf);
}

void oled_show_success_with_info(const char *info) {
    if (!info) {
        oled_show_pwnd_success();
        return;
    }
    oled_show_message("SUCCESS", info);
}

void oled_show_failure_with_info(const char *info) {
    if (!info) {
        oled_show_error();
        return;
    }
    oled_show_message("FAILED", info);
}

void oled_show_reset(void) {
    oled_show_message("RESET", "Bus reset");
}

void oled_show_ready(void) {
    oled_show_message("READY", "Waiting...");
}

// ============================================
// Legacy compatibility functions
// ============================================

void oled_show_status(bool connected) {
    if (!oled_ready) return;
    
    if (connected) {
        oled_show_message("PHONE:", "CONNECTED");
    } else {
        oled_show_message("PHONE:", "DISCONNECTED");
    }
}

void oled_show_connecting(void) {
    oled_show_message("CONNECTING", "Please wait");
}

void oled_show_running(void) {
    oled_show_message("RUNNING", "Exploit active");
}
