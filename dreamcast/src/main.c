#include <kos.h>
#include "session.h"
#include "surface_fb.h"
#include "dc_keyboard.h"
#include "dc_link.h"
#include "dc_sound.h"

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);

static void wait_frame(void)
{
    vid_waitvbl();
    surface_fb_flush();
}

static void apply_mode_geometry(void)
{
    session_set_margins(0, 0, surface_fb_rows() - 1);
    session_resize(surface_fb_cols());
}

static int on_key(const CmudKeyEvent *ev)
{
    switch (ev->kind) {
    case CMUD_KEY_F10: surface_fb_toggle_mode(); apply_mode_geometry(); return 1;
    case CMUD_KEY_F11: surface_fb_nudge(1); apply_mode_geometry(); return 1;
    case CMUD_KEY_F12: surface_fb_nudge(-1); apply_mode_geometry(); return 1;
    default: return 0;
    }
}

static void align_geometry(int *wide, int *gutter)
{
    *wide = 0;
    *gutter = surface_fb_inset();
}

int main(int argc, char **argv)
{
    static text_surface_t s;
    static session_platform_t plat;

    (void)argc;
    (void)argv;
    s = surface_fb_make();
    dc_keyboard_init();

    plat.surface        = &s;
    plat.text_rows      = surface_fb_rows() - 1;
    plat.margin_top     = 0;
    plat.margin_bottom  = 0;
    plat.poll_key       = dc_keyboard_poll;
    plat.wait_frame     = wait_frame;
    plat.open           = dc_link_open;
    plat.close          = dc_link_close;
    plat.on_key         = on_key;
    plat.align_geometry = align_geometry;
    plat.recolour       = surface_fb_recolour;
    plat.sound          = dc_sound_init();
    session_init(&plat);

    session_splash();
    session_run();
    return 0;
}
