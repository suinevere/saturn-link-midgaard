#ifndef COLOUR_THEME_H
#define COLOUR_THEME_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COLOUR_THEME_COUNT 5

const uint16_t *colour_theme_ink(int theme);

int colour_theme_next(int theme);

#ifdef __cplusplus
}
#endif
#endif
