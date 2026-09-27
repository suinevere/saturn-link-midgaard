#include <srl.hpp>
#include <string.h>
#include "surface_ascii.h"
#include "cell_attr.h"
#include "box_glyphs.h"
#include "box_tiles.h"
#include "colour_theme.h"

#define SA_PAGE_COLS 64

#define SA_PAGE_A ((uint16_t *)(VDP2_VRAM_B1 + 0x1E000))
#define SA_PAGE_B ((uint16_t *)(VDP2_VRAM_B1 + 0x18000))

#define SA_SCRATCH_ROW 60

static uint16_t sa_base;

static char sa_shadow[SURFACE_ASCII_ROWS][SURFACE_ASCII_COLS_MAX];
static char sa_live[SURFACE_ASCII_ROWS][SURFACE_ASCII_COLS_MAX];
static unsigned char sa_shadow_at[SURFACE_ASCII_ROWS][SURFACE_ASCII_COLS_MAX];
static unsigned char sa_live_at[SURFACE_ASCII_ROWS][SURFACE_ASCII_COLS_MAX];
static char sa_dirty[SURFACE_ASCII_ROWS];

static int sa_cols_now = SURFACE_ASCII_COLS_NARROW;
static int sa_gutter   = 0;
static int sa_wide     = 0;
static int sa_mode_pending = 0;

static int sa_gutter_narrow = 0;
static int sa_gutter_wide   = 4;

#define VDP2_TVMD      (*(volatile uint16_t *)0x25f80000)
#define TVMD_HRESO     0x0007
#define TVMD_HRESO_640 0x0002
#define TVMD_HRESO_704 0x0003

#define VDP2_RAMCTL_REG (*(volatile uint16_t *)0x25f8000e)

#define SA_PALETTE_INK(p) (((uint16_t *)(VDP2_COLRAM + ((p) << 5)))[1])

static int sa_theme_pending = 0;

static void sa_load_ink(void)
{
    const uint16_t *ink = colour_theme_ink();
    for (int p = 0; p < 16; p++) SA_PALETTE_INK(p) = ink[p];
}

static int  sa_cols(void *ctx) { (void)ctx; return sa_cols_now; }
static int  sa_rows(void *ctx) { (void)ctx; return SURFACE_ASCII_ROWS; }

static void sa_put(void *ctx, int x, int y, const char *s, const unsigned char *at)
{
    int room, i;

    (void)ctx;
    if (x < 0 || y < 0 || x >= sa_cols_now || y >= SURFACE_ASCII_ROWS) return;

    room = sa_cols_now - x;
    for (i = 0; i < room && s[i] != '\0'; i++) {
        sa_shadow[y][x + i] = s[i];
        sa_shadow_at[y][x + i] = at ? at[i] : (unsigned char) ANSI_ATTR_DEFAULT;
    }
}

static void sa_clear(void *ctx)
{
    (void)ctx;
    memset(sa_shadow, ' ', sizeof sa_shadow);
    memset(sa_shadow_at, ANSI_ATTR_DEFAULT, sizeof sa_shadow_at);
}

static void sa_flush(void)
{
    int y, x, col;

    if (sa_mode_pending) {
        VDP2_TVMD = (uint16_t)((VDP2_TVMD & ~TVMD_HRESO) |
                               (sa_wide ? TVMD_HRESO_704 : TVMD_HRESO_640));
        sa_mode_pending = 0;
    }

    if (sa_theme_pending) {
        sa_load_ink();
        sa_theme_pending = 0;
    }

    for (y = 0; y < SURFACE_ASCII_ROWS; y++) {
        if (!sa_dirty[y]) continue;
        for (col = 0; col < SURFACE_ASCII_SPAN; col++) {
            uint16_t cell;
            x = col - sa_gutter;
            if (x >= 0 && x < sa_cols_now) {
                cell = (uint16_t)(sa_base +
                           box_glyph_map((uint8_t)sa_shadow[y][x]));
                cell |= (uint16_t)(colour_theme_slot(sa_shadow_at[y][x]) << 12);
            } else {
                cell = (uint16_t)(sa_base + ' ');
            }
            if (col < SA_PAGE_COLS) SA_PAGE_A[col + (y << 6)] = cell;
            else                    SA_PAGE_B[(col - SA_PAGE_COLS) + (y << 6)] = cell;
        }
        memcpy(sa_live[y], sa_shadow[y], SURFACE_ASCII_COLS_MAX);
        memcpy(sa_live_at[y], sa_shadow_at[y], SURFACE_ASCII_COLS_MAX);
        sa_dirty[y] = 0;
    }
}

