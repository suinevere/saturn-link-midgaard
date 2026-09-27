#ifndef SURFACE_ASCII_H
#define SURFACE_ASCII_H
#include "text_surface.h"

#define SURFACE_ASCII_COLS_NARROW 78
#define SURFACE_ASCII_COLS_WIDE   80
#define SURFACE_ASCII_COLS_MAX    80

#define SURFACE_ASCII_SPAN 88

#define SURFACE_ASCII_ROWS 30

#ifdef __cplusplus
extern "C" {
#endif

text_surface_t surface_ascii_make(void);

void surface_ascii_nudge(int delta);
int  surface_ascii_gutter(void);

void surface_ascii_set_wide(int wide);
int  surface_ascii_is_wide(void);
int  surface_ascii_cols(void);

void surface_ascii_recolour(void);

#ifdef __cplusplus
}
#endif
#endif
