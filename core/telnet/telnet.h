#ifndef TELNET_H
#define TELNET_H
#include "cui_transport.h"
#include "ansi.h"
#include "msp.h"

#define TELNET_RX_BUDGET 512

#define TELNET_SB_MAX 128

#define TN_SE   240
#define TN_SB   250
#define TN_WILL 251
#define TN_WONT 252
#define TN_DO   253
#define TN_DONT 254
#define TN_IAC  255

#define TNOPT_ECHO   1
#define TNOPT_SGA    3
#define TNOPT_TTYPE  24
#define TNOPT_NAWS   31
#define TNOPT_MSP    90

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*telnet_text_fn)(void *ctx, const char *s,
                               const unsigned char *attrs, int len);

typedef enum {
    TN_DATA = 0,
    TN_IAC_SEEN,
    TN_NEG,
    TN_SB_OPT,
    TN_SB_DATA,
    TN_SB_IAC
} TelnetPhase;

typedef struct {
    TelnetPhase   phase;
    unsigned char neg_verb;
    unsigned char sb_opt;
    int           sb_len;
    unsigned char sb[TELNET_SB_MAX];

    unsigned char refused[32];
    int           server_echo;
    int           cols;
    int           rows;

    unsigned char mcp_mode;
    int           mcp_held;
    char          mcp_buf[3];
    unsigned char mcp_attr[3];

    msp_cue_fn    cue;
    void         *cue_ctx;
    int           msp_on;
    MspFilter     msp;

    const cui_transport_t *tr;
    AnsiState      ansi;
    telnet_text_fn text;
    void          *text_ctx;
} TelnetState;

void telnet_init(TelnetState *t, const cui_transport_t *tr,
                 int cols, int rows, telnet_text_fn text, void *text_ctx);

void telnet_resize(TelnetState *t, int cols, int rows);

void telnet_set_sound(TelnetState *t, msp_cue_fn cue, void *ctx);

int telnet_service(TelnetState *t, int max_bytes);

void telnet_hello(TelnetState *t);

void telnet_send_line(TelnetState *t, const char *line);

int telnet_server_echo(const TelnetState *t);

#ifdef __cplusplus
}
#endif
#endif
