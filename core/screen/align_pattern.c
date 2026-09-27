#include "align_pattern.h"
#include "cell_attr.h"

#define AP_ROW_MAX 96

#define BOX_TL 0xda
#define BOX_TR 0xbf
#define BOX_BL 0xc0
#define BOX_BR 0xd9
#define BOX_H  0xc4
#define BOX_V  0xb3

#define AP_INK_BORDER (ANSI_ATTR_BOLD | 7)
#define AP_INK_EDGE   (ANSI_ATTR_BOLD | 1)
#define AP_INK_RULER  (ANSI_ATTR_BOLD | 3)
#define AP_INK_LABEL  (ANSI_ATTR_BOLD | 6)

static void emit(const text_surface_t *s, int y, int cols,
                 char *row, unsigned char *attr)
{
    attr[0] = AP_INK_EDGE;
    attr[cols - 1] = AP_INK_EDGE;
    row[cols] = '\0';
    s->put(s->ctx, 0, y, row, attr);
}

static void put_label(char *row, unsigned char *attr, int cols, int x,
                      const char *text, unsigned char ink)
{
    int i;
    for (i = 0; text[i] != '\0' && x + i < cols; i++) {
        if (x + i < 0) continue;
        row[x + i] = text[i];
        attr[x + i] = ink;
    }
}

void align_pattern_draw(const text_surface_t *s, int wide, int gutter)
{
    char          row[AP_ROW_MAX + 1];
    unsigned char attr[AP_ROW_MAX + 1];
    char          label[32];
    int cols = s->cols(s->ctx);
    int rows = s->rows(s->ctx);
    int x, y, n;

    if (cols > AP_ROW_MAX) cols = AP_ROW_MAX;
    s->clear(s->ctx);

    for (y = 0; y < rows; y += rows - 1) {
        for (x = 0; x < cols; x++) { row[x] = (char) BOX_H; attr[x] = AP_INK_BORDER; }
        row[0]        = (char) (y == 0 ? BOX_TL : BOX_BL);
        row[cols - 1] = (char) (y == 0 ? BOX_TR : BOX_BR);
        emit(s, y, cols, row, attr);
    }

    for (n = 0; n < 2; n++) {
        int tens_row  = n ? rows - 3 : 1;
        int units_row = n ? rows - 2 : 2;

        for (x = 0; x < cols; x++) {
            int col = x + 1;
            row[x] = (col % 10 == 0) ? (char) ('0' + (col / 10) % 10) : ' ';
            attr[x] = AP_INK_RULER;
        }
        emit(s, tens_row, cols, row, attr);

        for (x = 0; x < cols; x++) {
            row[x] = (char) ('0' + (x + 1) % 10);
            attr[x] = AP_INK_RULER;
        }
        emit(s, units_row, cols, row, attr);
    }

    for (y = 3; y < rows - 3; y++) {
        for (x = 0; x < cols; x++) { row[x] = ' '; attr[x] = AP_INK_BORDER; }
        row[0] = (char) BOX_V;
        row[cols - 1] = (char) BOX_V;
        emit(s, y, cols, row, attr);
    }

    for (x = 0; x < cols; x++) { row[x] = ' '; attr[x] = AP_INK_BORDER; }
    row[0] = (char) BOX_V;
    row[cols - 1] = (char) BOX_V;

    n = 0;
    label[n++] = wide ? '7' : '6';
    label[n++] = wide ? '0' : '4';
    label[n++] = wide ? '4' : '0';
    label[n++] = ' ';
    label[n++] = (char) ('0' + (cols / 10) % 10);
    label[n++] = (char) ('0' + cols % 10);
    label[n++] = 'x';
    label[n++] = (char) ('0' + (rows / 10) % 10);
    label[n++] = (char) ('0' + rows % 10);
    label[n++] = ' ';
    label[n++] = 'L';
    label[n++] = (char) ('0' + gutter % 10);
    label[n] = '\0';
    put_label(row, attr, cols, (cols - n) / 2, label, AP_INK_LABEL);
    emit(s, rows / 2 - 1, cols, row, attr);

    for (x = 0; x < cols; x++) { row[x] = ' '; attr[x] = AP_INK_BORDER; }
    row[0] = (char) BOX_V;
    row[cols - 1] = (char) BOX_V;
    put_label(row, attr, cols, (cols - 22) / 2,
              "F10 mode F11/F12 shift", AP_INK_LABEL);
    emit(s, rows / 2, cols, row, attr);
}
