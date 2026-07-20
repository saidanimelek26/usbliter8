#include "ssd1306.h"
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_PAGES (SSD1306_HEIGHT/8)

static i2c_inst_t *i2c_port = NULL;
static uint8_t i2c_addr = 0x3C;
static uint8_t buffer[SSD1306_WIDTH * SSD1306_PAGES];

static const uint8_t init_sequence[] = {
    0xAE, // display off
    0xD5, 0x80, // set display clock div
    0xA8, 0x3F, // set multiplex (height-1) 0x3F=63
    0xD3, 0x00, // set display offset
    0x40, // set start line = 0
    0x8D, 0x14, // charge pump on
    0x20, 0x00, // memory mode horizontal
    0xA1, // segment remap
    0xC8, // com scan dec
    0xDA, 0x12, // com pins
    0x81, 0xCF, // contrast
    0xD9, 0xF1, // precharge
    0xDB, 0x40, // vcom detect
    0xA4, // resume to RAM display
    0xA6, // normal display
    0xAF // display on
};

static int write_cmd(const uint8_t *cmds, size_t len) {
    if (!i2c_port) return -1;
    uint8_t buf[len+1];
    buf[0] = 0x00; // control byte - command
    for (size_t i=0;i<len;i++) buf[i+1]=cmds[i];
    int rc = i2c_write_blocking(i2c_port, i2c_addr, buf, len+1, false);
    return rc;
}

static int write_data(const uint8_t *data, size_t len) {
    if (!i2c_port) return -1;
    const size_t chunk = 16;
    uint8_t tx[chunk+1];
    tx[0] = 0x40; // control byte - data
    size_t sent = 0;
    while (sent < len) {
        size_t tosend = (len - sent) > chunk ? chunk : (len - sent);
        for (size_t i=0;i<tosend;i++) tx[i+1] = data[sent+i];
        int rc = i2c_write_blocking(i2c_port, i2c_addr, tx, tosend+1, false);
        if (rc < 0) return rc;
        sent += tosend;
    }
    return 0;
}

bool ssd1306_init_i2c(i2c_inst_t *i2c, uint8_t address) {
    if (!i2c) return false;
    i2c_port = i2c;
    i2c_addr = address;

    // run init sequence
    if (write_cmd(init_sequence, sizeof(init_sequence)) < 0) return false;
    ssd1306_clear();
    ssd1306_update();
    return true;
}

void ssd1306_clear(void) {
    memset(buffer, 0, sizeof(buffer));
}

void ssd1306_update(void) {
    // set column and page addresses then stream buffer
    uint8_t cmds[3];
    cmds[0] = 0x21; // set column address
    cmds[1] = 0x00;
    cmds[2] = SSD1306_WIDTH - 1;
    write_cmd(cmds, 3);
    uint8_t cmds2[3];
    cmds2[0] = 0x22; // set page address
    cmds2[1] = 0x00;
    cmds2[2] = SSD1306_PAGES - 1;
    write_cmd(cmds2, 3);

    write_data(buffer, sizeof(buffer));
}

// Minimal 5x8 font for required characters. Each character is 5 bytes (columns), LSB at top.
// Supported characters: space, letters used in messages, colon '.' fallback to space.
static const uint8_t font_5x8[][5] = {
    // ' ' (space)
    {0x00,0x00,0x00,0x00,0x00},
    // 'A' (index 1) - placeholder if requested
    {0x7C,0x12,0x11,0x12,0x7C},
};

