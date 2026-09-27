#ifndef LINE_EDIT_H
#define LINE_EDIT_H

#define LINE_EDIT_MAX 128

#define LINE_HISTORY 8

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char line[LINE_EDIT_MAX];
    int  len;
    int  caret;
    char hist[LINE_HISTORY][LINE_EDIT_MAX];
    int  hist_count;
    int  hist_pos;
    char stash[LINE_EDIT_MAX];
} LineEdit;

void line_edit_init(LineEdit *e);

void line_edit_insert(LineEdit *e, char c);

void line_edit_backspace(LineEdit *e);
void line_edit_delete(LineEdit *e);

void line_edit_left(LineEdit *e);
void line_edit_right(LineEdit *e);
void line_edit_home(LineEdit *e);
void line_edit_end(LineEdit *e);

void line_edit_clear(LineEdit *e);

void line_edit_history_prev(LineEdit *e);
void line_edit_history_next(LineEdit *e);

void line_edit_commit(LineEdit *e);

#ifdef __cplusplus
}
#endif
#endif
