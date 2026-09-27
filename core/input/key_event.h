#ifndef KEY_EVENT_H
#define KEY_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CMUD_KEY_NONE = 0,
    CMUD_KEY_CHAR,
    CMUD_KEY_BACKSPACE,
    CMUD_KEY_ENTER,
    CMUD_KEY_ESCAPE,
    CMUD_KEY_LEFT,
    CMUD_KEY_RIGHT,
    CMUD_KEY_UP,
    CMUD_KEY_DOWN,
    CMUD_KEY_HOME,
    CMUD_KEY_END,
    CMUD_KEY_PAGEUP,
    CMUD_KEY_PAGEDOWN,
    CMUD_KEY_F5,
    CMUD_KEY_F6,
    CMUD_KEY_F7,
    CMUD_KEY_F8,
    CMUD_KEY_F9,
    CMUD_KEY_F10,
    CMUD_KEY_F11,
    CMUD_KEY_F12,
    CMUD_KEY_TAB,
    CMUD_KEY_CLEAR,
    CMUD_KEY_CTRL_LEFT,
    CMUD_KEY_CTRL_RIGHT,
    CMUD_KEY_CTRL_UP,
    CMUD_KEY_CTRL_DOWN,
    CMUD_KEY_DELETE
} CmudKeyKind;

typedef struct {
    CmudKeyKind kind;
    char ch;
} CmudKeyEvent;

#ifdef __cplusplus
}
#endif

#endif
