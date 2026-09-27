#include "rec_surface.h"
#include <string.h>

static int rs_cols(void *ctx) { return ((RecSurface *)ctx)->cols; }
static int rs_rows(void *ctx) { return ((RecSurface *)ctx)->rows; }

static void rs_put(void *ctx, int x, int y, const char *s, const unsigned char *at)
{
    RecSurface *r = (RecSurface *)ctx;
    int i;
    (void)at;
    if (y < 0 || y >= r->rows) return;
    for (i = 0; s[i] != '\0' && x + i < r->cols; i++) r->cell[y][x + i] = s[i];
}

static void rs_clear(void *ctx)
{
    RecSurface *r = (RecSurface *)ctx;
    int y;
    for (y = 0; y < REC_ROWS; y++) {
        memset(r->cell[y], ' ', REC_COLS);
        r->cell[y][REC_COLS] = '\0';
    }
}

static void rs_present(void *ctx) { ((RecSurface *)ctx)->presents++; }

void rec_surface_init(RecSurface *r, int cols, int rows)
{
    r->cols = cols;
    r->rows = rows;
    r->presents = 0;
    rs_clear(r);
}

text_surface_t rec_surface_make(RecSurface *r)
{
    text_surface_t s;
    s.cols = rs_cols;
    s.rows = rs_rows;
    s.put = rs_put;
    s.clear = rs_clear;
    s.present = rs_present;
    s.ctx = r;
    return s;
}

const char *rec_surface_row(const RecSurface *r, int y)
{

    static char ring[REC_ROW_KEEP][REC_COLS + 1];
    static int  next = 0;
    char *trimmed = ring[next];
    int n;

    next = (next + 1) % REC_ROW_KEEP;
    memcpy(trimmed, r->cell[y], REC_COLS);
    trimmed[REC_COLS] = '\0';
    n = REC_COLS;
    while (n > 0 && trimmed[n - 1] == ' ') trimmed[--n] = '\0';
    return trimmed;
}
