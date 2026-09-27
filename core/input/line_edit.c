#include "line_edit.h"

static void copy_into(LineEdit *e, const char *s)
{
    int i = 0;
    while (s[i] != '\0' && i < LINE_EDIT_MAX - 1) {
        e->line[i] = s[i];
        i++;
    }
    e->line[i] = '\0';
    e->len = i;
    e->caret = i;
}

static void stash_line(LineEdit *e)
{
    int i = 0;
    while (e->line[i] != '\0' && i < LINE_EDIT_MAX - 1) {
        e->stash[i] = e->line[i];
        i++;
    }
    e->stash[i] = '\0';
}

void line_edit_init(LineEdit *e)
{
    int i;
    e->line[0] = '\0';
    e->len = 0;
    e->caret = 0;
    e->hist_count = 0;
    e->hist_pos = LINE_HISTORY;
    e->stash[0] = '\0';
    for (i = 0; i < LINE_HISTORY; i++) e->hist[i][0] = '\0';
}

void line_edit_insert(LineEdit *e, char c)
{
    int i;
    if (e->len >= LINE_EDIT_MAX - 1) return;
    for (i = e->len; i > e->caret; i--) e->line[i] = e->line[i - 1];
    e->line[e->caret] = c;
    e->len++;
    e->caret++;
    e->line[e->len] = '\0';
}

void line_edit_backspace(LineEdit *e)
{
    int i;
    if (e->caret <= 0) return;
    for (i = e->caret - 1; i < e->len - 1; i++) e->line[i] = e->line[i + 1];
    e->len--;
    e->caret--;
    e->line[e->len] = '\0';
}

void line_edit_delete(LineEdit *e)
{
    int i;
    if (e->caret >= e->len) return;
    for (i = e->caret; i < e->len - 1; i++) e->line[i] = e->line[i + 1];
    e->len--;
    e->line[e->len] = '\0';
}

void line_edit_left(LineEdit *e)  { if (e->caret > 0) e->caret--; }
void line_edit_right(LineEdit *e) { if (e->caret < e->len) e->caret++; }
void line_edit_home(LineEdit *e)  { e->caret = 0; }
void line_edit_end(LineEdit *e)   { e->caret = e->len; }

void line_edit_clear(LineEdit *e)
{
    e->line[0] = '\0';
    e->len = 0;
    e->caret = 0;
    e->hist_pos = LINE_HISTORY;
}

void line_edit_history_prev(LineEdit *e)
{
    if (e->hist_count == 0) return;
    if (e->hist_pos == LINE_HISTORY) {
        stash_line(e);
        e->hist_pos = 0;
    } else if (e->hist_pos < e->hist_count - 1) {
        e->hist_pos++;
    }
    copy_into(e, e->hist[e->hist_pos]);
}

void line_edit_history_next(LineEdit *e)
{
    if (e->hist_pos == LINE_HISTORY) return;
    if (e->hist_pos == 0) {
        e->hist_pos = LINE_HISTORY;
        copy_into(e, e->stash);
        return;
    }
    e->hist_pos--;
    copy_into(e, e->hist[e->hist_pos]);
}

void line_edit_commit(LineEdit *e)
{
    int i, j;
    if (e->len > 0) {
        for (i = LINE_HISTORY - 1; i > 0; i--) {
            for (j = 0; j < LINE_EDIT_MAX; j++) e->hist[i][j] = e->hist[i - 1][j];
        }
        for (j = 0; j < LINE_EDIT_MAX; j++) e->hist[0][j] = e->line[j];
        if (e->hist_count < LINE_HISTORY) e->hist_count++;
    }
    line_edit_clear(e);
}
