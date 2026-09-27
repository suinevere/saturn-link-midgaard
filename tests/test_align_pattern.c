#include "test_util.h"
#include "rec_surface.h"
#include "align_pattern.h"
#include <string.h>

static void draw(RecSurface *r, int cols, int wide, int gutter)
{
    text_surface_t s;
    rec_surface_init(r, cols, 30);
    s = rec_surface_make(r);
    align_pattern_draw(&s, wide, gutter);
}

TEST(the_border_reaches_all_four_corners)
{
    RecSurface r;
    const char *top, *bottom;
    draw(&r, 80, 1, 4);
    top    = rec_surface_row(&r, 0);
    bottom = rec_surface_row(&r, 29);
    CHECK_INT((unsigned char) top[0],     0xda);
    CHECK_INT((unsigned char) top[79],    0xbf);
    CHECK_INT((unsigned char) bottom[0],  0xc0);
    CHECK_INT((unsigned char) bottom[79], 0xd9);
}

TEST(the_sides_are_ruled_on_every_middle_row)
{
    RecSurface r;
    int y;
    draw(&r, 80, 1, 4);
    for (y = 3; y < 27; y++) {
        const char *row = rec_surface_row(&r, y);
        CHECK_INT((unsigned char) row[0],  0xb3);
        CHECK_INT((unsigned char) row[79], 0xb3);
    }
}

TEST(the_units_ruler_numbers_from_one)
{
    RecSurface r;
    const char *units;
    draw(&r, 80, 1, 4);
    units = rec_surface_row(&r, 2);
    CHECK_INT(units[0],  '1');
    CHECK_INT(units[8],  '9');
    CHECK_INT(units[9],  '0');
    CHECK_INT(units[79], '0');
}

TEST(the_tens_ruler_marks_only_the_tens)
{
    RecSurface r;
    const char *tens;
    int x;
    draw(&r, 80, 1, 4);
    tens = rec_surface_row(&r, 1);
    for (x = 0; x < 80; x++) {
        if ((x + 1) % 10 == 0) CHECK_INT(tens[x], '0' + ((x + 1) / 10) % 10);
        else                   CHECK_INT(tens[x], ' ');
    }
}

TEST(both_ends_of_the_screen_carry_a_ruler)
{
    RecSurface r;
    draw(&r, 80, 1, 4);
    CHECK(strcmp(rec_surface_row(&r, 1), rec_surface_row(&r, 27)) == 0);
    CHECK(strcmp(rec_surface_row(&r, 2), rec_surface_row(&r, 28)) == 0);
}

TEST(the_label_names_the_geometry_it_was_given)
{
    RecSurface r;
    draw(&r, 80, 1, 4);
    CHECK(strstr(rec_surface_row(&r, 14), "704 80x30 L4") != 0);

    draw(&r, 78, 0, 1);
    CHECK(strstr(rec_surface_row(&r, 14), "640 78x30 L1") != 0);
}

TEST(a_narrower_grid_still_reaches_its_own_edges)
{
    RecSurface r;
    const char *top;
    draw(&r, 78, 0, 1);
    top = rec_surface_row(&r, 0);
    CHECK_INT((unsigned char) top[77], 0xbf);
    CHECK_INT(rec_surface_row(&r, 2)[77], '8');
}

int main(void)
{
    RUN(the_border_reaches_all_four_corners);
    RUN(the_sides_are_ruled_on_every_middle_row);
    RUN(the_units_ruler_numbers_from_one);
    RUN(the_tens_ruler_marks_only_the_tens);
    RUN(both_ends_of_the_screen_carry_a_ruler);
    RUN(the_label_names_the_geometry_it_was_given);
    RUN(a_narrower_grid_still_reaches_its_own_edges);
    TEST_MAIN_END();
}