static void sa_present(void *ctx)
{
    int y;

    (void)ctx;
    for (y = 0; y < SURFACE_ASCII_ROWS; y++)
        if (memcmp(sa_shadow[y], sa_live[y], SURFACE_ASCII_COLS_MAX) != 0 ||
            memcmp(sa_shadow_at[y], sa_live_at[y], SURFACE_ASCII_COLS_MAX) != 0) sa_dirty[y] = 1;
}

extern "C" text_surface_t surface_ascii_make(void)
{
    text_surface_t s;
    int y;

    slColRAMMode(CRM16_2048);
    VDP2_RAMCTL_REG = VDP2_RAMCTL;
    slMapNbg3((void *)SA_PAGE_A, (void *)SA_PAGE_B,
              (void *)SA_PAGE_A, (void *)SA_PAGE_B);
    slScrPosNbg3(toFIXED(0.0f), toFIXED(0.0f));

    SRL::ASCII::Clear();
    for (int i = 0; i < 64 * 64; i++) SA_PAGE_B[i] = 0;

    sa_load_ink();
    for (int p = 0; p < 16; p++)
        SRL::CRAM::SetBankUsedState((uint16_t)p,
            SRL::CRAM::TextureColorMode::Paletted16, true);

    SRL::ASCII::Print("A", 0, SA_SCRATCH_ROW);
    sa_base = (uint16_t)((SA_PAGE_A[SA_SCRATCH_ROW << 6] - 'A') & 0x0fff);
    SA_PAGE_A[SA_SCRATCH_ROW << 6] = 0;

    box_tiles_install((unsigned char *)(VDP2_VRAM_B1 + 0x18000 + sa_base * 0x20));

    s.cols = sa_cols;
    s.rows = sa_rows;
    s.put = sa_put;
    s.clear = sa_clear;
    s.present = sa_present;
    s.ctx = 0;

    sa_clear(0);
    memset(sa_live, 0, sizeof sa_live);
    memset(sa_live_at, 0xff, sizeof sa_live_at);
    for (y = 0; y < SURFACE_ASCII_ROWS; y++) sa_dirty[y] = 1;
    sa_flush();

    SRL::Core::OnAfterSync += &sa_flush;
    return s;
}

extern "C" void surface_ascii_set_wide(int wide)
{
    int y;

    wide = wide ? 1 : 0;
    if (wide == sa_wide) return;

    sa_wide = wide;
    sa_cols_now = wide ? SURFACE_ASCII_COLS_WIDE : SURFACE_ASCII_COLS_NARROW;
    sa_gutter = wide ? sa_gutter_wide : sa_gutter_narrow;
    sa_mode_pending = 1;

    for (y = 0; y < SURFACE_ASCII_ROWS; y++) sa_dirty[y] = 1;
}

extern "C" int surface_ascii_is_wide(void) { return sa_wide; }
extern "C" int surface_ascii_cols(void)    { return sa_cols_now; }

extern "C" void surface_ascii_nudge(int delta)
{
    int g = sa_gutter + delta;
    int max = SURFACE_ASCII_SPAN - sa_cols_now;
    int y;

    if (g < 0)   g = 0;
    if (g > max) g = max;
    if (g == sa_gutter) return;

    sa_gutter = g;
    if (sa_wide) sa_gutter_wide = g; else sa_gutter_narrow = g;

    for (y = 0; y < SURFACE_ASCII_ROWS; y++) sa_dirty[y] = 1;
}

extern "C" int surface_ascii_gutter(void) { return sa_gutter; }

extern "C" void surface_ascii_recolour(void)
{
    sa_theme_pending = 1;
}
