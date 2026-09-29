#include "test_util.h"
#include "rec_surface.h"
#include "fake_link.h"
#include "session.h"

static CmudKeyEvent script[96];
static int script_len, script_pos;
static int frames, geometry_asks;
static RecSurface rec;
static text_surface_t surf;
static session_platform_t plat;

static void key(CmudKeyKind kind, char ch)
{
    script[script_len].kind = kind;
    script[script_len].ch = ch;
    script_len++;
}

static void type(const char *s)
{
    while (*s) key(CMUD_KEY_CHAR, *s++);
}

static CmudKeyEvent poll_key(void)
{
    CmudKeyEvent none = { CMUD_KEY_NONE, 0 };
    return (script_pos < script_len) ? script[script_pos++] : none;
}

static void wait_frame(void) { frames++; }

static int keep_x(const CmudKeyEvent *ev)
{
    return ev->kind == CMUD_KEY_CHAR && ev->ch == 'x';
}

static void geometry(int *wide, int *gutter)
{
    geometry_asks++;
    *wide = 0;
    *gutter = 3;
}

static void setup(void)
{
    script_len = script_pos = frames = geometry_asks = 0;
    rec_surface_init(&rec, 64, 12);
    surf = rec_surface_make(&rec);
    memset(&plat, 0, sizeof plat);
    plat.surface = &surf;
    plat.text_rows = 9;
    plat.poll_key = poll_key;
    plat.wait_frame = wait_frame;
    session_init(&plat);
}

static int screen_has(const char *want)
{
    int y;
    for (y = 0; y < rec.rows; y++) {
        if (strstr(rec_surface_row(&rec, y), want)) return 1;
    }
    return 0;
}

TEST(a_typed_line_reaches_the_link)
{
    FakeLink f;
    cui_transport_t t;
    setup();
    fake_link_init(&f, 0, 0);
    t = fake_link_make(&f);
    type("look");
    key(CMUD_KEY_ENTER, 0);
    key(CMUD_KEY_ESCAPE, 0);
    key(CMUD_KEY_ESCAPE, 0);
    session_terminal(&t);
    CHECK_INT(f.out_len, 3 + 6);
    CHECK_MEM(f.out + 3, "look\r\n", 6);
    session_present();
    CHECK(screen_has("> look"));
    CHECK(screen_has("CARRIER LOST"));
}

TEST(the_platform_keeps_the_keys_it_claims)
{
    FakeLink f;
    cui_transport_t t;
    setup();
    plat.on_key = keep_x;
    fake_link_init(&f, 0, 0);
    t = fake_link_make(&f);
    type("xyx");
    key(CMUD_KEY_ENTER, 0);
    key(CMUD_KEY_ESCAPE, 0);
    key(CMUD_KEY_ESCAPE, 0);
    session_terminal(&t);
    CHECK_INT(f.out_len, 3 + 3);
    CHECK_MEM(f.out + 3, "y\r\n", 3);
}

TEST(one_escape_does_not_hang_up)
{
    FakeLink f;
    cui_transport_t t;
    setup();
    fake_link_init(&f, 0, 0);
    t = fake_link_make(&f);
    key(CMUD_KEY_ESCAPE, 0);
    while (script_len < 60) key(CMUD_KEY_NONE, 0);
    key(CMUD_KEY_ESCAPE, 0);
    key(CMUD_KEY_ESCAPE, 0);
    session_terminal(&t);
    CHECK_INT(script_pos, script_len);
    CHECK_INT(frames, script_len - 1);
}

TEST(the_server_is_told_two_columns_past_the_screen)
{
    static const unsigned char do_naws[] = { 255, 253, 31 };
    static const unsigned char want[] = { 255, 250, 31, 0, 66, 0, 9, 255, 240 };
    FakeLink f;
    cui_transport_t t;
    int i, found = 0;
    setup();
    fake_link_init(&f, do_naws, (int)sizeof do_naws);
    t = fake_link_make(&f);
    key(CMUD_KEY_NONE, 0);
    key(CMUD_KEY_ESCAPE, 0);
    key(CMUD_KEY_ESCAPE, 0);
    session_terminal(&t);
    for (i = 0; i + (int)sizeof want <= f.out_len; i++) {
        if (memcmp(f.out + i, want, sizeof want) == 0) found = 1;
    }
    CHECK(found);
}

TEST(the_ruler_asks_the_platform_for_its_geometry)
{
    CmudKeyEvent f9 = { CMUD_KEY_F9, 0 };
    setup();
    plat.align_geometry = geometry;
    session_present();
    CHECK_INT(geometry_asks, 0);
    session_handle_key(&f9);
    session_present();
    CHECK_INT(geometry_asks, 1);
    session_handle_key(&f9);
    session_present();
    CHECK_INT(geometry_asks, 1);
}

static int recolours;

static void count_recolour(void) { recolours++; }

TEST(the_splash_names_the_game_before_dialling)
{
    setup();
    rec_surface_init(&rec, 64, 40);
    key(CMUD_KEY_CHAR, ' ');
    session_splash();
    CHECK_INT(script_pos, 1);
    CHECK(screen_has("Saturn Link : Midgaard"));
    CHECK(screen_has("/\\"));
    CHECK(screen_has("CoffeeMUD"));
}

