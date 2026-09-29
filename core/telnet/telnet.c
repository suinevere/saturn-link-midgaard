#include "telnet.h"

#define MCP_MATCH 0
#define MCP_PASS  1
#define MCP_DROP  2

static void deliver(void *ctx, char c, unsigned char attr)
{
    TelnetState *t = (TelnetState *)ctx;
    if (t->text) t->text(t->text_ctx, &c, &attr, 1);
}

static void play_cue(void *ctx, const MspCue *cue)
{
    TelnetState *t = (TelnetState *)ctx;
    if (t->cue) t->cue(t->cue_ctx, cue);
}

static void sink(TelnetState *t, char c, unsigned char attr)
{
    if (t->msp_on) msp_feed(&t->msp, c, attr, deliver, play_cue, t);
    else deliver(t, c, attr);
}

static void mcp_release(TelnetState *t)
{
    int i;
    for (i = 0; i < t->mcp_held; i++) sink(t, t->mcp_buf[i], t->mcp_attr[i]);
    t->mcp_held = 0;
}

static void emit_text(void *ctx, unsigned char c, unsigned char attr)
{
    TelnetState *t = (TelnetState *)ctx;
    char ch = (char)c;

    if (ch == '\n') {
        if (t->mcp_mode != MCP_DROP) { mcp_release(t); sink(t, ch, attr); }
        t->mcp_held = 0;
        t->mcp_mode = MCP_MATCH;
        return;
    }
    if (t->mcp_mode == MCP_DROP) return;
    if (t->mcp_mode == MCP_PASS) { sink(t, ch, attr); return; }

    t->mcp_attr[t->mcp_held] = attr;
    t->mcp_buf[t->mcp_held++] = ch;
    if (t->mcp_held == 1 && ch != '#') { t->mcp_mode = MCP_PASS; mcp_release(t); return; }
    if (t->mcp_held == 2 && ch != '$') { t->mcp_mode = MCP_PASS; mcp_release(t); return; }
    if (t->mcp_held == 3) {
        t->mcp_held = 0;
        if (ch == '#') { t->mcp_mode = MCP_DROP; return; }
        if (ch == '"') { t->mcp_mode = MCP_PASS; return; }
        t->mcp_mode = MCP_PASS;
        t->mcp_buf[0] = '#';  t->mcp_attr[0] = attr;
        t->mcp_buf[1] = '$';  t->mcp_attr[1] = attr;
        t->mcp_buf[2] = ch;   t->mcp_attr[2] = attr;
        t->mcp_held = 3;
        mcp_release(t);
    }
}

static void send_cmd(TelnetState *t, unsigned char verb, unsigned char opt)
{
    unsigned char b[3];
    b[0] = TN_IAC;
    b[1] = verb;
    b[2] = opt;
    cui_transport_send(t->tr, b, 3);
}

static int already_refused(TelnetState *t, unsigned char opt)
{
    unsigned char mask = (unsigned char)(1u << (opt & 7));
    int idx = opt >> 3;
    if (t->refused[idx] & mask) return 1;
    t->refused[idx] |= mask;
    return 0;
}

static void send_naws(TelnetState *t)
{
    unsigned char b[9];
    b[0] = TN_IAC;
    b[1] = TN_SB;
    b[2] = TNOPT_NAWS;
    b[3] = (unsigned char)((t->cols >> 8) & 0xff);
    b[4] = (unsigned char)(t->cols & 0xff);
    b[5] = (unsigned char)((t->rows >> 8) & 0xff);
    b[6] = (unsigned char)(t->rows & 0xff);
    b[7] = TN_IAC;
    b[8] = TN_SE;
    cui_transport_send(t->tr, b, 9);
}

static void send_ttype(TelnetState *t)
{
    static const unsigned char b[10] = {
        TN_IAC, TN_SB, TNOPT_TTYPE, 0, 'D', 'U', 'M', 'B', TN_IAC, TN_SE
    };
    cui_transport_send(t->tr, b, 10);
}

