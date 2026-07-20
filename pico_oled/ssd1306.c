#include "ssd1306.h"
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// This driver targets SH1106-based modules (commonly 132x64 internal, we use 128x64 window)
// The SH1106 uses page addressing with commands 0xB0..0xB7 and column address set via
// lower/higher nibble commands (0x00..0x0F, 0x10..0x1F). Many 128x64 modules expect a
// 2-column offset (left padding) when written to a 132-wide controller.

#define LCD_WIDTH_LOGICAL 128
#define LCD_HEIGHT 64
#define LCD_PAGES (LCD_HEIGHT/8)
#define SH1106_COL_OFFSET 2

static i2c_inst_t *i2c_port = NULL;
static uint8_t i2c_addr = 0x3C;
static uint8_t buffer[LCD_WIDTH_LOGICAL * LCD_PAGES];

static const uint8_t init_sequence[] = {
    0xAE, // display off
    0xD5, 0x80, // set display clock divide ratio/oscillator frequency
    0xA8, 0x3F, // set multiplex ratio (1 to 64)
    0xD3, 0x00, // set display offset
    0x40,       // set start line to 0
    0xAD, 0x8B, // DC-DC ON (recommended for SH1106 modules)
    0xA1,       // segment remap
    0xC8,       // COM output scan direction remapped
    0xDA, 0x12, // COM pins hardware configuration
    0x81, 0x7F, // set contrast
    0xD9, 0x22, // set pre-charge period
    0xDB, 0x40, // set VCOMH deselect level
    0xA4,       // output RAM to display
    0xA6,       // normal display
    0xAF        // display ON
};

static int write_cmd(const uint8_t *cmds, size_t len) {
    if (!i2c_port) return -1;
    // control byte 0x00 for commands
    uint8_t tx[len + 1];
    tx[0] = 0x00;
    for (size_t i = 0; i < len; ++i) tx[i + 1] = cmds[i];
    return i2c_write_blocking(i2c_port, i2c_addr, tx, (int)(len + 1), false);
}

static int write_data(const uint8_t *data, size_t len) {
    if (!i2c_port) return -1;
    // control byte 0x40 for data
    const size_t chunk = 16;
    uint8_t tx[chunk + 1];
    tx[0] = 0x40;
    size_t sent = 0;
    while (sent < len) {
        size_t tosend = (len - sent) > chunk ? chunk : (len - sent);
        for (size_t i = 0; i < tosend; ++i) tx[i + 1] = data[sent + i];
        int rc = i2c_write_blocking(i2c_port, i2c_addr, tx, (int)(tosend + 1), false);
        if (rc < 0) return rc;
        sent += tosend;
    }
    return 0;
}

bool ssd1306_init_i2c(i2c_inst_t *i2c, uint8_t address) {
    if (!i2c) return false;
    i2c_port = i2c;
    i2c_addr = address;

    if (write_cmd(init_sequence, sizeof(init_sequence)) < 0) return false;
    ssd1306_clear();
    ssd1306_update();
    return true;
}

void ssd1306_clear(void) {
    memset(buffer, 0, sizeof(buffer));
}

void ssd1306_update(void) {
    // SH1106 doesn't support 0x21/0x22 column/page addressing consistently.
    // Instead update page-by-page using 0xB0..0xB7 and set column via lower/higher nibble.
    for (int page = 0; page < LCD_PAGES; ++page) {
        uint8_t cmds[3];
        cmds[0] = 0xB0 | (page & 0x07); // set page address
        if (write_cmd(cmds, 1) < 0) return;

        // set column address (lower and higher nibble) with offset
        uint8_t col = SH1106_COL_OFFSET & 0xFF;
        uint8_t col_low = 0x00 | (col & 0x0F);
        uint8_t col_high = 0x10 | ((col >> 4) & 0x0F);
        uint8_t col_cmds[2] = { col_low, col_high };
        if (write_cmd(col_cmds, 2) < 0) return;

        // write LCD_WIDTH_LOGICAL bytes for this page
        const uint8_t *page_ptr = &buffer[page * LCD_WIDTH_LOGICAL];
        if (write_data(page_ptr, LCD_WIDTH_LOGICAL) < 0) return;
    }
}

