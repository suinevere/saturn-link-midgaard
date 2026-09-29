#include <dc/maple.h>
#include <dc/maple/keyboard.h>
#include "dc_keyboard.h"

#define DC_REPEAT_START_MS    500
#define DC_REPEAT_INTERVAL_MS 67

void dc_keyboard_init(void)
{
    kbd_set_repeat_timing(DC_REPEAT_START_MS, DC_REPEAT_INTERVAL_MS);
}

static CmudKeyEvent event(CmudKeyKind kind, char ch)
{
    CmudKeyEvent ev;
    ev.kind = kind;
    ev.ch = ch;
    return ev;
}

static CmudKeyKind special(int key, int ctrl)
{
    switch (key) {
    case KBD_KEY_ENTER:
    case KBD_KEY_PAD_ENTER: return CMUD_KEY_ENTER;
    case KBD_KEY_BACKSPACE: return CMUD_KEY_BACKSPACE;
    case KBD_KEY_TAB:       return CMUD_KEY_TAB;
    case KBD_KEY_ESCAPE:    return CMUD_KEY_ESCAPE;
    case KBD_KEY_DEL:       return CMUD_KEY_DELETE;
    case KBD_KEY_HOME:      return CMUD_KEY_HOME;
    case KBD_KEY_END:       return CMUD_KEY_END;
    case KBD_KEY_PGUP:      return CMUD_KEY_PAGEUP;
    case KBD_KEY_PGDOWN:    return CMUD_KEY_PAGEDOWN;
    case KBD_KEY_LEFT:      return ctrl ? CMUD_KEY_CTRL_LEFT  : CMUD_KEY_LEFT;
    case KBD_KEY_RIGHT:     return ctrl ? CMUD_KEY_CTRL_RIGHT : CMUD_KEY_RIGHT;
    case KBD_KEY_UP:        return ctrl ? CMUD_KEY_CTRL_UP    : CMUD_KEY_UP;
    case KBD_KEY_DOWN:      return ctrl ? CMUD_KEY_CTRL_DOWN  : CMUD_KEY_DOWN;
    case KBD_KEY_F4:        return CMUD_KEY_F4;
    case KBD_KEY_F5:        return CMUD_KEY_F5;
    case KBD_KEY_F6:        return CMUD_KEY_F6;
    case KBD_KEY_F7:        return CMUD_KEY_F7;
    case KBD_KEY_F8:        return CMUD_KEY_F8;
    case KBD_KEY_F9:        return CMUD_KEY_F9;
    case KBD_KEY_F10:       return CMUD_KEY_F10;
    case KBD_KEY_F11:       return CMUD_KEY_F11;
    case KBD_KEY_F12:       return CMUD_KEY_F12;
    default:                return CMUD_KEY_NONE;
    }
}

CmudKeyEvent dc_keyboard_poll(void)
{
    maple_device_t *dev = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);
    kbd_state_t *state;
    kbd_mods_t mods;
    kbd_leds_t leds;
    CmudKeyKind kind;
    int raw, key, ctrl;
    char c;

    if (!dev) return event(CMUD_KEY_NONE, 0);
    raw = kbd_queue_pop(dev, false);
    if (raw == KBD_QUEUE_END) return event(CMUD_KEY_NONE, 0);

    key = raw & 0xff;
    mods.raw = (uint8_t)((raw >> 8) & 0xff);
    leds.raw = (uint8_t)((raw >> 16) & 0xff);
    ctrl = (mods.raw & KBD_MOD_CTRL) != 0;

    kind = special(key, ctrl);
    if (kind != CMUD_KEY_NONE) return event(kind, 0);
    if (ctrl) return event(key == KBD_KEY_C ? CMUD_KEY_CLEAR : CMUD_KEY_NONE, 0);

    state = kbd_get_state(dev);
    c = kbd_key_to_ascii((kbd_key_t)key, state ? state->region : KBD_REGION_US, mods, leds);
    if (c < 0x20 || c > 0x7e) return event(CMUD_KEY_NONE, 0);
    return event(CMUD_KEY_CHAR, c);
}
