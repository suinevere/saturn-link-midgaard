#ifndef CONSOLE_H
#define CONSOLE_H

#define CONSOLE_COLS 80
#define CONSOLE_MAX_LINES 256

#ifdef __cplusplus
extern "C" {
#endif

void console_set_cols(int n);
void console_set_margins(int top, int bottom);
int  console_top_margin(void);
int  console_bottom_margin(void);

void console_init(void);
void console_write(const char *str, unsigned int len);

void console_write_attr(const char *str, const unsigned char *attrs, unsigned int len);
void console_write_as(const char *str, unsigned int len, unsigned char attr);
const unsigned char *console_get_attrs(int index);
int  console_line_count(void);
long console_total_lines(void);
const char *console_get_line(int index);

#ifdef __cplusplus
}
#endif
#endif
