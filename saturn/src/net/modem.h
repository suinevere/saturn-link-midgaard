#ifndef MODEM_H
#define MODEM_H

#include "saturn_uart16550.h"
#include <string.h>

#define MODEM_TIMEOUT       2000000
#define MODEM_LINE_MAX      128
#define MODEM_GUARD_TIME    200000

#define MODEM_BAUD_9600       12
#define MODEM_SETTLE_CYCLES   2000000
#define MODEM_PROBE_TIMEOUT   3000000

typedef enum {
    MODEM_OK = 0,
    MODEM_ERROR,
    MODEM_TIMEOUT_ERR,
    MODEM_CONNECT,
    MODEM_NO_CARRIER,
    MODEM_BUSY,
    MODEM_NO_DIALTONE,
    MODEM_NO_ANSWER,
    MODEM_RING,
    MODEM_UNKNOWN
} modem_result_t;

static char modem_last_response[MODEM_LINE_MAX];
static int modem_last_response_len;

static inline int modem_read_line(const saturn_uart16550_t* uart,
                                   char* buf, int max_len, uint32_t timeout) {
    int idx = 0;
    while (idx < max_len - 1) {
        int c = saturn_uart_getc_timeout(uart, timeout);
        if (c < 0) {
            buf[idx] = '\0';
            return (idx > 0) ? idx : -1;
        }
        if (c == '\r' || c == '\n') {
            if (idx > 0) {
                buf[idx] = '\0';
                return idx;
            }
        } else {
            buf[idx++] = (char)c;
        }
    }
    buf[idx] = '\0';
    return idx;
}

static inline modem_result_t modem_parse_response(const char* response) {
    int len = (int)strlen(response);
    if (len >= MODEM_LINE_MAX) len = MODEM_LINE_MAX - 1;
    memcpy(modem_last_response, response, len);
    modem_last_response[len] = '\0';
    modem_last_response_len = len;

    if (strstr(response, "OK"))          return MODEM_OK;
    if (strstr(response, "ERROR"))       return MODEM_ERROR;
    if (strstr(response, "CONNECT"))     return MODEM_CONNECT;
    if (strstr(response, "NO CARRIER"))  return MODEM_NO_CARRIER;
    if (strstr(response, "BUSY"))        return MODEM_BUSY;
    if (strstr(response, "NO DIALTONE")) return MODEM_NO_DIALTONE;
    if (strstr(response, "NO ANSWER"))   return MODEM_NO_ANSWER;
    if (strstr(response, "RING"))        return MODEM_RING;

    if (len == 1) {
        switch (response[0]) {
            case '0': return MODEM_OK;
            case '1': return MODEM_CONNECT;
            case '2': return MODEM_RING;
            case '3': return MODEM_NO_CARRIER;
            case '4': return MODEM_ERROR;
            case '6': return MODEM_NO_DIALTONE;
            case '7': return MODEM_BUSY;
            case '8': return MODEM_NO_ANSWER;
        }
    }

    return MODEM_UNKNOWN;
}

static inline void modem_escape_to_command(const saturn_uart16550_t* uart) {
    for (volatile uint32_t i = 0; i < MODEM_GUARD_TIME; i++);
    saturn_uart_puts(uart, "+++");
    for (volatile uint32_t i = 0; i < MODEM_GUARD_TIME; i++);
}

static inline modem_result_t modem_command_timeout(const saturn_uart16550_t* uart,
                                                    const char* cmd,
                                                    char* response_buf, int buf_len,
                                                    uint32_t timeout) {
    saturn_uart_puts(uart, cmd);
    saturn_uart_puts(uart, "\r");

    while (1) {
        int len = modem_read_line(uart, response_buf, buf_len, timeout);
        if (len < 0) return MODEM_TIMEOUT_ERR;
        if (len == 0) continue;

        modem_result_t result = modem_parse_response(response_buf);
        if (result != MODEM_UNKNOWN) return result;
    }
}

static inline modem_result_t modem_command(const saturn_uart16550_t* uart,
                                            const char* cmd,
                                            char* response_buf, int buf_len) {
    return modem_command_timeout(uart, cmd, response_buf, buf_len, MODEM_TIMEOUT);
}

static inline modem_result_t modem_init(const saturn_uart16550_t* uart) {
    char buf[MODEM_LINE_MAX];

    if (modem_command(uart, "ATZ", buf, sizeof(buf)) != MODEM_OK)
        return MODEM_ERROR;
    if (modem_command(uart, "ATE0", buf, sizeof(buf)) != MODEM_OK)
        return MODEM_ERROR;
    if (modem_command(uart, "ATX3", buf, sizeof(buf)) != MODEM_OK)
        return MODEM_ERROR;
    if (modem_command(uart, "ATV1", buf, sizeof(buf)) != MODEM_OK)
        return MODEM_ERROR;

    return MODEM_OK;
}

static inline modem_result_t modem_probe(const saturn_uart16550_t* uart) {
    char buf[MODEM_LINE_MAX];
    int len;

    saturn_uart_init(uart, MODEM_BAUD_9600);

    for (volatile uint32_t d = 0; d < MODEM_SETTLE_CYCLES; d++);

    saturn_uart_flush_rx(uart);

    saturn_uart_puts(uart, "AT\r");

    len = modem_read_line(uart, buf, sizeof(buf), MODEM_PROBE_TIMEOUT);
    if (len < 0) return MODEM_TIMEOUT_ERR;

    if (modem_parse_response(buf) == MODEM_OK)
        return MODEM_OK;

    len = modem_read_line(uart, buf, sizeof(buf), MODEM_PROBE_TIMEOUT);
    if (len < 0) return MODEM_TIMEOUT_ERR;

    if (modem_parse_response(buf) == MODEM_OK)
        return MODEM_OK;

    return MODEM_TIMEOUT_ERR;
}

static inline modem_result_t modem_hangup(const saturn_uart16550_t* uart) {
    char buf[MODEM_LINE_MAX];
    modem_escape_to_command(uart);
    return modem_command(uart, "ATH0", buf, sizeof(buf));
}

#endif
