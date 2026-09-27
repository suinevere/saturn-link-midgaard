#include "test_util.h"
#include "line_edit.h"

static void type(LineEdit *e, const char *s)
{
    int i;
    for (i = 0; s[i] != '\0'; i++) line_edit_insert(e, s[i]);
}

TEST(typing_appends)
{
    LineEdit e;
    line_edit_init(&e);
    type(&e, "look");
    CHECK_STR(e.line, "look");
    CHECK_INT(e.caret, 4);
}

TEST(insert_happens_at_the_caret)
{
    LineEdit e;
    line_edit_init(&e);
    type(&e, "lok");
    line_edit_left(&e);
    line_edit_insert(&e, 'o');
    CHECK_STR(e.line, "look");
}

TEST(backspace_at_the_start_does_nothing)
{
    LineEdit e;
    line_edit_init(&e);
    type(&e, "ab");
    line_edit_home(&e);
    line_edit_backspace(&e);
    CHECK_STR(e.line, "ab");
    CHECK_INT(e.caret, 0);
}

TEST(delete_at_the_end_does_nothing)
{
    LineEdit e;
    line_edit_init(&e);
    type(&e, "ab");
    line_edit_end(&e);
    line_edit_delete(&e);
    CHECK_STR(e.line, "ab");
}

TEST(the_line_stops_at_its_bound)
{
    LineEdit e;
    int i;
    line_edit_init(&e);
    for (i = 0; i < LINE_EDIT_MAX + 20; i++) line_edit_insert(&e, 'x');
    CHECK_INT(e.len, LINE_EDIT_MAX - 1);
}

TEST(history_walks_back_and_forward)
{
    LineEdit e;
    line_edit_init(&e);
    type(&e, "north");
    line_edit_commit(&e);
    type(&e, "look");
    line_edit_commit(&e);
    CHECK_INT(e.len, 0);
    line_edit_history_prev(&e);
    CHECK_STR(e.line, "look");
    line_edit_history_prev(&e);
    CHECK_STR(e.line, "north");
    line_edit_history_prev(&e);
    CHECK_STR(e.line, "north");
    line_edit_history_next(&e);
    CHECK_STR(e.line, "look");
}

TEST(walking_out_of_history_restores_the_half_typed_line)
{
    LineEdit e;
    line_edit_init(&e);
    type(&e, "north");
    line_edit_commit(&e);
    type(&e, "sou");
    line_edit_history_prev(&e);
    CHECK_STR(e.line, "north");
    line_edit_history_next(&e);
    CHECK_STR(e.line, "sou");
}

TEST(history_keeps_only_the_last_eight)
{
    LineEdit e;
    int i;
    line_edit_init(&e);
    for (i = 0; i < LINE_HISTORY + 3; i++) {
        char buf[4];
        buf[0] = (char)('a' + i);
        buf[1] = '\0';
        type(&e, buf);
        line_edit_commit(&e);
    }
    for (i = 0; i < LINE_HISTORY + 5; i++) line_edit_history_prev(&e);
    CHECK_STR(e.line, "d");
}

int main(void)
{
    RUN(typing_appends);
    RUN(insert_happens_at_the_caret);
    RUN(backspace_at_the_start_does_nothing);
    RUN(delete_at_the_end_does_nothing);
    RUN(the_line_stops_at_its_bound);
    RUN(history_walks_back_and_forward);
    RUN(walking_out_of_history_restores_the_half_typed_line);
    RUN(history_keeps_only_the_last_eight);
    TEST_MAIN_END();
}
