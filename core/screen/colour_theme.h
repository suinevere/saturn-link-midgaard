#ifndef COLOUR_THEME_H
#define COLOUR_THEME_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COLOUR_ROLE_TEXT  0
#define COLOUR_ROLE_YOU   1
#define COLOUR_ROLE_NOTE  2
#define COLOUR_ROLE_MUD   3
#define COLOUR_ROLE_COUNT 4

#define COLOUR_THEME_COUNT 5
#define COLOUR_HUE_COUNT   8

void colour_theme_cycle(int role);

int colour_theme_pick(int role);

const uint16_t *colour_theme_ink(void);

unsigned char colour_theme_slot(unsigned char attr);

#ifdef __cplusplus
}
#endif
#endif
