#include "colour_theme.h"

static const uint16_t ANSI_INK[16] = {
    0x8000, 0x8014, 0x8280, 0x8294, 0xd000, 0xd014, 0xd280, 0xdef7,
    0xb18c, 0x801f, 0x83e0, 0x83ff, 0xfc00, 0xfc1f, 0xffe0, 0xffff
};

static const unsigned char LEVEL[16] = {
    0, 16, 18, 20, 16, 18, 20, 22, 12, 26, 28, 30, 26, 28, 30, 31
};

static const unsigned char TINT[COLOUR_THEME_COUNT][3] = {
    { 0, 0, 0 },
    { 4, 16, 4 },
    { 16, 10, 0 },
    { 16, 16, 16 },
    { 5, 13, 16 }
};

static uint16_t g_ink[16];

const uint16_t *colour_theme_ink(int theme)
{
    int i;

    if (theme <= 0 || theme >= COLOUR_THEME_COUNT) return ANSI_INK;
    for (i = 0; i < 16; i++) {
        unsigned r = (unsigned)(LEVEL[i] * TINT[theme][0] / 16);
        unsigned g = (unsigned)(LEVEL[i] * TINT[theme][1] / 16);
        unsigned b = (unsigned)(LEVEL[i] * TINT[theme][2] / 16);
        g_ink[i] = (uint16_t)(0x8000 | (b << 10) | (g << 5) | r);
    }
    return g_ink;
}

int colour_theme_next(int theme)
{
    return (theme + 1) % COLOUR_THEME_COUNT;
}
