#include "test_util.h"
#include "ansi.h"

static char  g_out[256];
static int   g_len;
static unsigned char g_attr;

static void sink(void *ctx, unsigned char c, unsigned char attr)
{
    (void)ctx;
    g_attr = attr;
    if (g_len < (int)sizeof(g_out) - 1) g_out[g_len++] = (char)c;
    g_out[g_len] = '\0';
}

static void feed(AnsiState *a, const char *s, int len)
{
    int i;
    for (i = 0; i < len; i++) ansi_feed(a, (unsigned char)s[i], sink, 0);
}

static void reset(AnsiState *a)
{
    g_len = 0;
    g_out[0] = '\0';
    ansi_init(a);
}

TEST(sgr_sets_the_colour_the_text_after_it_carries)
{
    AnsiState a;
    reset(&a);
    feed(&a, "\x1b[31mR", 6);
    CHECK_STR(g_out, "R");
    CHECK_INT(g_attr & ANSI_ATTR_COLOUR, 1);
}

TEST(sgr_zero_and_a_bare_m_both_restore_the_default)
{
    AnsiState a;
    reset(&a);
    feed(&a, "\x1b[32m\x1b[0mx", 10);
    CHECK_INT(g_attr, ANSI_ATTR_DEFAULT);
    feed(&a, "\x1b[32m\x1b[mx", 9);
    CHECK_INT(g_attr, ANSI_ATTR_DEFAULT);
}

TEST(bold_is_recorded_beside_the_colour_and_cleared_by_22)
{
    AnsiState a;
    reset(&a);
    feed(&a, "\x1b[1;36mx", 8);
    CHECK_INT(g_attr & ANSI_ATTR_COLOUR, 6);
    CHECK_INT(g_attr & ANSI_ATTR_BOLD, ANSI_ATTR_BOLD);
    feed(&a, "\x1b[22mx", 6);
    CHECK_INT(g_attr & ANSI_ATTR_BOLD, 0);
}

TEST(xterm_256_and_truecolour_are_skipped_not_misread)
{
    AnsiState a;
    reset(&a);
    feed(&a, "\x1b[38;5;196mx", 12);
    CHECK_INT(g_attr & ANSI_ATTR_COLOUR, ANSI_ATTR_DEFAULT);
    feed(&a, "\x1b[38;2;10;20;30mx", 17);
    CHECK_INT(g_attr & ANSI_ATTR_COLOUR, ANSI_ATTR_DEFAULT);
    feed(&a, "\x1b[33mx", 6);
    CHECK_INT(g_attr & ANSI_ATTR_COLOUR, 3);
}

TEST(plain_text_passes_through)
{
    AnsiState a;
    reset(&a);
    feed(&a, "Midgaard", 8);
    CHECK_STR(g_out, "Midgaard");
}

TEST(csi_sequence_is_removed)
{
    AnsiState a;
    reset(&a);
    feed(&a, "a\x1b[0;31mb", 9);
    CHECK_STR(g_out, "ab");
}

TEST(csi_split_across_feeds_is_still_removed)
{
    AnsiState a;
    reset(&a);
    feed(&a, "a\x1b[3", 4);
    feed(&a, "1mb", 3);
    CHECK_STR(g_out, "ab");
}

TEST(osc_terminated_by_bel_is_removed)
{
    AnsiState a;
    reset(&a);
    feed(&a, "a\x1b]0;title\x07""b", 12);
    CHECK_STR(g_out, "ab");
}

TEST(osc_terminated_by_st_is_removed)
{
    AnsiState a;
    reset(&a);
    feed(&a, "a\x1b]0;t\x1b\\b", 9);
    CHECK_STR(g_out, "ab");
}

TEST(bare_bel_is_dropped)
{
    AnsiState a;
    reset(&a);
    feed(&a, "a\x07""b", 3);
    CHECK_STR(g_out, "ab");
}

TEST(two_byte_escape_is_removed)
{
    AnsiState a;
    reset(&a);
    feed(&a, "a\x1b(Bb", 5);
    CHECK_STR(g_out, "ab");
}

TEST(a_csi_may_use_the_whole_buffer_and_still_terminate)
{
    AnsiState a;
    int i;
    reset(&a);
    feed(&a, "\x1b[", 2);
    for (i = 0; i < ANSI_ESC_MAX; i++) feed(&a, "0", 1);
    feed(&a, "m", 1);
    CHECK_INT(g_len, 0);
    feed(&a, "x", 1);
    CHECK_STR(g_out, "x");
}

TEST(unterminated_csi_is_emitted_literally_at_the_bound)
{
    AnsiState a;
    int i;
    reset(&a);
    feed(&a, "\x1b[", 2);
    for (i = 0; i < ANSI_ESC_MAX + 1; i++) feed(&a, "0", 1);
    CHECK_INT(g_len, ANSI_ESC_MAX + 3);
    CHECK(g_out[0] == 0x1b);
    CHECK(g_out[1] == '[');
    feed(&a, "Z", 1);
    CHECK(g_out[g_len - 1] == 'Z');
}

int main(void)
{
    RUN(plain_text_passes_through);
    RUN(csi_sequence_is_removed);
    RUN(csi_split_across_feeds_is_still_removed);
    RUN(osc_terminated_by_bel_is_removed);
    RUN(osc_terminated_by_st_is_removed);
    RUN(bare_bel_is_dropped);
    RUN(two_byte_escape_is_removed);
    RUN(a_csi_may_use_the_whole_buffer_and_still_terminate);
    RUN(unterminated_csi_is_emitted_literally_at_the_bound);
    RUN(sgr_sets_the_colour_the_text_after_it_carries);
    RUN(sgr_zero_and_a_bare_m_both_restore_the_default);
    RUN(bold_is_recorded_beside_the_colour_and_cleared_by_22);
    RUN(xterm_256_and_truecolour_are_skipped_not_misread);
    TEST_MAIN_END();
}
