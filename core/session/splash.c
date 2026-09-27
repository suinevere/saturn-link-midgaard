#include <string.h>
#include "splash.h"
#include "console.h"
#include "cell_attr.h"

#define SPLASH_ART_WIDTH 38

static const char *const SPLASH_ART[] = {
    "              _.--------._",
    "         _.-'      /\\   +  '-._",
    "      .-'  +      /**\\         '-.",
    "    .'    /\\     /.* *\\   +       '.",
    "   /     /**\\   /   .  \\    /\\      \\",
    "  |     /*.* \\ /  .   . \\  /**\\      |",
    "  |    /.   . V .   .   .\\/ * *\\     |",
    "  |   /   .   .   .   .   .   . \\    |",
    "   \\   ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^    /",
    "    './^\\ /^\\ /^\\ /^\\ /^\\ /^\\ /^\\ .'",
    "      '-.__|___|___|___|___|___.-'",
    "         '-._~~~~~~~~~~~~~~_.-'",
    "              '----------'"
};

static const char *const SPLASH_INK[] = {
    "              rrrrrrrrrrrr",
    "         rrrr      mm   y  rrrr",
    "      rrr  y      mssm         rrr",
    "    rr    mm     mms sm   y       rr",
    "   r     mssm   m   m  m    mm      r",
    "  r     msms m m  m   m m  mssm      r",
    "  r    mm   m m m   m   mmm s sm     r",
    "  r   m   m   m   m   m   m   m m    r",
    "   r   G g G g G g G g G g G g G    r",
    "    rrggg ggg ggg ggg ggg ggg ggg rr",
    "      rrrbbbbbbbbbbbbbbbbbbbbbbrrr",
    "         rrrrwcwcwcwcwcwcwcrrrr",
    "              rrrrrrrrrrrr"
};

static const char SPLASH_ABOUT[] =
    "CoffeeMUD, rehosted for the Sega Saturn and Dreamcast. Explore a "
    "high-fantasy sandbox in Midgaard: 97 areas and nearly 7,000 rooms, "
    "with dynamic weather and open-ocean naval combat. Gather and craft, "
    "conquer dungeons, build an economic empire or command a fleet, as one "
    "of 25 races.";

static void indent(int width, int cols)
{
    char pad[CONSOLE_COLS];
    int n = (cols - width) / 2;

    if (n < 0) n = 0;
    if (n > CONSOLE_COLS) n = CONSOLE_COLS;
    memset(pad, ' ', (size_t)n);
    console_write(pad, (unsigned int)n);
}

static void centred(const char *s, int width, int cols, unsigned char attr)
{
    indent(width, cols);
    console_write_as(s, (unsigned int)strlen(s), attr);
    console_write("\n", 1);
}

static unsigned char ink_of(char k)
{
    switch (k) {
    case 'r': return CELL_ATTR_NOTE;
    case 's': return ANSI_ATTR_BOLD | 7;
    case 'y': return ANSI_ATTR_BOLD | 3;
    case 'G': return ANSI_ATTR_BOLD | 2;
    case 'g': return 2;
    case 'b': return 3;
    case 'w': return ANSI_ATTR_BOLD | 4;
    case 'c': return ANSI_ATTR_BOLD | 6;
    default:  return ANSI_ATTR_DEFAULT;
    }
}

static void art_row(const char *art, const char *ink, int cols)
{
    unsigned char at[CONSOLE_COLS];
    int n = (int)strlen(art);
    int i;

    for (i = 0; i < n && i < CONSOLE_COLS; i++) at[i] = ink_of(ink[i]);
    indent(SPLASH_ART_WIDTH, cols);
    console_write_attr(art, at, (unsigned int)i);
    console_write("\n", 1);
}

void splash_write(int cols)
{
    unsigned int i;

    for (i = 0; i < sizeof SPLASH_ART / sizeof SPLASH_ART[0]; i++) {
        art_row(SPLASH_ART[i], SPLASH_INK[i], cols);
    }
    console_write("\n", 1);
    centred(SPLASH_TITLE, (int)(sizeof SPLASH_TITLE - 1), cols,
            ANSI_ATTR_BOLD | ANSI_ATTR_DEFAULT);
    console_write("\n", 1);
    console_write(SPLASH_ABOUT, (unsigned int)(sizeof SPLASH_ABOUT - 1));
    console_write("\n\n", 2);
}
