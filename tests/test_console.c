#include "test_util.h"
#include "console.h"

static void put(const char *s)
{
    int n = 0;
    while (s[n] != '\0') n++;
    console_write(s, n);
}

TEST(a_newline_ends_a_line)
{
    console_init();
    console_set_cols(64);
    put("one\ntwo\n");
    CHECK_INT(console_line_count(), 2);
    CHECK_STR(console_get_line(0), "one");
    CHECK_STR(console_get_line(1), "two");
}

TEST(carriage_returns_are_dropped)
{
    console_init();
    console_set_cols(64);
    put("one\r\ntwo\r\n");
    CHECK_INT(console_line_count(), 2);
    CHECK_STR(console_get_line(0), "one");
}

TEST(text_wraps_at_the_set_width)
{
    console_init();
    console_set_cols(10);
    put("aaaaaaaaaaaa\n");
    CHECK_INT(console_line_count(), 2);
    CHECK_INT((int)strlen(console_get_line(0)), 10);
}

TEST(an_eighty_column_line_fills_one_row)
{
    char line[82];
    memset(line, 'x', 80);
    line[80] = '\n';
    line[81] = '\0';
    console_init();
    console_set_cols(80);
    put(line);
    put("next\n");
    CHECK_INT(console_line_count(), 2);
    CHECK_INT((int)strlen(console_get_line(0)), 80);
    CHECK_STR(console_get_line(1), "next");
}

TEST(a_tab_advances_to_the_next_multiple_of_eight)
{
    console_init();
    console_set_cols(64);
    put("ab\tc\n");
    CHECK_STR(console_get_line(0), "ab      c");
}

TEST(a_tab_at_a_stop_advances_a_whole_stop)
{
    console_init();
    console_set_cols(64);
    put("abcdefgh\tx\n");
    CHECK_INT((int)strlen(console_get_line(0)), 17);
}

TEST(the_total_keeps_counting_past_eviction)
{
    int i;
    console_init();
    console_set_cols(64);
    for (i = 0; i < CONSOLE_MAX_LINES + 20; i++) put("x\n");
    CHECK_INT(console_line_count(), CONSOLE_MAX_LINES);
    CHECK_INT(console_total_lines(), CONSOLE_MAX_LINES + 20);
}

int main(void)
{
    RUN(a_newline_ends_a_line);
    RUN(carriage_returns_are_dropped);
    RUN(text_wraps_at_the_set_width);
    RUN(an_eighty_column_line_fills_one_row);
    RUN(a_tab_advances_to_the_next_multiple_of_eight);
    RUN(a_tab_at_a_stop_advances_a_whole_stop);
    RUN(the_total_keeps_counting_past_eviction);
    TEST_MAIN_END();
}
