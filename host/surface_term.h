#ifndef SURFACE_TERM_H
#define SURFACE_TERM_H
#include "text_surface.h"

text_surface_t surface_term_make(int cols, int rows);

void surface_term_raw(void);
void surface_term_restore(void);

int surface_term_getkey(void);

void surface_term_sleep_frame(void);

#endif