static void telnet_option(TelnetState *t, unsigned char verb, unsigned char opt)
{
    switch (verb) {
    case TN_DO:
        if (opt == TNOPT_NAWS) {
            send_cmd(t, TN_WILL, TNOPT_NAWS);
            send_naws(t);
        } else if (opt == TNOPT_TTYPE) {
            send_cmd(t, TN_WILL, TNOPT_TTYPE);
        } else if (opt == TNOPT_SGA) {
            send_cmd(t, TN_WILL, TNOPT_SGA);
        } else if (!already_refused(t, opt)) {
            send_cmd(t, TN_WONT, opt);
        }
        return;

    case TN_WILL:
        if (opt == TNOPT_ECHO) {
            t->server_echo = 1;
            send_cmd(t, TN_DO, TNOPT_ECHO);
        } else if (opt == TNOPT_SGA) {
            send_cmd(t, TN_DO, TNOPT_SGA);
        } else if (opt == TNOPT_MSP && t->cue) {
            if (!t->msp_on) {
                t->msp_on = 1;
                msp_init(&t->msp);
                send_cmd(t, TN_DO, TNOPT_MSP);
            }
        } else if (!already_refused(t, opt)) {
            send_cmd(t, TN_DONT, opt);
        }
        return;

    case TN_WONT:
        if (opt == TNOPT_ECHO) {
            t->server_echo = 0;
            send_cmd(t, TN_DONT, TNOPT_ECHO);
        } else if (opt == TNOPT_MSP) {
            t->msp_on = 0;
        }
        return;

    case TN_DONT:
    default:
        return;
    }
}

static void telnet_subneg(TelnetState *t)
{
    if (t->sb_opt == TNOPT_TTYPE && t->sb_len >= 1 && t->sb[0] == 1) send_ttype(t);
}

void telnet_resize(TelnetState *t, int cols, int rows)
{
    if (t->cols == cols && t->rows == rows) return;
    t->cols = cols;
    t->rows = rows;
    send_naws(t);
}

void telnet_set_sound(TelnetState *t, msp_cue_fn cue, void *ctx)
{
    t->cue = cue;
    t->cue_ctx = ctx;
}

void telnet_init(TelnetState *t, const cui_transport_t *tr,
                 int cols, int rows, telnet_text_fn text, void *text_ctx)
{
    int i;
    t->phase = TN_DATA;
    t->neg_verb = 0;
    t->sb_opt = 0;
    t->sb_len = 0;
    for (i = 0; i < 32; i++) t->refused[i] = 0;
    t->server_echo = 0;
    t->mcp_mode = MCP_MATCH;
    t->mcp_held = 0;
    t->cue = 0;
    t->cue_ctx = 0;
    t->msp_on = 0;
    msp_init(&t->msp);
    t->cols = cols;
    t->rows = rows;
    t->tr = tr;
    t->text = text;
    t->text_ctx = text_ctx;
    ansi_init(&t->ansi);
}

int telnet_service(TelnetState *t, int max_bytes)
{
    int read = 0;

    while (read < max_bytes && cui_transport_rx_ready(t->tr)) {
        unsigned char c = cui_transport_rx_byte(t->tr);
        read++;

        switch (t->phase) {
        case TN_DATA:
            if (c == TN_IAC) t->phase = TN_IAC_SEEN;
            else ansi_feed(&t->ansi, c, emit_text, t);
            break;

        case TN_IAC_SEEN:
            if (c == TN_IAC) {
                ansi_feed(&t->ansi, c, emit_text, t);
                t->phase = TN_DATA;
            } else if (c == TN_SB) {
                t->phase = TN_SB_OPT;
            } else if (c == TN_WILL || c == TN_WONT || c == TN_DO || c == TN_DONT) {
                t->neg_verb = c;
                t->phase = TN_NEG;
            } else {
                t->phase = TN_DATA;
            }
            break;

        case TN_NEG:
            telnet_option(t, t->neg_verb, c);
            t->phase = TN_DATA;
            break;

        case TN_SB_OPT:
            t->sb_opt = c;
            t->sb_len = 0;
            t->phase = TN_SB_DATA;
            break;

        case TN_SB_DATA:
            if (c == TN_IAC) t->phase = TN_SB_IAC;
            else if (t->sb_len < TELNET_SB_MAX) t->sb[t->sb_len++] = c;
            break;

        case TN_SB_IAC:
            if (c == TN_SE) {
                telnet_subneg(t);
                t->phase = TN_DATA;
            } else {
                if (c == TN_IAC && t->sb_len < TELNET_SB_MAX) t->sb[t->sb_len++] = c;
                t->phase = TN_SB_DATA;
            }
            break;
        }
    }

    return read;
}

void telnet_hello(TelnetState *t)
{
    send_cmd(t, TN_WILL, TNOPT_NAWS);
}

void telnet_send_line(TelnetState *t, const char *line)
{
    unsigned char crlf[2];
    int n = 0;
    while (line[n] != '\0') n++;
    if (n > 0) cui_transport_send(t->tr, (const unsigned char *)line, n);
    crlf[0] = '\r';
    crlf[1] = '\n';
    cui_transport_send(t->tr, crlf, 2);
}

int telnet_server_echo(const TelnetState *t)
{
    return t->server_echo;
}
