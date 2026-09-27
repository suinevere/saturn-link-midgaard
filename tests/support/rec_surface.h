#ifndef REC_SURFACE_H
#define REC_SURFACE_H
#include "text_surface.h"

#define REC_COLS 96
#define REC_ROWS 64

#define REC_ROW_KEEP 8

typedef struct {
    char cell[REC_ROWS][REC_COLS + 1];
    int  cols;
    int  rows;
    int  presents;
} RecSurface;

void           rec_surface_init(RecSurface *r, int cols, int rows);
text_surface_t rec_surface_make(RecSurface *r);
const char    *rec_surface_row(const RecSurface *r, int y);

#endif
