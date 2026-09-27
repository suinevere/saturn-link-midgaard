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
    for (y = 0; y < 12; y++) {
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

int main(void)
{
    RUN(a_typed_line_reaches_the_link);
    RUN(the_platform_keeps_the_keys_it_claims);
    RUN(one_escape_does_not_hang_up);
    RUN(the_server_is_told_two_columns_past_the_screen);
    RUN(the_ruler_asks_the_platform_for_its_geometry);
    TEST_MAIN_END();
}
