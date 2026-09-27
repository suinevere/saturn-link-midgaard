#include "colour_theme.h"
#include "cell_attr.h"

#define SLOT_YOU   0
#define SLOT_TEXT  7
#define SLOT_NOTE  8
#define SLOT_BRIGHT 15

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

static const unsigned char HUE[COLOUR_HUE_COUNT][3] = {
    { 31, 31, 31 },
    { 8, 31, 8 },
    { 31, 20, 0 },
    { 4, 28, 31 },
    { 31, 31, 4 },
    { 31, 12, 31 },
    { 12, 18, 31 },
    { 31, 10, 8 }
};

static int g_pick[COLOUR_ROLE_COUNT] = { 0, 3, 4, 0 };

static uint16_t g_ink[16];

static uint16_t rgb(unsigned r, unsigned g, unsigned b)
{
    return (uint16_t)(0x8000 | (b << 10) | (g << 5) | r);
}

static uint16_t hue(int role, unsigned level)
{
    const unsigned char *h = HUE[g_pick[role]];
    return rgb(h[0] * level / 31, h[1] * level / 31, h[2] * level / 31);
}

void colour_theme_cycle(int role)
{
    int count = (role == COLOUR_ROLE_MUD) ? COLOUR_THEME_COUNT : COLOUR_HUE_COUNT;

    if (role < 0 || role >= COLOUR_ROLE_COUNT) return;
    g_pick[role] = (g_pick[role] + 1) % count;
}

int colour_theme_pick(int role)
{
    return (role < 0 || role >= COLOUR_ROLE_COUNT) ? 0 : g_pick[role];
}

const uint16_t *colour_theme_ink(void)
{
    int theme = g_pick[COLOUR_ROLE_MUD];
    int i;

    for (i = 0; i < 16; i++) {
        g_ink[i] = (theme == 0) ? ANSI_INK[i]
                 : rgb(LEVEL[i] * TINT[theme][0] / 16u,
                       LEVEL[i] * TINT[theme][1] / 16u,
                       LEVEL[i] * TINT[theme][2] / 16u);
    }
    g_ink[SLOT_TEXT]   = hue(COLOUR_ROLE_TEXT, 23);
    g_ink[SLOT_BRIGHT] = hue(COLOUR_ROLE_TEXT, 31);
    g_ink[SLOT_YOU]    = hue(COLOUR_ROLE_YOU, 31);
    g_ink[SLOT_NOTE]   = hue(COLOUR_ROLE_NOTE, 31);
    return g_ink;
}

unsigned char colour_theme_slot(unsigned char attr)
{
    unsigned char c = (unsigned char)(attr & (ANSI_ATTR_COLOUR | ANSI_ATTR_BOLD));

    if (attr & CELL_ATTR_YOU)  return SLOT_YOU;
    if (attr & CELL_ATTR_NOTE) return SLOT_NOTE;
    if ((c & ANSI_ATTR_COLOUR) == 0) return SLOT_TEXT;
    return c;
}
