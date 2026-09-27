#include "ansi.h"

#define ESC 0x1b
#define BEL 0x07

static void flush_literal(AnsiState *a, ansi_emit_fn emit, void *ctx)
{
    int i;
    emit(ctx, ESC, a->attr);
    if (a->phase == ANSI_CSI) emit(ctx, '[', a->attr);
    if (a->phase == ANSI_OSC) emit(ctx, ']', a->attr);
    for (i = 0; i < a->held; i++) emit(ctx, a->buf[i], a->attr);
    a->held = 0;
    a->osc_esc = 0;
    a->phase = ANSI_GROUND;
}

static int hold(AnsiState *a, unsigned char c)
{
    if (a->held >= ANSI_ESC_MAX) return 0;
    a->buf[a->held++] = c;
    return 1;
}

static void apply_sgr(AnsiState *a)
{
    int i = 0;
    int any = 0;

    while (i < a->held) {
        int v = 0, digits = 0;
        while (i < a->held && a->buf[i] >= '0' && a->buf[i] <= '9') {
            if (v < 1000) v = v * 10 + (a->buf[i] - '0');
            digits++;
            i++;
        }
        if (digits > 0) {
            any = 1;
            if (v == 0)                   a->attr = ANSI_ATTR_DEFAULT;
            else if (v == 1)              a->attr |= ANSI_ATTR_BOLD;
            else if (v == 22)             a->attr &= (unsigned char)~ANSI_ATTR_BOLD;
            else if (v >= 30 && v <= 37)  a->attr = (unsigned char)
                                              ((a->attr & ~ANSI_ATTR_COLOUR) | (v - 30));
            else if (v == 39)             a->attr = (unsigned char)
                                              ((a->attr & ~ANSI_ATTR_COLOUR) | ANSI_ATTR_DEFAULT);
            else if (v == 38 || v == 48) {
                int want = 0;
                if (i < a->held && a->buf[i] == ';') i++;
                { int m = 0, d = 0;
                  while (i < a->held && a->buf[i] >= '0' && a->buf[i] <= '9') {
                      m = m * 10 + (a->buf[i] - '0'); d++; i++; }
                  if (d > 0) want = (m == 2) ? 3 : 1; }
                while (want-- > 0) {
                    if (i < a->held && a->buf[i] == ';') i++;
                    while (i < a->held && a->buf[i] >= '0' && a->buf[i] <= '9') i++;
                }
            }
        }
        if (i < a->held && a->buf[i] == ';') i++;
        else if (digits == 0) i++;
    }

    if (!any) a->attr = ANSI_ATTR_DEFAULT;
}

void ansi_init(AnsiState *a)
{
    a->phase = ANSI_GROUND;
    a->held = 0;
    a->osc_esc = 0;
    a->attr = ANSI_ATTR_DEFAULT;
}

void ansi_feed(AnsiState *a, unsigned char c, ansi_emit_fn emit, void *ctx)
{
    switch (a->phase) {
    case ANSI_GROUND:
        if (c == ESC) { a->phase = ANSI_ESC; a->held = 0; a->osc_esc = 0; }
        else if (c != BEL) emit(ctx, c, a->attr);
        return;

    case ANSI_ESC:
        if (c == '[') { a->phase = ANSI_CSI; a->held = 0; }
        else if (c == ']') { a->phase = ANSI_OSC; a->held = 0; }
        else if (c >= 0x20 && c <= 0x2f) { a->phase = ANSI_NF; a->held = 0; hold(a, c); }
        else a->phase = ANSI_GROUND;
        return;

    case ANSI_NF:
        if (c >= 0x30 && c <= 0x7e) { a->phase = ANSI_GROUND; a->held = 0; return; }
        if (c < 0x20 || c > 0x2f) { a->phase = ANSI_GROUND; a->held = 0; ansi_feed(a, c, emit, ctx); return; }
        if (!hold(a, c)) { flush_literal(a, emit, ctx); ansi_feed(a, c, emit, ctx); }
        return;

    case ANSI_CSI:
        if (c >= 0x40 && c <= 0x7e) {
            if (c == 'm') apply_sgr(a);
            a->phase = ANSI_GROUND; a->held = 0; return;
        }
        if (!hold(a, c)) { flush_literal(a, emit, ctx); ansi_feed(a, c, emit, ctx); }
        return;

    case ANSI_OSC:
        if (c == BEL) { a->phase = ANSI_GROUND; a->held = 0; a->osc_esc = 0; return; }
        if (a->osc_esc && c == '\\') { a->phase = ANSI_GROUND; a->held = 0; a->osc_esc = 0; return; }
        a->osc_esc = (c == ESC);
        if (!hold(a, c)) { flush_literal(a, emit, ctx); ansi_feed(a, c, emit, ctx); }
        return;
    }
}
