#ifndef DC_KEYBOARD_H
#define DC_KEYBOARD_H
#include "key_event.h"

#ifdef __cplusplus
extern "C" {
#endif

void dc_keyboard_init(void);

CmudKeyEvent dc_keyboard_poll(void);

#ifdef __cplusplus
}
#endif
#endif
