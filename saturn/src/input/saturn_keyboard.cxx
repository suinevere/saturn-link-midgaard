#include <srl.hpp>
#include "saturn_keyboard.h"

static int g_caps;
static int g_num = 1;
static int g_insert;
static int g_scrolllock;

extern "C" void keyboard_set_caps(int on)       { g_caps = on; }
extern "C" int  keyboard_get_caps(void)         { return g_caps; }
extern "C" void keyboard_set_num(int on)        { g_num = on; }
extern "C" int  keyboard_get_num(void)          { return g_num; }
extern "C" void keyboard_set_insert(int on)     { g_insert = on; }
extern "C" int  keyboard_get_insert(void)       { return g_insert; }
extern "C" void keyboard_set_scrolllock(int on) { g_scrolllock = on; }
extern "C" int  keyboard_get_scrolllock(void)   { return g_scrolllock; }

#define KBD_ID          0x34
#define KBD_COND_MAKE   0x08
#define KBD_COND_BREAK  0x01
#define KBD_CODE_LCTRL  0x14
#define KBD_CODE_LSHIFT 0x12
#define KBD_CODE_RSHIFT 0x59
#define KBD_CODE_CAPS   0x58
#define KBD_CODE_NUM    0x77
#define KBD_CODE_INSERT 129
#define KBD_CODE_SCRLK  0x7E
#define KBD_OFF_ID      0
#define KBD_OFF_COND    8
#define KBD_OFF_CODE    9

#define KBD_REPEAT_DELAY 30
#define KBD_REPEAT_RATE  4

static const char kbd_map[128] = {
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,  '`',  0,   0,   0,   0,   0,
     0,  'q', '1',  0,   0,   0,  'z', 's', 'a', 'w',
     '2', 0,   0,  'c', 'x', 'd', 'e', '4', '3',  0,
     0,  ' ', 'v', 'f', 't', 'r', '5',  0,   0,  'n',
     'b', 'h', 'g', 'y', '6',  0,   0,   0,  'm', 'j',
     'u', '7', '8',  0,   0,  ',', 'k', 'i', 'o', '0',
     '9', 0,   0,  '.', '/', 'l', ';', 'p', '-',  0,
     0,   0,  '\'', 0,  '[', '=',  0,   0,   0,   0,
     0,  ']',  0,  '\\', 0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,  '1',  0,  '4', '7',  0,
     0,   0,  '0', '.', '2', '5', '6', '8',  0,   0,
     0,  '+', '3', '-', '*', '9',  0,   0
};

static char apply_mods(char c, int shift, int caps) {
    if (c >= 'a' && c <= 'z') {
        return (shift ^ caps) ? (char)(c - 'a' + 'A') : c;
    }
    if (shift) {
        switch (c) {
            case '1': return '!'; case '2': return '@'; case '3': return '#';
            case '4': return '$'; case '5': return '%'; case '6': return '^';
            case '7': return '&'; case '8': return '*'; case '9': return '(';
            case '0': return ')'; case '`': return '~'; case '-': return '_';
            case '=': return '+'; case '[': return '{'; case ']': return '}';
            case '\\': return '|'; case ';': return ':'; case '\'': return '"';
            case ',': return '<'; case '.': return '>'; case '/': return '?';
        }
    }
    return c;
}

static int is_numpad_digit(uint8_t code) {
    switch (code) {
        case 105: case 107: case 108: case 112: case 113:
        case 114: case 115: case 116: case 117: case 122: case 125:
            return 1;
        default: return 0;
    }
}

static int find_keyboard_port(void) {
    for (int p = 0; p < 12; p++) {
        const uint8_t *raw = (const uint8_t *) SRL::Input::Management::GetRawData(p);
        if (raw != nullptr && raw[KBD_OFF_ID] == KBD_ID) {
            return p;
        }
    }
    return -1;
}

extern "C" int saturn_keyboard_present(void) {
    return find_keyboard_port() >= 0 ? 1 : 0;
}

static int kbd_is_modifier(uint8_t code)
{
    return code == KBD_CODE_LCTRL  || code == KBD_CODE_LSHIFT ||
           code == KBD_CODE_RSHIFT || code == KBD_CODE_CAPS   ||
           code == KBD_CODE_NUM    || code == KBD_CODE_SCRLK  ||
           code == KBD_CODE_INSERT;
}

extern "C" int saturn_keyboard_any_down(void) {
    int port = find_keyboard_port();
    if (port < 0) return 0;
    const uint8_t *raw = (const uint8_t *) SRL::Input::Management::GetRawData(port);
    return (raw[KBD_OFF_COND] & KBD_COND_MAKE) != 0 && raw[KBD_OFF_CODE] != 0;
}

