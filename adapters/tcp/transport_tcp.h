#ifndef TRANSPORT_TCP_H
#define TRANSPORT_TCP_H
#include "cui_transport.h"

int transport_tcp_open(const char *host, int port);

cui_transport_t transport_tcp_make(int fd);

void transport_tcp_close(int fd);

#endif
