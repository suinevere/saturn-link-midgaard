#include "session.h"
#include "console.h"
#include "console_view.h"
#include "align_pattern.h"
#include "line_edit.h"
#include "telnet.h"
#include "splash.h"
#include "cell_attr.h"
#include "colour_theme.h"

#define CMUD_CARRIER_LOST_FRAMES 12

#define CMUD_SPLASH_FRAMES 240

#define CMUD_NAWS_SLACK 2

#define CMUD_ESC_WINDOW 60

static const session_platform_t *g_plat;
static int g_text_rows;
static ConsoleView g_view;
static LineEdit    g_edit;
static TelnetState g_telnet;

static int g_align = 0;

static void to_console(void *ctx, const char *s, const unsigned char *at, int len)
{
    (void)ctx;
    console_write_attr(s, at, (unsigned int)len);
}

static int surface_cols(void)
{
    return g_plat->surface->cols(g_plat->surface->ctx);
}

static void paint(int masked)
{
    int wide = 0, gutter = 0;

    if (g_align) {
        if (g_plat->align_geometry) g_plat->align_geometry(&wide, &gutter);
        align_pattern_draw(g_plat->surface, wide, gutter);
    } else {
        console_view_paint(&g_view, g_plat->surface, g_edit.line, g_edit.caret, masked);
    }
}

void session_init(const session_platform_t *p)
{
    g_plat = p;
    g_text_rows = p->text_rows;
    console_init();
    console_set_cols(surface_cols());
    console_set_margins(p->margin_top, p->margin_bottom);
    console_view_init(&g_view);
    line_edit_init(&g_edit);
}

void session_say(const char *msg)
{
    unsigned int n = 0;
    while (msg[n] != '\0') n++;
    console_write_as(msg, n, CELL_ATTR_NOTE);
    console_write("\n", 1);
}

void session_splash(void)
{
    int f;

    splash_write(surface_cols());
    for (f = 0; f < CMUD_SPLASH_FRAMES; f++) {
        CmudKeyEvent ev = g_plat->poll_key();
        paint(0);
        g_plat->wait_frame();
        if (ev.kind != CMUD_KEY_NONE) return;
    }
}

static void recolour(int role)
{
    colour_theme_cycle(role);
    if (g_plat->recolour) g_plat->recolour();
}

void session_present(void)
{
    paint(0);
    g_plat->wait_frame();
}

void session_resize(int cols)
{
    console_set_cols(cols);
    telnet_resize(&g_telnet, cols + CMUD_NAWS_SLACK, g_text_rows);
}

void session_set_margins(int top, int bottom, int text_rows)
{
    g_text_rows = text_rows;
    console_set_margins(top, bottom);
}

void session_handle_key(const CmudKeyEvent *ev)
{
    if (g_plat->on_key && g_plat->on_key(ev)) return;

    switch (ev->kind) {
    case CMUD_KEY_CHAR:      line_edit_insert(&g_edit, ev->ch); break;
    case CMUD_KEY_BACKSPACE: line_edit_backspace(&g_edit); break;
    case CMUD_KEY_DELETE:    line_edit_delete(&g_edit); break;
    case CMUD_KEY_LEFT:      line_edit_left(&g_edit); break;
    case CMUD_KEY_RIGHT:     line_edit_right(&g_edit); break;
    case CMUD_KEY_HOME:      line_edit_home(&g_edit); break;
    case CMUD_KEY_END:       line_edit_end(&g_edit); break;
    case CMUD_KEY_CLEAR:     line_edit_clear(&g_edit); break;
    case CMUD_KEY_UP:        line_edit_history_prev(&g_edit); break;
    case CMUD_KEY_DOWN:      line_edit_history_next(&g_edit); break;
    case CMUD_KEY_PAGEUP:    console_view_scroll(&g_view, -20); break;
    case CMUD_KEY_PAGEDOWN:  console_view_scroll(&g_view, 20); break;
    case CMUD_KEY_CTRL_UP:   console_view_scroll(&g_view, -1); break;
    case CMUD_KEY_CTRL_DOWN: console_view_scroll(&g_view, 1); break;
    case CMUD_KEY_F5:        recolour(COLOUR_ROLE_TEXT); break;
    case CMUD_KEY_F6:        recolour(COLOUR_ROLE_YOU); break;
    case CMUD_KEY_F7:        recolour(COLOUR_ROLE_NOTE); break;
    case CMUD_KEY_F8:        recolour(COLOUR_ROLE_MUD); break;
    case CMUD_KEY_F9:        g_align = !g_align; break;
    case CMUD_KEY_ENTER:
        if (!telnet_server_echo(&g_telnet)) {
            console_write_as("> ", 2, CELL_ATTR_YOU);
            console_write_as(g_edit.line, (unsigned int)g_edit.len, CELL_ATTR_YOU);
            console_write("\n", 1);
        }
        telnet_send_line(&g_telnet, g_edit.line);
        line_edit_commit(&g_edit);
        console_view_follow(&g_view);
        break;
    default: break;
    }
}

void session_terminal(const cui_transport_t *tr)
{
    int esc_frames = 0;
    int carrier_gone = 0;

    telnet_init(&g_telnet, tr, surface_cols() + CMUD_NAWS_SLACK, g_text_rows,
                to_console, 0);
    telnet_hello(&g_telnet);
    line_edit_init(&g_edit);
    console_view_init(&g_view);

    for (;;) {
        CmudKeyEvent ev;

        if (telnet_service(&g_telnet, TELNET_RX_BUDGET) > 0) console_view_follow(&g_view);

        ev = g_plat->poll_key();
        if (ev.kind == CMUD_KEY_ESCAPE) {
            if (esc_frames > 0) break;
            esc_frames = CMUD_ESC_WINDOW;
        } else {
            session_handle_key(&ev);
        }
        if (esc_frames > 0) esc_frames--;

        carrier_gone = cui_transport_is_connected(tr) ? 0 : carrier_gone + 1;
        if (carrier_gone >= CMUD_CARRIER_LOST_FRAMES) break;

        paint(telnet_server_echo(&g_telnet));
        g_plat->wait_frame();
    }

    session_say("");
    session_say("CARRIER LOST");
}

static void wait_for_key(void)
{
    for (;;) {
        CmudKeyEvent ev = g_plat->poll_key();
        paint(0);
        g_plat->wait_frame();
        if (ev.kind != CMUD_KEY_NONE) return;
    }
}

void session_run(void)
{
    for (;;) {
        const cui_transport_t *tr = g_plat->open();

        if (tr) {
            session_terminal(tr);
            g_plat->close();
        }

        session_say("PRESS ANY KEY TO DIAL AGAIN");
        wait_for_key();
    }
}
