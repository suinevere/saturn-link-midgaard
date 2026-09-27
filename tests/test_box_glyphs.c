#include "test_util.h"
#include "box_glyphs.h"

TEST(printable_ascii_is_passed_through_unchanged)
{
    int c;
    for (c = 0x20; c < 0x80; c++) CHECK_INT(box_glyph_map((unsigned char) c), c);
}

TEST(control_codes_become_spaces)
{
    CHECK_INT(box_glyph_map(0x00), ' ');
    CHECK_INT(box_glyph_map(0x01), ' ');
    CHECK_INT(box_glyph_map(0x1f), ' ');
}

TEST(no_byte_resolves_past_the_font)
{
    int c;
    for (c = 0; c < 256; c++) {
        unsigned char g = box_glyph_map((unsigned char) c);
        CHECK(g < 0x80);
        CHECK(g == ' ' || g == '?' || g >= 0x20 ||
              (g >= BOX_GLYPH_FIRST && g < BOX_GLYPH_FIRST + BOX_GLYPH_COUNT));
    }
}

TEST(the_cp437_line_runs_reach_line_glyphs)
{
    CHECK_INT(box_glyph_map(0xc4), box_glyph_map(0xcd));
    CHECK_INT(box_glyph_map(0xb3), box_glyph_map(0xba));
    CHECK(box_glyph_map(0xc4) >= BOX_GLYPH_FIRST);
    CHECK(box_glyph_map(0xb3) >= BOX_GLYPH_FIRST);
    CHECK(box_glyph_map(0xc5) >= BOX_GLYPH_FIRST);
    CHECK(box_glyph_map(0xda) != box_glyph_map(0xbf));
    CHECK(box_glyph_map(0xc0) != box_glyph_map(0xd9));
    CHECK(box_glyph_map(0xda) != box_glyph_map(0xc0));
}

TEST(an_unmapped_high_byte_becomes_a_question_mark)
{
    CHECK_INT(box_glyph_map(0x80), '?');
    CHECK_INT(box_glyph_map(0xe9), '?');
    CHECK_INT(box_glyph_map(0xff), '?');
}

TEST(every_glyph_code_has_rows_and_nothing_else_does)
{
    const unsigned char *h = box_glyph_rows(box_glyph_map(0xc4));
    int c;
    CHECK(h != 0);
    if (h) CHECK_INT(h[3], 0xff);
    for (c = 0; c < 256; c++) {
        int is_glyph = c >= BOX_GLYPH_FIRST && c < BOX_GLYPH_FIRST + BOX_GLYPH_COUNT;
        CHECK_INT(box_glyph_rows((unsigned char)c) != 0, is_glyph);
    }
}

int main(void)
{
    RUN(every_glyph_code_has_rows_and_nothing_else_does);
    RUN(printable_ascii_is_passed_through_unchanged);
    RUN(control_codes_become_spaces);
    RUN(no_byte_resolves_past_the_font);
    RUN(the_cp437_line_runs_reach_line_glyphs);
    RUN(an_unmapped_high_byte_becomes_a_question_mark);
    TEST_MAIN_END();
}
