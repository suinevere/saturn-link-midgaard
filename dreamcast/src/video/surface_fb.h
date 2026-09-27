#ifndef SURFACE_FB_H
#define SURFACE_FB_H
#include "text_surface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SURFACE_FB_COLS 80
#define SURFACE_FB_ROWS 30

text_surface_t surface_fb_make(void);

void surface_fb_flush(void);

void surface_fb_next_theme(void);

void surface_fb_toggle_mode(void);

int surface_fb_is_480(void);

int surface_fb_cols(void);

#ifdef __cplusplus
}
#endif
#endif
