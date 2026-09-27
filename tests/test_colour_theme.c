#include "test_util.h"
#include "colour_theme.h"
#include "cell_attr.h"

static void reset(void)
{
    int role;

    for (role = 0; role < COLOUR_ROLE_COUNT; role++) {
        while (colour_theme_pick(role) != 0) colour_theme_cycle(role);
    }
}

TEST(each_kind_of_text_has_its_own_slot)
{
    unsigned char you = colour_theme_slot(CELL_ATTR_YOU | ANSI_ATTR_DEFAULT);
    unsigned char note = colour_theme_slot(CELL_ATTR_NOTE | ANSI_ATTR_DEFAULT);
    unsigned char text = colour_theme_slot(ANSI_ATTR_DEFAULT);
    unsigned char red = colour_theme_slot(1);

    CHECK(you != note);
    CHECK(you != text);
    CHECK(note != text);
    CHECK(red != text && red != you && red != note);
    CHECK_INT(colour_theme_slot(ANSI_ATTR_BOLD | 3), ANSI_ATTR_BOLD | 3);
}

TEST(black_text_is_drawn_as_plain_text)
{
    CHECK_INT(colour_theme_slot(0), colour_theme_slot(ANSI_ATTR_DEFAULT));
    CHECK_INT(colour_theme_slot(ANSI_ATTR_BOLD), colour_theme_slot(ANSI_ATTR_DEFAULT));
}

TEST(cycling_one_kind_leaves_the_others_alone)
{
    uint16_t before[16], after[16];
    unsigned char text = colour_theme_slot(ANSI_ATTR_DEFAULT);
    unsigned char you = colour_theme_slot(CELL_ATTR_YOU);
    unsigned char note = colour_theme_slot(CELL_ATTR_NOTE);
    int i;

    reset();
    memcpy(before, colour_theme_ink(), sizeof before);
    colour_theme_cycle(COLOUR_ROLE_YOU);
    memcpy(after, colour_theme_ink(), sizeof after);
    CHECK(after[you] != before[you]);
    for (i = 0; i < 16; i++) if (i != you) CHECK_INT(after[i], before[i]);

    memcpy(before, after, sizeof before);
    colour_theme_cycle(COLOUR_ROLE_MUD);
    memcpy(after, colour_theme_ink(), sizeof after);
    CHECK_INT(after[text], before[text]);
    CHECK_INT(after[you], before[you]);
    CHECK_INT(after[note], before[note]);
    CHECK(after[1] != before[1]);
}

TEST(every_kind_wraps_back_to_its_first_colour)
{
    int i;

    reset();
    for (i = 0; i < COLOUR_HUE_COUNT; i++) colour_theme_cycle(COLOUR_ROLE_TEXT);
    CHECK_INT(colour_theme_pick(COLOUR_ROLE_TEXT), 0);
    for (i = 0; i < COLOUR_THEME_COUNT; i++) colour_theme_cycle(COLOUR_ROLE_MUD);
    CHECK_INT(colour_theme_pick(COLOUR_ROLE_MUD), 0);
}

int main(void)
{
    RUN(each_kind_of_text_has_its_own_slot);
    RUN(black_text_is_drawn_as_plain_text);
    RUN(cycling_one_kind_leaves_the_others_alone);
    RUN(every_kind_wraps_back_to_its_first_colour);
    TEST_MAIN_END();
}