// helper to get a very small glyph for ASCII; fallback to space
static const uint8_t *get_glyph(char c) {
    // for simplicity, only provide basic glyphs for characters we use
    static const uint8_t glyph_space[5] = {0,0,0,0,0};
    static const uint8_t glyph_dash[5]  = {0x08,0x08,0x08,0x08,0x08};
    // very small handcrafted glyphs for common lowercase letters used
    static const uint8_t glyph_a[5] = {0x20,0x54,0x54,0x54,0x78}; // a
    static const uint8_t glyph_c[5] = {0x38,0x44,0x44,0x44,0x20}; // c
    static const uint8_t glyph_d[5] = {0x38,0x44,0x44,0x44,0x78}; // d
    static const uint8_t glyph_e[5] = {0x38,0x54,0x54,0x54,0x18}; // e
    static const uint8_t glyph_f[5] = {0x08,0x7E,0x09,0x01,0x02}; // f (approx)
    static const uint8_t glyph_g[5] = {0x18,0xA4,0xA4,0xA4,0x7C}; // g (approx)
    static const uint8_t glyph_h[5] = {0x7F,0x08,0x04,0x04,0x78}; // h
    static const uint8_t glyph_i[5] = {0x00,0x44,0x7D,0x40,0x00}; // i
    static const uint8_t glyph_l[5] = {0x00,0x41,0x7F,0x40,0x00}; // l
    static const uint8_t glyph_n[5] = {0x7C,0x04,0x02,0x04,0x78}; // n
    static const uint8_t glyph_o[5] = {0x38,0x44,0x44,0x44,0x38}; // o
    static const uint8_t glyph_p[5] = {0xFC,0x24,0x24,0x24,0x18}; // p
    static const uint8_t glyph_r[5] = {0x7C,0x08,0x04,0x04,0x08}; // r
    static const uint8_t glyph_s[5] = {0x48,0x54,0x54,0x54,0x24}; // s (approx)
    static const uint8_t glyph_t[5] = {0x04,0x3F,0x44,0x40,0x20}; // t
    static const uint8_t glyph_w[5] = {0x7C,0x40,0x30,0x40,0x7C}; // w (approx)
    static const uint8_t glyph_colon[5] = {0x00,0x36,0x36,0x00,0x00}; // :
    static const uint8_t glyph_dot[5] = {0x00,0x00,0x60,0x60,0x00}; // .
    static const uint8_t glyph_C[5] = {0x3E,0x41,0x41,0x41,0x22}; // C
    static const uint8_t glyph_P[5] = {0x7F,0x09,0x09,0x09,0x06}; // P
    static const uint8_t glyph_D[5] = {0x7F,0x41,0x41,0x22,0x1C}; // D

    if (c==' ') return glyph_space;
    if (c==':') return glyph_colon;
    if (c=='.') return glyph_dot;
    if (c=='a') return glyph_a;
    if (c=='c') return glyph_c;
    if (c=='d') return glyph_d;
    if (c=='e') return glyph_e;
    if (c=='f') return glyph_f;
    if (c=='g') return glyph_g;
    if (c=='h') return glyph_h;
    if (c=='i') return glyph_i;
    if (c=='l') return glyph_l;
    if (c=='n') return glyph_n;
    if (c=='o') return glyph_o;
    if (c=='p') return glyph_p;
    if (c=='r') return glyph_r;
    if (c=='s') return glyph_s;
    if (c=='t') return glyph_t;
    if (c=='w' || c=='W') return glyph_w;
    if (c=='C') return glyph_C;
    if (c=='P') return glyph_P;
    if (c=='D') return glyph_D;
    // fallback
    return glyph_space;
}

void ssd1306_draw_text(int x, int y, const char *text) {
    if (!text) return;
    if (x >= SSD1306_WIDTH) return;
    if (y % 8 != 0) return; // only support multiples of 8 for simplicity
    int page = y / 8;
    int col = x;
    for (const char *p = text; *p && col < SSD1306_WIDTH-6; p++) {
        const uint8_t *g = get_glyph(*p);
        for (int i=0;i<5;i++) {
            if (col >= 0 && col < SSD1306_WIDTH) {
                buffer[page*SSD1306_WIDTH + col] = g[i];
            }
            col++;
        }
        // one pixel spacing
        if (col >=0 && col < SSD1306_WIDTH) buffer[page*SSD1306_WIDTH + col] = 0x00;
        col++;
    }
}
