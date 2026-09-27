#include "console_view.h"
#include "console.h"

#define CONSOLE_VIEW_ROW_MAX 256

static void layout(const text_surface_t *s, int *top_row, int *rows, int *input_row)
{
    int srows = s->rows(s->ctx);
    int mt = console_top_margin();
    int mb = console_bottom_margin();
    int span;

    if (mt < 0) mt = 0;
    if (mb < 0) mb = 0;
    if (mt + mb + 1 > srows) { mt = 0; mb = 0; }

    *input_row = srows - 1 - mb;
    if (*input_row < 0) *input_row = 0;

    *top_row = mt;
    span = *input_row - mt;
    *rows = (span > 0) ? span : 0;
}

static int bottom_top(int rows)
{
    int t = console_line_count() - rows;
    return (t > 0) ? t : 0;
}

void console_view_init(ConsoleView *v)
{
    v->top = 0;
    v->pinned = 0;
}

void console_view_scroll(ConsoleView *v, int lines)
{
    int max_top = console_line_count();
    v->top += lines;
    if (v->top < 0) v->top = 0;
    if (v->top > max_top) v->top = max_top;
    v->pinned = (v->top < max_top);
}

void console_view_follow(ConsoleView *v)
{
    if (!v->pinned) v->top = 0;
}

void console_view_paint(const ConsoleView *v, const text_surface_t *s,
                        const char *input, int caret, int masked)
{
    char row[CONSOLE_VIEW_ROW_MAX];
    int cols = s->cols(s->ctx);
    int top_row, rows, input_row;
    int top, i, n, width, start, k;

    s->clear(s->ctx);
    layout(s, &top_row, &rows, &input_row);

    top = v->pinned ? v->top : bottom_top(rows);
    if (top > console_line_count() - 1) top = console_line_count() - 1;
    if (top < 0) top = 0;

    n = console_line_count() - top;
    if (n > rows) n = rows;

    for (i = 0; i < n; i++) {
        s->put(s->ctx, 0, top_row + i, console_get_line(top + i),
               console_get_attrs(top + i));
    }

    for (n = 0; input[n] != '\0'; n++) { }
    width = cols - (int)(sizeof(CONSOLE_VIEW_PROMPT) - 1);
    if (width < 1) width = 1;
    start = (caret > width - 1) ? caret - width + 1 : 0;

    k = 0;
    row[k++] = CONSOLE_VIEW_PROMPT[0];
    row[k++] = CONSOLE_VIEW_PROMPT[1];
    for (i = start; i < n && k < CONSOLE_VIEW_ROW_MAX - 1 && k < cols; i++) {
        row[k++] = masked ? '*' : input[i];
    }
    row[k] = '\0';
    s->put(s->ctx, 0, input_row, row, 0);

    s->present(s->ctx);
}
