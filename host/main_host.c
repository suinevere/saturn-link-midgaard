#include <stdio.h>
#include <stdlib.h>
#include "session.h"
#include "transport_tcp.h"
#include "surface_term.h"

#define HOST_COLS 78
#define HOST_ROWS 30
#define HOST_MARGIN_TOP    2
#define HOST_MARGIN_BOTTOM 2

#define HOST_TEXT_ROWS (HOST_ROWS - HOST_MARGIN_TOP - HOST_MARGIN_BOTTOM - 1)

static int             g_fd = -1;
static cui_transport_t g_tr;
static const char     *g_host;
static int             g_port;

static void quit(int status)
{
    surface_term_restore();
    transport_tcp_close(g_fd);
    exit(status);
}

static CmudKeyEvent event(CmudKeyKind kind, char ch)
{
    CmudKeyEvent ev;
    ev.kind = kind;
    ev.ch = ch;
    return ev;
}

static CmudKeyEvent function_key(int lead)
{
    int n = (lead - '0') * 10 + (surface_term_getkey() - '0');

    surface_term_getkey();
    switch (n) {
    case 15: return event(CMUD_KEY_F5, 0);
    case 17: return event(CMUD_KEY_F6, 0);
    case 18: return event(CMUD_KEY_F7, 0);
    case 19: return event(CMUD_KEY_F8, 0);
    case 20: return event(CMUD_KEY_F9, 0);
    default: return event(CMUD_KEY_NONE, 0);
    }
}

static CmudKeyEvent escape_sequence(void)
{
    int c = surface_term_getkey();
    int d;

    if (c != '[') return event(CMUD_KEY_ESCAPE, 0);
    d = surface_term_getkey();
    switch (d) {
    case 'A': return event(CMUD_KEY_UP, 0);
    case 'B': return event(CMUD_KEY_DOWN, 0);
    case 'C': return event(CMUD_KEY_RIGHT, 0);
    case 'D': return event(CMUD_KEY_LEFT, 0);
    case 'H': return event(CMUD_KEY_HOME, 0);
    case 'F': return event(CMUD_KEY_END, 0);
    case '1':
    case '2': return function_key(d);
    case '3': surface_term_getkey(); return event(CMUD_KEY_DELETE, 0);
    case '5': surface_term_getkey(); return event(CMUD_KEY_PAGEUP, 0);
    case '6': surface_term_getkey(); return event(CMUD_KEY_PAGEDOWN, 0);
    default:  return event(CMUD_KEY_NONE, 0);
    }
}

#ifdef _WIN32
static CmudKeyEvent console_extended(void)
{
    switch (surface_term_getkey()) {
    case 72: return event(CMUD_KEY_UP, 0);
    case 80: return event(CMUD_KEY_DOWN, 0);
    case 77: return event(CMUD_KEY_RIGHT, 0);
    case 75: return event(CMUD_KEY_LEFT, 0);
    case 71: return event(CMUD_KEY_HOME, 0);
    case 79: return event(CMUD_KEY_END, 0);
    case 83: return event(CMUD_KEY_DELETE, 0);
    case 73: return event(CMUD_KEY_PAGEUP, 0);
    case 81: return event(CMUD_KEY_PAGEDOWN, 0);
    case 63: return event(CMUD_KEY_F5, 0);
    case 64: return event(CMUD_KEY_F6, 0);
    case 65: return event(CMUD_KEY_F7, 0);
    case 66: return event(CMUD_KEY_F8, 0);
    case 67: return event(CMUD_KEY_F9, 0);
    default: return event(CMUD_KEY_NONE, 0);
    }
}
#endif

static CmudKeyEvent poll_key(void)
{
    int c = surface_term_getkey();

    if (c < 0) return event(CMUD_KEY_NONE, 0);
    if (c == 3) quit(0);
    if (c == '\r' || c == '\n') return event(CMUD_KEY_ENTER, 0);
    if (c == 127 || c == 8) return event(CMUD_KEY_BACKSPACE, 0);
    if (c == '\t') return event(CMUD_KEY_TAB, 0);
    if (c == 27) return escape_sequence();
#ifdef _WIN32
    if (c == 0 || c == 0xe0) return console_extended();
#endif
    if (c >= 32 && c < 127) return event(CMUD_KEY_CHAR, (char)c);
    return event(CMUD_KEY_NONE, 0);
}

static const cui_transport_t *open_link(void)
{
    g_fd = transport_tcp_open(g_host, g_port);
    if (g_fd < 0) {
        session_say("CANNOT CONNECT");
        return 0;
    }
    g_tr = transport_tcp_make(g_fd);
    return &g_tr;
}

static void close_link(void)
{
    quit(0);
}

int main(int argc, char **argv)
{
    static text_surface_t s;
    static session_platform_t plat;
    g_host = (argc > 1) ? argv[1] : "127.0.0.1";
    g_port = (argc > 2) ? atoi(argv[2]) : 5555;

    s = surface_term_make(HOST_COLS, HOST_ROWS);
    surface_term_raw();

    plat.surface       = &s;
    plat.text_rows     = HOST_TEXT_ROWS;
    plat.margin_top    = HOST_MARGIN_TOP;
    plat.margin_bottom = HOST_MARGIN_BOTTOM;
    plat.poll_key      = poll_key;
    plat.wait_frame    = surface_term_sleep_frame;
    plat.open          = open_link;
    plat.close         = close_link;
    session_init(&plat);
    session_splash();

    session_run();
    return 0;
}