// Minimal 5x8 font for required characters. Each character is 5 bytes (columns), LSB at top.
static const uint8_t font_5x8[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // space
    {0x7C,0x12,0x11,0x12,0x7C}  // A placeholder
};

static const uint8_t *get_glyph(char c) {
    static const uint8_t glyph_space[5] = {0,0,0,0,0};
    static const uint8_t glyph_colon[5] = {0x00,0x36,0x36,0x00,0x00};
    static const uint8_t glyph_dot[5] = {0x00,0x00,0x60,0x60,0x00};
    static const uint8_t glyph_a[5] = {0x20,0x54,0x54,0x54,0x78};
    static const uint8_t glyph_c[5] = {0x38,0x44,0x44,0x44,0x20};
    static const uint8_t glyph_d[5] = {0x38,0x44,0x44,0x44,0x78};
    static const uint8_t glyph_e[5] = {0x38,0x54,0x54,0x54,0x18};
    static const uint8_t glyph_f[5] = {0x08,0x7E,0x09,0x01,0x02};
    static const uint8_t glyph_g[5] = {0x18,0xA4,0xA4,0xA4,0x7C};
    static const uint8_t glyph_h[5] = {0x7F,0x08,0x04,0x04,0x78};
    static const uint8_t glyph_i[5] = {0x00,0x44,0x7D,0x40,0x00};
    static const uint8_t glyph_l[5] = {0x00,0x41,0x7F,0x40,0x00};
    static const uint8_t glyph_n[5] = {0x7C,0x04,0x02,0x04,0x78};
    static const uint8_t glyph_o[5] = {0x38,0x44,0x44,0x44,0x38};
    static const uint8_t glyph_p[5] = {0xFC,0x24,0x24,0x24,0x18};
    static const uint8_t glyph_r[5] = {0x7C,0x08,0x04,0x04,0x08};
    static const uint8_t glyph_s[5] = {0x48,0x54,0x54,0x54,0x24};
    static const uint8_t glyph_t[5] = {0x04,0x3F,0x44,0x40,0x20};
    static const uint8_t glyph_w[5] = {0x7C,0x40,0x30,0x40,0x7C};
    static const uint8_t glyph_C[5] = {0x3E,0x41,0x41,0x41,0x22};
    static const uint8_t glyph_P[5] = {0x7F,0x09,0x09,0x09,0x06};
    static const uint8_t glyph_D[5] = {0x7F,0x41,0x41,0x22,0x1C};

    if (c == ' ') return glyph_space;
    if (c == ':') return glyph_colon;
    if (c == '.') return glyph_dot;
    if (c == 'a') return glyph_a;
    if (c == 'c') return glyph_c;
    if (c == 'd') return glyph_d;
    if (c == 'e') return glyph_e;
    if (c == 'f') return glyph_f;
    if (c == 'g') return glyph_g;
    if (c == 'h') return glyph_h;
    if (c == 'i') return glyph_i;
    if (c == 'l') return glyph_l;
    if (c == 'n') return glyph_n;
    if (c == 'o') return glyph_o;
    if (c == 'p') return glyph_p;
    if (c == 'r') return glyph_r;
    if (c == 's') return glyph_s;
    if (c == 't') return glyph_t;
    if (c == 'w' || c == 'W') return glyph_w;
    if (c == 'C') return glyph_C;
    if (c == 'P') return glyph_P;
    if (c == 'D') return glyph_D;
    return glyph_space;
}

void ssd1306_draw_text(int x, int y, const char *text) {
    if (!text) return;
    if (x >= LCD_WIDTH_LOGICAL) return;
    if (y % 8 != 0) return; // only support multiples of 8 for simplicity
    int page = y / 8;
    int col = x;
    for (const char *p = text; *p && col < LCD_WIDTH_LOGICAL - 6; p++) {
        const uint8_t *g = get_glyph(*p);
        for (int i = 0; i < 5; ++i) {
            if (col >= 0 && col < LCD_WIDTH_LOGICAL) {
                buffer[page * LCD_WIDTH_LOGICAL + col] = g[i];
            }
            col++;
        }
        if (col >= 0 && col < LCD_WIDTH_LOGICAL) buffer[page * LCD_WIDTH_LOGICAL + col] = 0x00;
        col++;
    }
}
