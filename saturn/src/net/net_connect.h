#ifndef NET_CONNECT_H
#define NET_CONNECT_H
#include "cui_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { NET_OK = 0, NET_NO_MODEM, NET_DIAL_FAIL, NET_CANCELLED }
    net_connect_result_t;

typedef int (*net_connect_poll_fn)(void *ctx);

net_connect_result_t net_connect_open_poll(const char *dial_number,
                                           net_connect_poll_fn poll, void *ctx);
const cui_transport_t *net_connect_transport(void);
void net_connect_close(void);

void net_connect_reset(void);

#define CMUD_DIAL_CODE "199409"

#ifdef __cplusplus
}
#endif
#endif
