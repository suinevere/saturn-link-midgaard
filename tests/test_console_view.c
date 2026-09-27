#include "test_util.h"
#include "rec_surface.h"
#include "console.h"
#include "console_view.h"

static void put(const char *s)
{
    int n = 0;
    while (s[n] != '\0') n++;
    console_write(s, n);
}

TEST(text_fills_from_the_top_down)
{
    RecSurface r;
    text_surface_t s;
    ConsoleView v;
    console_init();
    console_set_cols(64);
    console_set_margins(1, 1);
    put("first\nsecond\n");
    rec_surface_init(&r, 64, 8);
    s = rec_surface_make(&r);
    console_view_init(&v);
    console_view_paint(&v, &s, "", 0, 0);
    CHECK_STR(rec_surface_row(&r, 1), "first");
    CHECK_STR(rec_surface_row(&r, 2), "second");
    CHECK_STR(rec_surface_row(&r, 3), "");
}

TEST(the_margins_at_each_end_stay_empty)
{
    RecSurface r;
    text_surface_t s;
    ConsoleView v;
    int i;
    console_init();
    console_set_cols(64);
    console_set_margins(2, 2);
    for (i = 0; i < 40; i++) put("line\n");
    rec_surface_init(&r, 64, 12);
    s = rec_surface_make(&r);
    console_view_init(&v);
    console_view_paint(&v, &s, "typed", 5, 0);
    CHECK_STR(rec_surface_row(&r, 0), "");
    CHECK_STR(rec_surface_row(&r, 1), "");
    CHECK_STR(rec_surface_row(&r, 2), "line");
    CHECK_STR(rec_surface_row(&r, 9), "> typed");
    CHECK_STR(rec_surface_row(&r, 10), "");
    CHECK_STR(rec_surface_row(&r, 11), "");
}

TEST(a_full_screen_still_shows_the_newest_lines_last)
{
    RecSurface r;
    text_surface_t s;
    ConsoleView v;
    console_init();
    console_set_cols(64);
    console_set_margins(1, 1);
    put("a\nb\nc\nd\ne\nf\n");
    rec_surface_init(&r, 64, 6);
    s = rec_surface_make(&r);
    console_view_init(&v);
    console_view_paint(&v, &s, "", 0, 0);
    CHECK_STR(rec_surface_row(&r, 1), "d");
    CHECK_STR(rec_surface_row(&r, 2), "e");
    CHECK_STR(rec_surface_row(&r, 3), "f");
    CHECK_STR(rec_surface_row(&r, 4), ">");
}

TEST(the_input_row_carries_a_prompt)
{
    RecSurface r;
    text_surface_t s;
    ConsoleView v;
    console_init();
    console_set_cols(64);
    console_set_margins(1, 0);
    rec_surface_init(&r, 64, 6);
    s = rec_surface_make(&r);
    console_view_init(&v);
    console_view_paint(&v, &s, "look", 4, 0);
    CHECK_STR(rec_surface_row(&r, 5), "> look");
}

TEST(a_masked_input_row_shows_asterisks)
{
    RecSurface r;
    text_surface_t s;
    ConsoleView v;
    console_init();
    console_set_cols(64);
    console_set_margins(1, 0);
    rec_surface_init(&r, 64, 6);
    s = rec_surface_make(&r);
    console_view_init(&v);
    console_view_paint(&v, &s, "hunter2", 7, 1);
    CHECK_STR(rec_surface_row(&r, 5), "> *******");
}

TEST(the_input_window_follows_the_caret)
{
    RecSurface r;
    text_surface_t s;
    ConsoleView v;
    char longline[100];
    int i;
    for (i = 0; i < 99; i++) longline[i] = (char)('a' + (i % 26));
    longline[99] = '\0';
    console_init();
    console_set_cols(64);
    console_set_margins(1, 0);
    rec_surface_init(&r, 20, 6);
    s = rec_surface_make(&r);
    console_view_init(&v);
    console_view_paint(&v, &s, longline, 99, 0);
    CHECK_INT((int)strlen(rec_surface_row(&r, 5)), 19);
    CHECK(rec_surface_row(&r, 5)[18] == longline[98]);
    CHECK(rec_surface_row(&r, 5)[2] == longline[82]);
}

TEST(a_scrolled_back_view_does_not_follow_new_output)
{
    ConsoleView v;
    int i;
    console_init();
    console_set_cols(64);
    for (i = 0; i < 20; i++) put("line\n");
    console_view_init(&v);
    console_view_scroll(&v, -5);
    CHECK_INT(v.pinned, 1);
    console_view_follow(&v);
    CHECK_INT(v.pinned, 1);
    console_view_scroll(&v, 99);
    CHECK_INT(v.pinned, 0);
}

int main(void)
{
    RUN(text_fills_from_the_top_down);
    RUN(the_margins_at_each_end_stay_empty);
    RUN(a_full_screen_still_shows_the_newest_lines_last);
    RUN(the_input_row_carries_a_prompt);
    RUN(a_masked_input_row_shows_asterisks);
    RUN(the_input_window_follows_the_caret);
    RUN(a_scrolled_back_view_does_not_follow_new_output);
    TEST_MAIN_END();
}
