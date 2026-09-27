#include "net_connect.h"
#include "saturn_uart16550.h"
#include "modem.h"
#include "transport_uart.h"

static saturn_uart16550_t g_uart;
static cui_transport_t    g_transport;
static int                g_open = 0;

#define MODEM_DIAL_TIMEOUT 105000000u

#define DIAL_POLL_SLICE 50000u

static int detect_uart(void) {
    static const struct { uint32_t base; uint32_t stride; } addrs[] = {
        { 0x25895001, 4 },
        { 0x04895001, 4 },
    };
    int i;
    saturn_netlink_smpc_enable();
    for (i = 0; i < 2; i++) {
        g_uart.base = addrs[i].base;
        g_uart.stride = addrs[i].stride;
        if (saturn_uart_detect(&g_uart)) return 1;
    }
    return 0;
}

static modem_result_t dial_polled(const saturn_uart16550_t *uart,
                                  const char *number, uint32_t timeout,
                                  net_connect_poll_fn poll, void *ctx,
                                  int *cancelled) {
    char line[MODEM_LINE_MAX];
    int      idx   = 0;
    uint32_t spent = 0;

    *cancelled = 0;
    saturn_uart_puts(uart, "ATDT");
    saturn_uart_puts(uart, number);
    saturn_uart_puts(uart, "\r");

    while (spent < timeout) {
        int c = saturn_uart_getc_timeout(uart, DIAL_POLL_SLICE);
        if (c < 0) {
            spent += DIAL_POLL_SLICE;
            if (poll != 0 && poll(ctx)) { *cancelled = 1; return MODEM_NO_CARRIER; }
            continue;
        }

        if (c == '\r' || c == '\n') {
            if (idx > 0) {
                modem_result_t r;
                line[idx] = '\0';
                idx = 0;
                r = modem_parse_response(line);
                if (r != MODEM_UNKNOWN) return r;
            }
        } else if (idx < MODEM_LINE_MAX - 1) {
            line[idx++] = (char) c;
        }
    }
    return MODEM_TIMEOUT_ERR;
}

#define NET_BAUD_FAST 3

static int raise_dte(const saturn_uart16550_t *uart) {
    char line[MODEM_LINE_MAX];
    int  i, len;

    saturn_uart_set_baud(uart, NET_BAUD_FAST);
    saturn_uart_flush_rx(uart);
    saturn_uart_puts(uart, "AT\r");

    for (i = 0; i < 2; i++) {
        len = modem_read_line(uart, line, sizeof(line), MODEM_PROBE_TIMEOUT);
        if (len < 0) break;
        if (modem_parse_response(line) == MODEM_OK) return 1;
    }

    saturn_uart_set_baud(uart, MODEM_BAUD_9600);
    saturn_uart_flush_rx(uart);
    saturn_uart_puts(uart, "AT\r");
    for (i = 0; i < 2; i++) {
        len = modem_read_line(uart, line, sizeof(line), MODEM_PROBE_TIMEOUT);
        if (len < 0) break;
        if (modem_parse_response(line) == MODEM_OK) break;
    }
    return 0;
}

net_connect_result_t net_connect_open_poll(const char *dial_number,
                                           net_connect_poll_fn poll, void *ctx) {
    modem_result_t rc;
    int cancelled = 0;

    g_open = 0;
    if (!detect_uart())                    return NET_NO_MODEM;
    if (modem_probe(&g_uart) != MODEM_OK)  return NET_NO_MODEM;
    raise_dte(&g_uart);
    if (modem_init(&g_uart) != MODEM_OK)   return NET_NO_MODEM;

    rc = dial_polled(&g_uart, dial_number, MODEM_DIAL_TIMEOUT, poll, ctx, &cancelled);
    if (cancelled) {
        saturn_uart_puts(&g_uart, "\r");
        modem_hangup(&g_uart);
        return NET_CANCELLED;
    }
    if (rc != MODEM_CONNECT) return NET_DIAL_FAIL;

    g_transport = transport_uart_make(&g_uart);
    g_open = 1;
    return NET_OK;
}

const cui_transport_t *net_connect_transport(void) {
    return g_open ? &g_transport : 0;
}

void net_connect_close(void) {
    if (g_open) { modem_hangup(&g_uart); g_open = 0; }
}

void net_connect_reset(void) {
    g_open = 0;
    if (!detect_uart()) return;
    saturn_uart_init(&g_uart, MODEM_BAUD_9600);
    modem_escape_to_command(&g_uart);
    modem_hangup(&g_uart);
}
