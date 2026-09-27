#include "console.h"
#include "cell_attr.h"
#include <string.h>

static int wrap_cols = CONSOLE_COLS;

static char lines[CONSOLE_MAX_LINES][CONSOLE_COLS + 1];
static unsigned char attrs[CONSOLE_MAX_LINES][CONSOLE_COLS];
static int  head;
static int  count;
static char cur[CONSOLE_COLS + 1];
static unsigned char curattr[CONSOLE_COLS];
static int  curlen;
static long total_ever;

#define BRK_PARA  0
#define BRK_SPACE 1
#define BRK_TIGHT 2
static unsigned char brk[CONSOLE_MAX_LINES];

static void push_line(const char *s, const unsigned char *at, int len, int how) {
    int slot;
    if (len > CONSOLE_COLS) len = CONSOLE_COLS;
    slot = (head + count) % CONSOLE_MAX_LINES;
    memcpy(lines[slot], s, (size_t) len);
    lines[slot][len] = '\0';
    memcpy(attrs[slot], at, (size_t) len);
    brk[slot] = (unsigned char) how;
    if (count < CONSOLE_MAX_LINES) count++;
    else head = (head + 1) % CONSOLE_MAX_LINES;
    total_ever++;
}

static void flush_cur(int how) {
    push_line(cur, curattr, curlen, how);
    curlen = 0;
    cur[0] = '\0';
}

static char          snap[CONSOLE_MAX_LINES][CONSOLE_COLS + 1];
static unsigned char snapattr[CONSOLE_MAX_LINES][CONSOLE_COLS];
static unsigned char snapbrk[CONSOLE_MAX_LINES];

static void rejoin(int newcols) {
    char keep[CONSOLE_COLS + 1];
    unsigned char keepattr[CONSOLE_COLS];
    int  keeplen = curlen;
    int  oc = count;
    int  i;

    memcpy(keep, cur, (size_t) curlen + 1);
    memcpy(keepattr, curattr, (size_t) curlen);

    for (i = 0; i < oc; i++) {
        int slot = (head + i) % CONSOLE_MAX_LINES;
        memcpy(snap[i], lines[slot], (size_t) CONSOLE_COLS + 1);
        memcpy(snapattr[i], attrs[slot], (size_t) CONSOLE_COLS);
        snapbrk[i] = brk[slot];
    }

    curlen = 0;
    cur[0] = '\0';
    count = 0;
    wrap_cols = newcols;

    for (i = 0; i < oc; i++) {
        int how  = snapbrk[i];
        int slen = (int) strlen(snap[i]);
        if (slen > 0)              console_write(snap[i], (unsigned int) slen);
        if (how == BRK_PARA)       console_write("\n", 1);
        else if (how == BRK_SPACE) console_write(" ", 1);
    }
    if (keeplen > 0) console_write(keep, (unsigned int) keeplen);

    total_ever -= (long) oc;
}

void console_set_cols(int n) {
    if (n < 1) n = 1;
    if (n > CONSOLE_COLS) n = CONSOLE_COLS;
    if (n == wrap_cols) return;
    rejoin(n);
}

static int g_top = 1;
static int g_bottom = 0;

void console_set_margins(int top, int bottom) {
    g_top    = (top    < 0) ? 0 : top;
    g_bottom = (bottom < 0) ? 0 : bottom;
}

int console_top_margin(void)    { return g_top; }
int console_bottom_margin(void) { return g_bottom; }

void console_init(void) {
    head = 0; count = 0; curlen = 0; cur[0] = '\0'; total_ever = 0;
}

void console_write(const char *str, unsigned int len) {
    console_write_attr(str, 0, len);
}

void console_write_attr(const char *str, const unsigned char *at, unsigned int len) {
    unsigned int i;
    for (i = 0; i < len; i++) {
        char c = str[i];
        unsigned char a = at ? at[i] : (unsigned char) ANSI_ATTR_DEFAULT;
        if (c == '\r') continue;
        if (c == '\n') { flush_cur(BRK_PARA); continue; }
        if (c == '\t') {
            int stop = (curlen + 8) & ~7;
            if (stop > wrap_cols) stop = wrap_cols;
            while (curlen < stop) { curattr[curlen] = a; cur[curlen++] = ' '; }
            cur[curlen] = '\0';
            continue;
        }
        if (c == ' ' && curlen >= wrap_cols) { flush_cur(BRK_SPACE); continue; }
        if (c != ' ' && curlen >= wrap_cols) {
            int sp = -1, j;
            for (j = curlen - 1; j >= 0; j--) { if (cur[j] == ' ') { sp = j; break; } }
            if (sp > 0) {
                int carry = curlen - (sp + 1);
                push_line(cur, curattr, sp, BRK_SPACE);
                memmove(cur, cur + sp + 1, (size_t) carry);
                memmove(curattr, curattr + sp + 1, (size_t) carry);
                curlen = carry;
                cur[curlen] = '\0';
            } else {
                flush_cur(BRK_TIGHT);
            }
        }
        curattr[curlen] = a;
        cur[curlen++] = c;
        cur[curlen] = '\0';
    }
}

int console_line_count(void) {
    return count + (curlen > 0 ? 1 : 0);
}

long console_total_lines(void) {
    return total_ever + (curlen > 0 ? 1 : 0);
}

const char *console_get_line(int index) {
    if (index < count) return lines[(head + index) % CONSOLE_MAX_LINES];
    return cur;
}

const unsigned char *console_get_attrs(int index) {
    if (index < 0 || index > count) return curattr;
    if (index == count) return curattr;
    return attrs[(head + index) % CONSOLE_MAX_LINES];
}
