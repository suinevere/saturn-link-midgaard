#ifndef CONSOLE_VIEW_H
#define CONSOLE_VIEW_H
#include "text_surface.h"

#define CONSOLE_VIEW_PROMPT "> "

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int top;
    int pinned;
} ConsoleView;

void console_view_init(ConsoleView *v);

void console_view_scroll(ConsoleView *v, int lines);

void console_view_follow(ConsoleView *v);

void console_view_paint(const ConsoleView *v, const text_surface_t *s,
                        const char *input, int caret, int masked);

#ifdef __cplusplus
}
#endif
#endif
