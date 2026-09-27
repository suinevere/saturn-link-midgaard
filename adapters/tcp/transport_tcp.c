#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200112L
#endif

#include "transport_tcp.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

static int           g_fd = -1;
static unsigned char g_buf[4096];
static int           g_len;
static int           g_pos;
static int           g_up = 1;

#ifdef _WIN32
static long sock_read(int fd, void *buf, int len)        { return recv((SOCKET)fd, (char *)buf, len, 0); }
static long sock_write(int fd, const void *buf, int len) { return send((SOCKET)fd, (const char *)buf, len, 0); }
static void sock_close(int fd)                           { closesocket((SOCKET)fd); }
static int  sock_would_block(void)                       { return WSAGetLastError() == WSAEWOULDBLOCK; }
#else
static long sock_read(int fd, void *buf, int len)        { return (long)read(fd, buf, (size_t)len); }
static long sock_write(int fd, const void *buf, int len) { return (long)write(fd, buf, (size_t)len); }
static void sock_close(int fd)                           { close(fd); }
static int  sock_would_block(void)                       { return errno == EAGAIN || errno == EWOULDBLOCK; }
#endif

static void set_nonblocking(int fd)
{
#ifdef _WIN32
    u_long on = 1;
    ioctlsocket((SOCKET)fd, FIONBIO, &on);
#else
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
#endif
}

static void pump(void)
{
    long n;
    if (g_pos < g_len) return;
    n = sock_read(g_fd, g_buf, (int)sizeof(g_buf));
    if (n > 0) { g_len = (int)n; g_pos = 0; return; }
    if (n == 0) g_up = 0;
    else if (!sock_would_block()) g_up = 0;
    g_len = 0;
    g_pos = 0;
}

static bool tcp_rx_ready(void *ctx)
{
    (void)ctx;
    pump();
    return g_pos < g_len;
}

static uint8_t tcp_rx_byte(void *ctx)
{
    (void)ctx;
    pump();
    return (g_pos < g_len) ? g_buf[g_pos++] : 0;
}

static int tcp_send(void *ctx, const uint8_t *data, int len)
{
    (void)ctx;
    return (int)sock_write(g_fd, data, len);
}

static bool tcp_connected(void *ctx)
{
    (void)ctx;
    return g_up != 0;
}

int transport_tcp_open(const char *host, int port)
{
    struct addrinfo hints, *res, *p;
    char portstr[16];
    int fd = -1;

#ifdef _WIN32
    static int started;
    if (!started) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
        started = 1;
    }
#endif

    snprintf(portstr, sizeof(portstr), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, portstr, &hints, &res) != 0) {
#ifndef _WIN32
        errno = 0;
#endif
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        fd = (int)socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;
        if (connect((
#ifdef _WIN32
            SOCKET
#else
            int
#endif
            )fd, p->ai_addr, (int)p->ai_addrlen) == 0) break;
        sock_close(fd);
        fd = -1;
    }
    freeaddrinfo(res);

    if (fd >= 0) {
        set_nonblocking(fd);
        g_fd = fd;
        g_len = 0;
        g_pos = 0;
        g_up = 1;
    }
    return fd;
}

cui_transport_t transport_tcp_make(int fd)
{
    cui_transport_t t;
    g_fd = fd;
    t.rx_ready = tcp_rx_ready;
    t.rx_byte = tcp_rx_byte;
    t.send = tcp_send;
    t.is_connected = tcp_connected;
    t.ctx = 0;
    return t;
}

void transport_tcp_close(int fd)
{
    if (fd >= 0) sock_close(fd);
    g_fd = -1;
    g_up = 0;
}