TEST(each_colour_key_asks_the_platform_to_recolour)
{
    CmudKeyEvent ev = { CMUD_KEY_F5, 0 };
    setup();
    recolours = 0;
    plat.recolour = count_recolour;
    session_handle_key(&ev);
    ev.kind = CMUD_KEY_F6;
    session_handle_key(&ev);
    ev.kind = CMUD_KEY_F7;
    session_handle_key(&ev);
    ev.kind = CMUD_KEY_F8;
    session_handle_key(&ev);
    CHECK_INT(recolours, 4);
}

static int  plays, stops, services;
static char last_played[32];
static int  last_volume, last_loops;

static void fake_play(void *ctx, const char *name, int volume, int loops, int priority)
{
    int i;
    (void)ctx;
    (void)priority;
    plays++;
    for (i = 0; name[i] && i < 31; i++) last_played[i] = name[i];
    last_played[i] = '\0';
    last_volume = volume;
    last_loops = loops;
}

static void fake_stop(void *ctx) { (void)ctx; stops++; }
static void fake_service(void *ctx) { (void)ctx; services++; }

static cui_sound_t fake_sound = { fake_play, fake_stop, fake_service, 0 };

static void run_with_sound(const char *from_mud)
{
    static unsigned char in[256];
    static FakeLink f;
    static cui_transport_t t;
    int n = 0;
    in[n++] = 255;
    in[n++] = 251;
    in[n++] = 90;
    while (*from_mud) in[n++] = (unsigned char)*from_mud++;
    fake_link_init(&f, in, n);
    t = fake_link_make(&f);
    key(CMUD_KEY_ESCAPE, 0);
    key(CMUD_KEY_ESCAPE, 0);
    session_terminal(&t);
}

static void sound_setup(void)
{
    setup();
    plays = stops = services = 0;
    last_played[0] = '\0';
    plat.sound = &fake_sound;
}

TEST(a_sound_cue_is_played_by_the_platform_and_not_shown)
{
    sound_setup();
    key(CMUD_KEY_NONE, 0);
    run_with_sound("You bark.!!SOUND(bark.wav V=40 L=-1)\n");
    CHECK_INT(plays, 1);
    CHECK_STR(last_played, "bark");
    CHECK_INT(last_volume, 40);
    CHECK_INT(last_loops, -1);
    session_present();
    CHECK(screen_has("You bark."));
    CHECK(!screen_has("SOUND"));
}

TEST(off_stops_the_platform_sound)
{
    sound_setup();
    key(CMUD_KEY_NONE, 0);
    run_with_sound("!!SOUND(Off)");
    CHECK_INT(plays, 0);
    CHECK(stops >= 1);
}

TEST(the_platform_is_serviced_every_frame_and_silenced_on_hang_up)
{
    sound_setup();
    key(CMUD_KEY_NONE, 0);
    key(CMUD_KEY_NONE, 0);
    run_with_sound("");
    CHECK(services >= 3);
    CHECK(stops >= 1);
}

TEST(f4_mutes_and_unmutes_sound)
{
    CmudKeyEvent f4 = { CMUD_KEY_F4, 0 };
    sound_setup();
    session_handle_key(&f4);
    CHECK(stops >= 1);
    key(CMUD_KEY_NONE, 0);
    run_with_sound("!!SOUND(bark)");
    CHECK_INT(plays, 0);
    session_present();
    CHECK(screen_has("SOUND OFF"));

    sound_setup();
    session_handle_key(&f4);
    session_handle_key(&f4);
    key(CMUD_KEY_NONE, 0);
    run_with_sound("!!SOUND(bark)");
    CHECK_INT(plays, 1);
    session_present();
    CHECK(screen_has("SOUND ON"));
}

TEST(f4_says_nothing_where_there_is_no_sound)
{
    CmudKeyEvent f4 = { CMUD_KEY_F4, 0 };
    setup();
    session_handle_key(&f4);
    session_present();
    CHECK(!screen_has("SOUND"));
}

TEST(without_sound_the_platform_refuses_msp)
{
    static const unsigned char will_msp[] = { 255, 251, 90 };
    static const unsigned char dont_msp[] = { 255, 254, 90 };
    FakeLink f;
    cui_transport_t t;
    int i, found = 0;
    setup();
    fake_link_init(&f, will_msp, 3);
    t = fake_link_make(&f);
    key(CMUD_KEY_NONE, 0);
    key(CMUD_KEY_ESCAPE, 0);
    key(CMUD_KEY_ESCAPE, 0);
    session_terminal(&t);
    for (i = 0; i + 3 <= f.out_len; i++) {
        if (memcmp(f.out + i, dont_msp, 3) == 0) found = 1;
    }
    CHECK(found);
}

int main(void)
{
    RUN(a_sound_cue_is_played_by_the_platform_and_not_shown);
    RUN(off_stops_the_platform_sound);
    RUN(the_platform_is_serviced_every_frame_and_silenced_on_hang_up);
    RUN(f4_mutes_and_unmutes_sound);
    RUN(f4_says_nothing_where_there_is_no_sound);
    RUN(without_sound_the_platform_refuses_msp);
    RUN(the_splash_names_the_game_before_dialling);
    RUN(each_colour_key_asks_the_platform_to_recolour);
    RUN(a_typed_line_reaches_the_link);
    RUN(the_platform_keeps_the_keys_it_claims);
    RUN(one_escape_does_not_hang_up);
    RUN(the_server_is_told_two_columns_past_the_screen);
    RUN(the_ruler_asks_the_platform_for_its_geometry);
    TEST_MAIN_END();
}
