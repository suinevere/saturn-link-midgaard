#include <kos.h>
#include "session.h"
#include "surface_fb.h"
#include "dc_keyboard.h"
#include "dc_link.h"

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);

#define CMUD_TV_MARGIN 1

static void wait_frame(void)
{
    vid_waitvbl();
    surface_fb_flush();
}

static int mode_margin(void)
{
    return surface_fb_is_480() ? 0 : CMUD_TV_MARGIN;
}

static void apply_mode_geometry(void)
{
    int m = mode_margin();
    session_set_margins(m, m, SURFACE_FB_ROWS - 2 * m - 1);
    session_resize(surface_fb_cols());
}

static int on_key(const CmudKeyEvent *ev)
{
    switch (ev->kind) {
    case CMUD_KEY_F8:  surface_fb_next_theme(); return 1;
    case CMUD_KEY_F10: surface_fb_toggle_mode(); apply_mode_geometry(); return 1;
    default: return 0;
    }
}

int main(int argc, char **argv)
{
    static text_surface_t s;
    static session_platform_t plat;
    int margin;

    (void)argc;
    (void)argv;
    s = surface_fb_make();
    dc_keyboard_init();
    margin = mode_margin();

    plat.surface       = &s;
    plat.text_rows     = SURFACE_FB_ROWS - 2 * margin - 1;
    plat.margin_top    = margin;
    plat.margin_bottom = margin;
    plat.poll_key      = dc_keyboard_poll;
    plat.wait_frame    = wait_frame;
    plat.open          = dc_link_open;
    plat.close         = dc_link_close;
    plat.on_key        = on_key;
    session_init(&plat);

    session_run();
    return 0;
}
