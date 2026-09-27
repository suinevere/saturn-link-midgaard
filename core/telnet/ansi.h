#ifndef ANSI_H
#define ANSI_H
#include "cell_attr.h"

#define ANSI_ESC_MAX 32

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { ANSI_GROUND = 0, ANSI_ESC, ANSI_CSI, ANSI_OSC, ANSI_NF } AnsiPhase;

typedef struct {
    AnsiPhase     phase;
    int           held;
    unsigned char buf[ANSI_ESC_MAX];
    int           osc_esc;
    unsigned char attr;
} AnsiState;

typedef void (*ansi_emit_fn)(void *ctx, unsigned char c, unsigned char attr);

void ansi_init(AnsiState *a);

void ansi_feed(AnsiState *a, unsigned char c, ansi_emit_fn emit, void *ctx);

#ifdef __cplusplus
}
#endif
#endif
