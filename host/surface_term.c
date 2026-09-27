#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200112L
#endif

#include "surface_term.h"
#include "cell_attr.h"
#include "colour_theme.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <time.h>
#endif

#define ST_MAX_COLS 128
#define ST_MAX_ROWS 80

static int  g_cols = 64;
static int  g_rows = 60;
static char g_cell[ST_MAX_ROWS][ST_MAX_COLS + 1];
static unsigned char g_attr[ST_MAX_ROWS][ST_MAX_COLS];

#ifdef _WIN32
static DWORD g_saved_in;
static DWORD g_saved_out;
#else
static struct termios g_saved;
#endif

static int  st_cols(void *ctx) { (void)ctx; return g_cols; }
static int  st_rows(void *ctx) { (void)ctx; return g_rows; }

static void st_put(void *ctx, int x, int y, const char *s, const unsigned char *at)
{
    int i;
    (void)ctx;
    if (y < 0 || y >= g_rows) return;
    for (i = 0; s[i] != '\0' && x + i < g_cols; i++) {
        g_cell[y][x + i] = s[i];
        g_attr[y][x + i] = at ? at[i] : (unsigned char) ANSI_ATTR_DEFAULT;
    }
}

static void st_clear(void *ctx)
{
    int y;
    (void)ctx;
    for (y = 0; y < ST_MAX_ROWS; y++) {
        memset(g_cell[y], ' ', ST_MAX_COLS);
        memset(g_attr[y], ANSI_ATTR_DEFAULT, ST_MAX_COLS);
        g_cell[y][ST_MAX_COLS] = '\0';
    }
}

static void st_present(void *ctx)
{
    const uint16_t *pal = colour_theme_ink();
    int y;
    (void)ctx;
    printf("\033[H");
    for (y = 0; y < g_rows; y++) {
        int x, ink = -1;
        for (x = 0; x < g_cols; x++) {
            int a = colour_theme_slot(g_attr[y][x]);
            if (a != ink) {
                unsigned v = pal[a];
                printf("\033[38;2;%u;%u;%um", (v & 31) * 255 / 31,
                       ((v >> 5) & 31) * 255 / 31, ((v >> 10) & 31) * 255 / 31);
                ink = a;
            }
            putchar(g_cell[y][x]);
        }
        printf("\033[0m\033[K\r\n");
    }
    fflush(stdout);
}

text_surface_t surface_term_make(int cols, int rows)
{
    text_surface_t s;
    g_cols = (cols < ST_MAX_COLS) ? cols : ST_MAX_COLS;
    g_rows = (rows < ST_MAX_ROWS) ? rows : ST_MAX_ROWS;
    st_clear(0);
    s.cols = st_cols;
    s.rows = st_rows;
    s.put = st_put;
    s.clear = st_clear;
    s.present = st_present;
    s.ctx = 0;
    return s;
}

int surface_term_getkey(void)
{
#ifdef _WIN32
    return _kbhit() ? _getch() : -1;
#else
    unsigned char c;
    return (read(STDIN_FILENO, &c, 1) == 1) ? (int)c : -1;
#endif
}

void surface_term_raw(void)
{
#ifdef _WIN32
    HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(hin, &g_saved_in);
    GetConsoleMode(hout, &g_saved_out);
    SetConsoleMode(hin, g_saved_in & (DWORD)~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
    SetConsoleMode(hout, g_saved_out | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    struct termios raw;
    tcgetattr(STDIN_FILENO, &g_saved);
    raw = g_saved;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
#endif
    printf("\033[2J");
}

void surface_term_restore(void)
{
#ifdef _WIN32
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), g_saved_in);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), g_saved_out);
#else
    tcsetattr(STDIN_FILENO, TCSANOW, &g_saved);
#endif
    printf("\033[2J\033[H");
    fflush(stdout);
}

void surface_term_sleep_frame(void)
{
#ifdef _WIN32
    Sleep(16);
#else
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 16000000L;
    nanosleep(&ts, 0);
#endif
}
