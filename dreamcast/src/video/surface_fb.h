#ifndef SURFACE_FB_H
#define SURFACE_FB_H
#include "text_surface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SURFACE_FB_COLS 80
#define SURFACE_FB_ROWS 30

#define SURFACE_FB_INSET_DEFAULT 2
#define SURFACE_FB_INSET_MAX     6

text_surface_t surface_fb_make(void);

void surface_fb_flush(void);

void surface_fb_recolour(void);

void surface_fb_nudge(int delta);

void surface_fb_toggle_mode(void);

int surface_fb_is_480(void);

int surface_fb_cols(void);

int surface_fb_rows(void);

int surface_fb_inset(void);

#ifdef __cplusplus
}
#endif
#endif
