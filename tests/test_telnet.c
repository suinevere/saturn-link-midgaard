#include "test_util.h"
#include "fake_link.h"
#include "telnet.h"

static char g_text[512];
static int  g_text_len;

static void text_sink(void *ctx, const char *s, const unsigned char *at, int len)
{
    int i;
    (void)ctx;
    (void)at;
    for (i = 0; i < len; i++) {
        if (g_text_len < (int)sizeof(g_text) - 1) g_text[g_text_len++] = s[i];
    }
    g_text[g_text_len] = '\0';
}

static cui_transport_t g_tr;

static void drive(const unsigned char *in, int len, FakeLink *f, TelnetState *t)
{
    g_text_len = 0;
    g_text[0] = '\0';
    fake_link_init(f, in, len);
    g_tr = fake_link_make(f);
    telnet_init(t, &g_tr, 64, 59, text_sink, 0);
    telnet_service(t, TELNET_RX_BUDGET);
}

TEST(plain_text_reaches_the_sink)
{
    static const unsigned char in[] = "You are in Midgaard.";
    FakeLink f;
    TelnetState t;
    drive(in, 20, &f, &t);
    CHECK_STR(g_text, "You are in Midgaard.");
}

TEST(escaped_iac_becomes_one_literal_byte)
{
    static const unsigned char in[] = { 'a', TN_IAC, TN_IAC, 'b' };
    FakeLink f;
    TelnetState t;
    drive(in, 4, &f, &t);
    CHECK_INT(g_text_len, 3);
    CHECK_INT((unsigned char)g_text[1], 0xff);
}

TEST(subnegotiation_is_discarded_to_its_terminator)
{
    static const unsigned char in[] = {
        TN_IAC, TN_SB, 70, 1, 2, 3, TN_IAC, TN_SE, 'H', 'e', 'l', 'l', 'o'
    };
    FakeLink f;
    TelnetState t;
    drive(in, 13, &f, &t);
    CHECK_STR(g_text, "Hello");
}

TEST(subnegotiation_split_across_two_services_is_still_discarded)
{
    static const unsigned char in[] = {
        TN_IAC, TN_SB, 70, 1, 2, 3, TN_IAC, TN_SE, 'H', 'i'
    };
    FakeLink f;
    TelnetState t;
    g_text_len = 0;
    g_text[0] = '\0';
    fake_link_init(&f, in, 10);
    g_tr = fake_link_make(&f);
    telnet_init(&t, &g_tr, 64, 59, text_sink, 0);
    telnet_service(&t, 4);
    telnet_service(&t, TELNET_RX_BUDGET);
    CHECK_STR(g_text, "Hi");
}

TEST(unterminated_subnegotiation_reaches_its_bound_and_resyncs)
{
    static unsigned char in[TELNET_SB_MAX + 32];
    FakeLink f;
    TelnetState t;
    int i, n = 0;
    in[n++] = TN_IAC;
    in[n++] = TN_SB;
    in[n++] = 70;
    for (i = 0; i < TELNET_SB_MAX + 8; i++) in[n++] = 'x';
    in[n++] = TN_IAC;
    in[n++] = TN_SE;
    in[n++] = 'O';
    in[n++] = 'K';
    drive(in, n, &f, &t);
    CHECK_STR(g_text, "OK");
}

TEST(do_naws_is_answered_with_will_and_the_window_size)
{
    static const unsigned char in[] = { TN_IAC, TN_DO, TNOPT_NAWS };
    static const unsigned char want[] = {
        TN_IAC, TN_WILL, TNOPT_NAWS,
        TN_IAC, TN_SB, TNOPT_NAWS, 0, 64, 0, 59, TN_IAC, TN_SE
    };
    FakeLink f;
    TelnetState t;
    drive(in, 3, &f, &t);
    CHECK_INT(f.out_len, 12);
    CHECK_MEM(f.out, want, 12);
}

TEST(do_naws_split_across_services_is_still_answered)
{
    static const unsigned char in[] = { TN_IAC, TN_DO, TNOPT_NAWS };
    static const unsigned char want[] = {
        TN_IAC, TN_WILL, TNOPT_NAWS,
        TN_IAC, TN_SB, TNOPT_NAWS, 0, 64, 0, 59, TN_IAC, TN_SE
    };
    FakeLink f;
    TelnetState t;
    fake_link_init(&f, in, 3);
    g_tr = fake_link_make(&f);
    telnet_init(&t, &g_tr, 64, 59, text_sink, 0);
    telnet_service(&t, 1);
    telnet_service(&t, TELNET_RX_BUDGET);
    CHECK_INT(f.out_len, 12);
    CHECK_MEM(f.out, want, 12);
}

TEST(mccp2_is_refused)
{
    static const unsigned char in[] = { TN_IAC, TN_WILL, 86 };
    static const unsigned char want[] = { TN_IAC, TN_DONT, 86 };
    FakeLink f;
    TelnetState t;
    drive(in, 3, &f, &t);
    CHECK_INT(f.out_len, 3);
    CHECK_MEM(f.out, want, 3);
}

TEST(a_repeated_offer_is_refused_only_once)
{
    static const unsigned char in[] = {
        TN_IAC, TN_WILL, 99, TN_IAC, TN_WILL, 99
    };
    FakeLink f;
    TelnetState t;
    drive(in, 6, &f, &t);
    CHECK_INT(f.out_len, 3);
}

