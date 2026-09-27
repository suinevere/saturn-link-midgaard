#ifndef BOX_GLYPHS_H
#define BOX_GLYPHS_H

#define BOX_GLYPH_FIRST 1
#define BOX_GLYPH_COUNT 17

#ifdef __cplusplus
extern "C" {
#endif

unsigned char box_glyph_map(unsigned char c);

const unsigned char *box_glyph_rows(unsigned char code);

#ifdef __cplusplus
}
#endif
#endif
