#ifndef FAKE_LINK_H
#define FAKE_LINK_H
#include "cui_transport.h"

typedef struct {
    const unsigned char *in;
    int                  in_len;
    int                  in_pos;
    unsigned char        out[1024];
    int                  out_len;
} FakeLink;

void            fake_link_init(FakeLink *f, const unsigned char *in, int in_len);
cui_transport_t fake_link_make(FakeLink *f);

#endif
