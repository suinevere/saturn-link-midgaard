#ifndef AUTHPROXY_H
#define AUTHPROXY_H

#ifdef __cplusplus
extern "C" {
#endif

#define AUTHPROXY_ACCEPTED 0x01

#define AUTHPROXY_HELLO_MAX (4 + 1 + 255)

int authproxy_hello(unsigned char *out, int cap, const char *secret);

#ifdef __cplusplus
}
#endif
#endif
