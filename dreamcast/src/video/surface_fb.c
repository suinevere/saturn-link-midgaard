#include <string.h>
#include <stdint.h>
#include <dc/video.h>
#include "minifont.h"
#include "font8x8.h"
#include "surface_fb.h"
#include "box_glyphs.h"
#include "cell_attr.h"
#include "colour_theme.h"

#define FB_CELL_W 8
#define FB_STRIDE 640

static int fb_cell_h = 16;

static vid_mode_t FB_TV_240P = {
    .generic = DM_320x240, .width = 640, .height = 240, .flags = 0,
    .cable_type = CT_ANY, .pm = PM_RGB565,
    .scanlines = 262, .clocks = 857, .bitmapx = 164, .bitmapy = 24,
    .scanint1 = 21, .scanint2 = 260, .borderx1 = 141, .borderx2 = 843,
    .bordery1 = 24, .bordery2 = 263,
    .fb_curr = 0, .fb_count = 1, .fb_size = 640 * 240 * 2
};

#define FROM555(v) ((uint16_t)((((v) & 31) << 11) | ((((v) >> 5) & 31) << 6) | \
                               (((((v) >> 5) & 31) >> 4) << 5) | (((v) >> 10) & 31)))

static uint16_t fb_ink[16];
static int      fb_480;
static int      fb_left;
static int      fb_inset;
static int      fb_inset_480 = SURFACE_FB_INSET_DEFAULT;
static int      fb_inset_240 = SURFACE_FB_INSET_DEFAULT;

static char          fb_shadow[SURFACE_FB_ROWS][SURFACE_FB_COLS];
static unsigned char fb_shadow_at[SURFACE_FB_ROWS][SURFACE_FB_COLS];
static char          fb_live[SURFACE_FB_ROWS][SURFACE_FB_COLS];
static unsigned char fb_live_at[SURFACE_FB_ROWS][SURFACE_FB_COLS];
static unsigned char fb_dirty[SURFACE_FB_ROWS];

static unsigned char glyph_row(unsigned char c, int y)
{
    unsigned char code = box_glyph_map(c);
    const unsigned char *box = box_glyph_rows(code);

    if (fb_cell_h == 8) {
        if (box) return box[y];
        if (code < FONT8X8_FIRST || code >= FONT8X8_FIRST + FONT8X8_COUNT) return 0;
        return FONT8X8[code - FONT8X8_FIRST][y];
    }
    if (box) return box[y / 2];
    if (code < 33 || code > 126) return 0;
    return minifont_data[(code - 33) * 16 + y];
}

static void draw_cell(int cx, int cy, unsigned char c, unsigned char at)
{
    uint16_t ink = fb_ink[colour_theme_slot(at)];
    uint16_t *px = vram_s + ((cy + fb_inset) * fb_cell_h) * FB_STRIDE + (cx + fb_left) * FB_CELL_W;
    int y, x;

    for (y = 0; y < fb_cell_h; y++, px += FB_STRIDE) {
        unsigned char bits = glyph_row(c, y);
        for (x = 0; x < FB_CELL_W; x++) px[x] = (bits & (0x80 >> x)) ? ink : 0;
    }
}

static int fb_cols(void *ctx) { (void)ctx; return SURFACE_FB_COLS - fb_left; }
static int fb_rows(void *ctx) { (void)ctx; return SURFACE_FB_ROWS - 2 * fb_inset; }

static void fb_put(void *ctx, int x, int y, const char *s, const unsigned char *at)
{
    int i;

    (void)ctx;
    if (x < 0 || y < 0 || x >= SURFACE_FB_COLS - fb_left || y >= fb_rows(0)) return;
    for (i = 0; x + i < SURFACE_FB_COLS - fb_left && s[i] != '\0'; i++) {
        fb_shadow[y][x + i] = s[i];
        fb_shadow_at[y][x + i] = at ? at[i] : (unsigned char)ANSI_ATTR_DEFAULT;
    }
}

static void fb_clear(void *ctx)
{
    (void)ctx;
    memset(fb_shadow, ' ', sizeof fb_shadow);
    memset(fb_shadow_at, ANSI_ATTR_DEFAULT, sizeof fb_shadow_at);
}

static void fb_present(void *ctx)
{
    int y;

    (void)ctx;
    for (y = 0; y < fb_rows(0); y++) {
        if (memcmp(fb_shadow[y], fb_live[y], SURFACE_FB_COLS) != 0 ||
            memcmp(fb_shadow_at[y], fb_live_at[y], SURFACE_FB_COLS) != 0) fb_dirty[y] = 1;
    }
}

void surface_fb_flush(void)
{
    int y, x;

    for (y = 0; y < fb_rows(0); y++) {
        if (!fb_dirty[y]) continue;
        for (x = 0; x < SURFACE_FB_COLS - fb_left; x++) {
            if (fb_shadow[y][x] == fb_live[y][x] && fb_shadow_at[y][x] == fb_live_at[y][x]) continue;
            draw_cell(x, y, (unsigned char)fb_shadow[y][x], fb_shadow_at[y][x]);
            fb_live[y][x] = fb_shadow[y][x];
            fb_live_at[y][x] = fb_shadow_at[y][x];
        }
        fb_dirty[y] = 0;
    }
}

static void fb_redraw_all(void)
{
    memset(fb_live, 0, sizeof fb_live);
    memset(fb_live_at, 0xff, sizeof fb_live_at);
    memset(fb_dirty, 1, sizeof fb_dirty);
}

static void fb_load_ink(void)
{
    const uint16_t *ink = colour_theme_ink();
    int i;

    for (i = 0; i < 16; i++) fb_ink[i] = FROM555(ink[i]);
}

static void fb_apply_mode(void)
{
    fb_left = fb_480 ? 0 : 1;
    fb_inset = fb_480 ? fb_inset_480 : fb_inset_240;
    if (fb_480) {
        fb_cell_h = 16;
        vid_set_mode(DM_640x480, PM_RGB565);
    } else {
        fb_cell_h = 8;
        vid_set_mode_ex(&FB_TV_240P);
    }
    vid_clear(0, 0, 0);
}

int surface_fb_is_480(void) { return fb_480; }

int surface_fb_cols(void) { return SURFACE_FB_COLS - fb_left; }

int surface_fb_rows(void) { return fb_rows(0); }

int surface_fb_inset(void) { return fb_inset; }

void surface_fb_recolour(void)
{
    fb_load_ink();
    fb_redraw_all();
}

void surface_fb_nudge(int delta)
{
    int n = fb_inset + delta;

    if (n < 0) n = 0;
    if (n > SURFACE_FB_INSET_MAX) n = SURFACE_FB_INSET_MAX;
    if (n == fb_inset) return;

    fb_inset = n;
    if (fb_480) fb_inset_480 = n; else fb_inset_240 = n;
    fb_clear(0);
    vid_clear(0, 0, 0);
    fb_redraw_all();
}

void surface_fb_toggle_mode(void)
{
    fb_480 = !fb_480;
    fb_apply_mode();
    fb_redraw_all();
}

text_surface_t surface_fb_make(void)
{
    text_surface_t s;

    fb_480 = vid_check_cable() == CT_VGA;
    fb_load_ink();
    fb_apply_mode();
    fb_clear(0);
    memset(fb_live, 0, sizeof fb_live);
    memset(fb_live_at, 0, sizeof fb_live_at);
    memset(fb_dirty, 0, sizeof fb_dirty);

    s.cols = fb_cols;
    s.rows = fb_rows;
    s.put = fb_put;
    s.clear = fb_clear;
    s.present = fb_present;
    s.ctx = 0;
    return s;
}
