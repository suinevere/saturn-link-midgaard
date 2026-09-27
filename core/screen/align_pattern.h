#ifndef ALIGN_PATTERN_H
#define ALIGN_PATTERN_H
#include "text_surface.h"

#ifdef __cplusplus
extern "C" {
#endif

void align_pattern_draw(const text_surface_t *s, int wide, int gutter);

#ifdef __cplusplus
}
#endif
#endif
