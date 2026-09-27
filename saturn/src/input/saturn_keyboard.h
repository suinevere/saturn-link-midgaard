#ifndef SATURN_KEYBOARD_H
#define SATURN_KEYBOARD_H
#include "key_event.h"

#ifdef __cplusplus
extern "C" {
#endif

int saturn_keyboard_present(void);
int saturn_keyboard_any_down(void);
CmudKeyEvent saturn_keyboard_poll(void);

void keyboard_set_caps(int on);
int  keyboard_get_caps(void);
void keyboard_set_num(int on);
int  keyboard_get_num(void);
void keyboard_set_insert(int on);
int  keyboard_get_insert(void);
void keyboard_set_scrolllock(int on);
int  keyboard_get_scrolllock(void);

#ifdef __cplusplus
}
#endif

#endif
