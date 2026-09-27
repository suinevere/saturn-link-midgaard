#include <srl.hpp>
#include "saturn_keyboard.h"

extern "C" {
#include "surface_ascii.h"
#include "session.h"
#include "net_connect.h"
}

#define CMUD_RESOLUTION SRL::TV::Resolutions::Normal640x240

#define CMUD_MARGIN_TOP    2
#define CMUD_MARGIN_BOTTOM 2

#define CMUD_TEXT_ROWS (SURFACE_ASCII_ROWS - CMUD_MARGIN_TOP - CMUD_MARGIN_BOTTOM - 1)

#define CMUD_DIAL_ATTEMPTS 3
#define CMUD_RETRY_FRAMES  180

#define CMUD_LINE_SETTLE_FRAMES 900

static void wait_frame(void)
{
    SRL::Core::Synchronize();
}

static int dial_poll(void *ctx)
{
    (void)ctx;
    CmudKeyEvent ev = saturn_keyboard_poll();
    session_present();
    return (ev.kind == CMUD_KEY_ESCAPE) ? 1 : 0;
}

static void toggle_width(void)
{
    surface_ascii_set_wide(!surface_ascii_is_wide());
    session_resize(surface_ascii_cols());
}

static int on_key(const CmudKeyEvent *ev)
{
    switch (ev->kind) {
    case CMUD_KEY_F10: toggle_width(); return 1;
    case CMUD_KEY_F11: surface_ascii_nudge(-1); return 1;
    case CMUD_KEY_F12: surface_ascii_nudge(1); return 1;
    default: return 0;
    }
}

static void align_geometry(int *wide, int *gutter)
{
    *wide = surface_ascii_is_wide();
    *gutter = surface_ascii_gutter();
}

static void say_dialling(int attempt)
{
    char line[40];
    const char *p = "DIALLING MIDGAARD... (";
    int k = 0;
    while (*p != 0) line[k++] = *p++;
    line[k++] = (char)('0' + attempt);
    line[k++] = '/';
    line[k++] = (char)('0' + CMUD_DIAL_ATTEMPTS);
    line[k++] = ')';
    line[k] = 0;
    session_say(line);
}

static int hold_for_retry(void)
{
    int f;
    session_say("NO CARRIER. RETRYING...");
    for (f = 0; f < CMUD_RETRY_FRAMES; f++) {
        CmudKeyEvent ev = saturn_keyboard_poll();
        if (ev.kind == CMUD_KEY_ESCAPE) return 1;
        session_present();
    }
    return 0;
}

static void settle_line(void)
{
    int f;
    session_say("RELEASING THE LINE. PLEASE WAIT...");
    for (f = 0; f < CMUD_LINE_SETTLE_FRAMES; f++) session_present();
}

static net_connect_result_t dial_with_retry(void)
{
    net_connect_result_t rc = NET_DIAL_FAIL;
    int attempt;

    for (attempt = 1; attempt <= CMUD_DIAL_ATTEMPTS; attempt++) {
        say_dialling(attempt);
        rc = net_connect_open_poll(CMUD_DIAL_CODE, dial_poll, 0);
        if (rc == NET_OK || rc == NET_NO_MODEM || rc == NET_CANCELLED) break;
        if (attempt < CMUD_DIAL_ATTEMPTS && hold_for_retry()) {
            rc = NET_CANCELLED;
            break;
        }
    }
    return rc;
}

static const cui_transport_t *open_link(void)
{
    net_connect_result_t rc = dial_with_retry();

    if (rc == NET_OK)             return net_connect_transport();
    if (rc == NET_NO_MODEM)       session_say("NO MODEM DETECTED");
    else if (rc == NET_CANCELLED) session_say("CANCELLED");
    else                          session_say("DIAL FAILED");
    return 0;
}

static void close_link(void)
{
    net_connect_close();
}

int main()
{
    static text_surface_t s;
    static session_platform_t plat;

    SRL::Core::Initialize(SRL::Types::HighColor::Colors::Black, CMUD_RESOLUTION);
    s = surface_ascii_make();

    plat.surface        = &s;
    plat.text_rows      = CMUD_TEXT_ROWS;
    plat.margin_top     = CMUD_MARGIN_TOP;
    plat.margin_bottom  = CMUD_MARGIN_BOTTOM;
    plat.poll_key       = saturn_keyboard_poll;
    plat.wait_frame     = wait_frame;
    plat.open           = open_link;
    plat.close          = close_link;
    plat.on_key         = on_key;
    plat.align_geometry = align_geometry;
    plat.recolour       = surface_ascii_recolour;
    session_init(&plat);
    session_splash();

#ifdef NETBIN
    net_connect_reset();
    settle_line();
#endif

    session_run();
    return 0;
}