extern "C" CmudKeyEvent saturn_keyboard_poll(void) {
    static uint8_t held_code = 0;
    static int repeat_timer = 0;
    static int ctrl_down = 0;
    static int shift_down = 0;
    static int caps_held = 0;
    static int num_held  = 0;
    static int insert_held = 0;
    static int scrl_held = 0;

    CmudKeyEvent ev;
    ev.kind = CMUD_KEY_NONE;
    ev.ch = 0;

    int port = find_keyboard_port();
    if (port < 0) { held_code = 0; ctrl_down = 0; return ev; }

    const uint8_t *raw = (const uint8_t *) SRL::Input::Management::GetRawData(port);
    uint8_t cond = raw[KBD_OFF_COND];
    uint8_t code = raw[KBD_OFF_CODE];

    if (code == KBD_CODE_LCTRL) {
        if (cond & KBD_COND_MAKE)  ctrl_down = 1;
        if (cond & KBD_COND_BREAK) ctrl_down = 0;
    }
    if (code == KBD_CODE_LSHIFT || code == KBD_CODE_RSHIFT) {
        if (cond & KBD_COND_MAKE)  shift_down = 1;
        if (cond & KBD_COND_BREAK) shift_down = 0;
    }
    if (code == KBD_CODE_CAPS) {
        if (cond & KBD_COND_MAKE)  { if (!caps_held) { keyboard_set_caps(!keyboard_get_caps()); caps_held = 1; } }
        if (cond & KBD_COND_BREAK) { caps_held = 0; }
    }
    if (code == KBD_CODE_NUM) {
        if (cond & KBD_COND_MAKE)  { if (!num_held) { keyboard_set_num(!keyboard_get_num()); num_held = 1; } }
        if (cond & KBD_COND_BREAK) { num_held = 0; }
    }
    if (code == KBD_CODE_INSERT) {
        if (cond & KBD_COND_MAKE)  { if (!insert_held) { keyboard_set_insert(!keyboard_get_insert()); insert_held = 1; } }
        if (cond & KBD_COND_BREAK) { insert_held = 0; }
    }
    if (code == KBD_CODE_SCRLK) {
        if (cond & KBD_COND_MAKE)  { if (!scrl_held) { keyboard_set_scrolllock(!keyboard_get_scrolllock()); scrl_held = 1; } }
        if (cond & KBD_COND_BREAK) { scrl_held = 0; }
    }

    int fresh = 0;
    if (!kbd_is_modifier(code)) {
        if (cond & KBD_COND_BREAK) {
            if (code == held_code) held_code = 0;
        } else if ((cond & KBD_COND_MAKE) && code != 0 && code != held_code) {
            held_code = code;
            repeat_timer = KBD_REPEAT_DELAY;
            fresh = 1;
        }
    }

    if (held_code == 0) return ev;
    if (!fresh) {
        if (--repeat_timer > 0) return ev;
        repeat_timer = KBD_REPEAT_RATE;
    }

    code = held_code;

    if (code == 90 || code == 25) { ev.kind = CMUD_KEY_ENTER; return ev; }
    if (code == 13)               { ev.kind = CMUD_KEY_TAB; return ev; }
    if (code == 102)              { ev.kind = CMUD_KEY_BACKSPACE; return ev; }
    if (code == 118)              { ev.kind = CMUD_KEY_ESCAPE; return ev; }
    if (code == 134)              { ev.kind = ctrl_down ? CMUD_KEY_CTRL_LEFT  : CMUD_KEY_LEFT;  return ev; }
    if (code == 141)              { ev.kind = ctrl_down ? CMUD_KEY_CTRL_RIGHT : CMUD_KEY_RIGHT; return ev; }
    if (code == 137)              { ev.kind = ctrl_down ? CMUD_KEY_CTRL_UP   : CMUD_KEY_UP;    return ev; }
    if (code == 138)              { ev.kind = ctrl_down ? CMUD_KEY_CTRL_DOWN : CMUD_KEY_DOWN;  return ev; }
    if (code == 133)              { ev.kind = CMUD_KEY_DELETE;   return ev; }
    if (code == 128)              { ev.kind = CMUD_KEY_CHAR; ev.ch = '/'; return ev; }
    if (code == 135)              { ev.kind = CMUD_KEY_HOME;     return ev; }
    if (code == 136)              { ev.kind = CMUD_KEY_END;      return ev; }
    if (code == 139)              { ev.kind = CMUD_KEY_PAGEUP;   return ev; }
    if (code == 140)              { ev.kind = CMUD_KEY_PAGEDOWN; return ev; }

    if (code == 0x0C)             { ev.kind = CMUD_KEY_F4;  return ev; }
    if (code == 0x03)             { ev.kind = CMUD_KEY_F5;  return ev; }
    if (code == 0x0B)             { ev.kind = CMUD_KEY_F6;  return ev; }
    if (code == 0x83)             { ev.kind = CMUD_KEY_F7;  return ev; }
    if (code == 0x0A)             { ev.kind = CMUD_KEY_F8;  return ev; }
    if (code == 0x01)             { ev.kind = CMUD_KEY_F9;  return ev; }
    if (code == 0x09)             { ev.kind = CMUD_KEY_F10; return ev; }
    if (code == 0x78)             { ev.kind = CMUD_KEY_F11; return ev; }
    if (code == 0x07)             { ev.kind = CMUD_KEY_F12; return ev; }
    if (ctrl_down && code < 128 && kbd_map[code] == 'c') { ev.kind = CMUD_KEY_CLEAR; return ev; }
    if (is_numpad_digit(code) && !keyboard_get_num()) return ev;
    if (code < 128 && kbd_map[code] != 0) {
        ev.kind = CMUD_KEY_CHAR;
        ev.ch = apply_mods(kbd_map[code], shift_down, keyboard_get_caps());
    }
    return ev;
}
