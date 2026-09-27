#include "box_tiles.h"
#include "box_glyphs.h"

void box_tiles_install(unsigned char *tile0)
{
    int g, row, pair;

    for (g = 0; g < BOX_GLYPH_COUNT; g++) {
        const unsigned char *rows = box_glyph_rows((unsigned char)(BOX_GLYPH_FIRST + g));
        unsigned char *tile = tile0 + (BOX_GLYPH_FIRST + g) * BOX_TILE_BYTES;
        for (row = 0; row < 8; row++) {
            unsigned char bits = rows[row];
            for (pair = 0; pair < 4; pair++) {
                unsigned char left  = (bits & (0x80 >> (pair * 2))) ? 0x10 : 0x00;
                unsigned char right = (bits & (0x40 >> (pair * 2))) ? 0x01 : 0x00;
                tile[row * 4 + pair] = (unsigned char)(left | right);
            }
        }
    }
}