TEST(server_echo_is_taken_and_released)
{
    static const unsigned char on[]  = { TN_IAC, TN_WILL, TNOPT_ECHO };
    static const unsigned char off[] = { TN_IAC, TN_WONT, TNOPT_ECHO };
    static const unsigned char want_on[]  = { TN_IAC, TN_DO,   TNOPT_ECHO };
    static const unsigned char want_off[] = { TN_IAC, TN_DONT, TNOPT_ECHO };
    FakeLink f;
    TelnetState t;

    drive(on, 3, &f, &t);
    CHECK_INT(telnet_server_echo(&t), 1);
    CHECK_MEM(f.out, want_on, 3);

    fake_link_init(&f, off, 3);
    telnet_service(&t, TELNET_RX_BUDGET);
    CHECK_INT(telnet_server_echo(&t), 0);
    CHECK_MEM(f.out, want_off, 3);
}

TEST(terminal_type_is_reported_as_dumb)
{
    static const unsigned char in[] = {
        TN_IAC, TN_DO, TNOPT_TTYPE,
        TN_IAC, TN_SB, TNOPT_TTYPE, 1, TN_IAC, TN_SE
    };
    static const unsigned char want[] = {
        TN_IAC, TN_WILL, TNOPT_TTYPE,
        TN_IAC, TN_SB, TNOPT_TTYPE, 0, 'D', 'U', 'M', 'B', TN_IAC, TN_SE
    };
    FakeLink f;
    TelnetState t;
    drive(in, 9, &f, &t);
    CHECK_INT(f.out_len, 13);
    CHECK_MEM(f.out, want, 13);
}

TEST(hello_offers_naws_and_nothing_else)
{
    static const unsigned char want[] = { TN_IAC, TN_WILL, TNOPT_NAWS };
    FakeLink f;
    TelnetState t;
    fake_link_init(&f, (const unsigned char *)"", 0);
    g_tr = fake_link_make(&f);
    telnet_init(&t, &g_tr, 64, 59, text_sink, 0);
    telnet_hello(&t);
    CHECK_INT(f.out_len, 3);
    CHECK_MEM(f.out, want, 3);
}

TEST(a_sent_line_carries_crlf)
{
    FakeLink f;
    TelnetState t;
    fake_link_init(&f, (const unsigned char *)"", 0);
    g_tr = fake_link_make(&f);
    telnet_init(&t, &g_tr, 64, 59, text_sink, 0);
    telnet_send_line(&t, "look");
    CHECK_INT(f.out_len, 6);
    CHECK_MEM(f.out, "look\r\n", 6);
}

TEST(an_mcp_handshake_line_is_not_shown)
{
    static const unsigned char in[] =
        "#$#mcp version: 2.1 to: 2.1 authentication-key: 8F3A1C09B2\nWelcome.\n";
    FakeLink f;
    TelnetState t;
    drive(in, (int)sizeof(in) - 1, &f, &t);
    CHECK_STR(g_text, "Welcome.\n");
}

TEST(an_mcp_line_after_ordinary_text_is_not_shown)
{
    static const unsigned char in[] = "Hello.\n#$#mcp-negotiate-end\nWorld.\n";
    FakeLink f;
    TelnetState t;
    drive(in, (int)sizeof(in) - 1, &f, &t);
    CHECK_STR(g_text, "Hello.\nWorld.\n");
}

TEST(an_mcp_line_split_across_two_services_is_still_dropped)
{
    static const unsigned char a[] = "#$#mc";
    static const unsigned char b[] = "p foo\nHi\n";
    FakeLink f;
    TelnetState t;
    drive(a, (int)sizeof(a) - 1, &f, &t);
    CHECK_STR(g_text, "");
    fake_link_init(&f, b, (int)sizeof(b) - 1);
    g_tr = fake_link_make(&f);
    t.tr = &g_tr;
    telnet_service(&t, TELNET_RX_BUDGET);
    CHECK_STR(g_text, "Hi\n");
}

TEST(a_quoted_line_loses_only_its_prefix)
{
    static const unsigned char in[] = "#$\"#$#not really mcp\n";
    FakeLink f;
    TelnetState t;
    drive(in, (int)sizeof(in) - 1, &f, &t);
    CHECK_STR(g_text, "#$#not really mcp\n");
}

TEST(ordinary_text_beginning_with_a_hash_survives)
{
    static const unsigned char in[] = "#$5 is the price\n# alone\n";
    FakeLink f;
    TelnetState t;
    drive(in, (int)sizeof(in) - 1, &f, &t);
    CHECK_STR(g_text, "#$5 is the price\n# alone\n");
}

int main(void)
{
    RUN(plain_text_reaches_the_sink);
    RUN(escaped_iac_becomes_one_literal_byte);
    RUN(subnegotiation_is_discarded_to_its_terminator);
    RUN(subnegotiation_split_across_two_services_is_still_discarded);
    RUN(unterminated_subnegotiation_reaches_its_bound_and_resyncs);
    RUN(do_naws_is_answered_with_will_and_the_window_size);
    RUN(do_naws_split_across_services_is_still_answered);
    RUN(mccp2_is_refused);
    RUN(a_repeated_offer_is_refused_only_once);
    RUN(server_echo_is_taken_and_released);
    RUN(terminal_type_is_reported_as_dumb);
    RUN(hello_offers_naws_and_nothing_else);
    RUN(a_sent_line_carries_crlf);
    RUN(an_mcp_handshake_line_is_not_shown);
    RUN(an_mcp_line_after_ordinary_text_is_not_shown);
    RUN(an_mcp_line_split_across_two_services_is_still_dropped);
    RUN(a_quoted_line_loses_only_its_prefix);
    RUN(ordinary_text_beginning_with_a_hash_survives);
    TEST_MAIN_END();
}
