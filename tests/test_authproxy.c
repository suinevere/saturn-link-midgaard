#include "test_util.h"
#include "authproxy.h"

TEST(the_hello_is_magic_length_secret)
{
    unsigned char out[AUTHPROXY_HELLO_MAX];
    int n = authproxy_hello(out, (int)sizeof out, "s3cret");
    CHECK_INT(n, 11);
    CHECK_MEM(out, "AUTH\x06s3cret", 11);
}

TEST(an_empty_secret_is_refused)
{
    unsigned char out[AUTHPROXY_HELLO_MAX];
    CHECK_INT(authproxy_hello(out, (int)sizeof out, ""), -1);
}

TEST(a_secret_past_one_length_byte_is_refused)
{
    unsigned char out[AUTHPROXY_HELLO_MAX + 8];
    char secret[257];
    memset(secret, 'k', 256);
    secret[256] = '\0';
    CHECK_INT(authproxy_hello(out, (int)sizeof out, secret), -1);
    secret[255] = '\0';
    CHECK_INT(authproxy_hello(out, (int)sizeof out, secret), 260);
    CHECK_INT(out[4], 255);
}

TEST(a_hello_that_does_not_fit_is_refused)
{
    unsigned char out[8];
    CHECK_INT(authproxy_hello(out, (int)sizeof out, "four"), -1);
    CHECK_INT(authproxy_hello(out, (int)sizeof out, "abc"), 8);
}

int main(void)
{
    RUN(the_hello_is_magic_length_secret);
    RUN(an_empty_secret_is_refused);
    RUN(a_secret_past_one_length_byte_is_refused);
    RUN(a_hello_that_does_not_fit_is_refused);
    TEST_MAIN_END();
}
