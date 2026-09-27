#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <kos/net.h>
#include <kos/thread.h>
#include <dc/modem/modem.h>
#include <dc/flashrom.h>
#include <ppp/ppp.h>
#include "net_dhcp.h"
#include "dc_link.h"
#include "transport_tcp.h"
#include "authproxy.h"
#include "session.h"

#ifndef CMUD_DC_HOST
#define CMUD_DC_HOST "suinevere.duckdns.org"
#endif
#ifndef CMUD_DC_PORT
#define CMUD_DC_PORT 2324
#endif
#ifndef CMUD_DC_SECRET
#define CMUD_DC_SECRET ""
#endif

#define CMUD_DC_DIAL     "555"
#define CMUD_DC_PPP_USER "dream"
#define CMUD_DC_PPP_PASS "cast"

#define CMUD_DC_AUTH_FRAMES 300
#define CMUD_DC_PPP_TRIES 4
#define CMUD_DC_PPP_SETTLE_MS 3000

static int             g_fd = -1;
static cui_transport_t g_tr;
static int             g_ppp;

static void say_now(const char *msg)
{
    session_say(msg);
    session_present();
}

static int have_address(void)
{
    const uint8_t *ip;
    if (!net_default_dev) return 0;
    ip = net_default_dev->ip_addr;
    return (ip[0] | ip[1] | ip[2] | ip[3]) != 0;
}

static int dial_dreampi(void)
{
    if (!modem_init()) {
        session_say("NO NETWORK ADAPTER OR MODEM");
        return 0;
    }
    say_now("DIALLING DREAMPI...");
    ppp_init();
    if (ppp_modem_init(CMUD_DC_DIAL, 0, 0) != 0) {
        session_say("NO ANSWER");
        ppp_shutdown();
        return 0;
    }
    say_now("LOGGING IN...");
    ppp_set_login(CMUD_DC_PPP_USER, CMUD_DC_PPP_PASS);
    if (ppp_connect() != 0) {
        session_say("PPP FAILED");
        ppp_shutdown();
        return 0;
    }
    g_ppp = 1;
    net_init(0);
    return 1;
}

static int authenticate(void)
{
    unsigned char hello[AUTHPROXY_HELLO_MAX];
    int n = authproxy_hello(hello, (int)sizeof hello, CMUD_DC_SECRET);
    int f;

    if (n < 0) return 1;
    if (cui_transport_send(&g_tr, hello, n) != n) return 0;
    for (f = 0; f < CMUD_DC_AUTH_FRAMES; f++) {
        if (cui_transport_rx_ready(&g_tr))
            return cui_transport_rx_byte(&g_tr) == AUTHPROXY_ACCEPTED;
        if (!cui_transport_is_connected(&g_tr)) return 0;
        session_present();
    }
    return 0;
}

static void drop_ppp(void)
{
    if (!g_ppp) return;
    ppp_shutdown();
    if (net_default_dev) memset(net_default_dev->ip_addr, 0, 4);
    g_ppp = 0;
}

static int static_from_flash(void)
{
    flashrom_ispcfg_t cfg;
    return flashrom_get_ispcfg(&cfg) == 0 && cfg.method == FLASHROM_ISP_STATIC;
}

static int ask_dhcp(void)
{
    uint8_t ip[4], nm[4], gw[4];
    netif_t *nif = net_default_dev;

    memcpy(ip, nif->ip_addr, 4);
    memcpy(nm, nif->netmask, 4);
    memcpy(gw, nif->gateway, 4);
    memset(nif->ip_addr, 0, 4);
    memset(nif->netmask, 0, 4);
    memset(nif->gateway, 0, 4);

    if (net_dhcp_request(0) >= 0 && have_address()) return 1;

    memcpy(nif->ip_addr, ip, 4);
    memcpy(nif->netmask, nm, 4);
    memcpy(nif->gateway, gw, 4);
    return 0;
}

static int connect_from_here(void)
{
    char line[64];
    const uint8_t *ip = net_default_dev->ip_addr;

    snprintf(line, sizeof line, "CONNECTING TO MIDGAARD FROM %d.%d.%d.%d...",
             ip[0], ip[1], ip[2], ip[3]);
    say_now(line);
    errno = 0;
    return transport_tcp_open(CMUD_DC_HOST, CMUD_DC_PORT);
}

const cui_transport_t *dc_link_open(void)
{
    char line[64];
    int err;
    int tries;

    if (!have_address() && !dial_dreampi()) return 0;

    g_fd = connect_from_here();
    err = errno;
    for (tries = 1; g_fd < 0 && g_ppp && tries < CMUD_DC_PPP_TRIES; tries++) {
        say_now("WAITING FOR THE LINE TO SETTLE...");
        thd_sleep(CMUD_DC_PPP_SETTLE_MS);
        g_fd = connect_from_here();
        err = errno;
    }
    if (g_fd < 0 && !g_ppp && err != 0 && static_from_flash()) {
        say_now("RETRYING WITH DHCP...");
        if (ask_dhcp()) {
            g_fd = connect_from_here();
            err = errno;
        }
    }
    if (g_fd < 0) {
        snprintf(line, sizeof line, "CONNECT FAILED: %s",
                 err ? strerror(err) : "NO SUCH HOST");
        session_say(line);
        drop_ppp();
        return 0;
    }
    g_tr = transport_tcp_make(g_fd);

    if (!authenticate()) {
        session_say("REFUSED BY THE SERVER");
        dc_link_close();
        return 0;
    }
    return &g_tr;
}

void dc_link_close(void)
{
    transport_tcp_close(g_fd);
    g_fd = -1;
}
