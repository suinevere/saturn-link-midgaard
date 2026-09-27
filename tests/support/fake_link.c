#include "fake_link.h"

static bool fl_rx_ready(void *ctx)
{
    FakeLink *f = (FakeLink *)ctx;
    return f->in_pos < f->in_len;
}

static uint8_t fl_rx_byte(void *ctx)
{
    FakeLink *f = (FakeLink *)ctx;
    return (f->in_pos < f->in_len) ? f->in[f->in_pos++] : 0;
}

static int fl_send(void *ctx, const uint8_t *data, int len)
{
    FakeLink *f = (FakeLink *)ctx;
    int i;
    for (i = 0; i < len; i++) {
        if (f->out_len < (int)sizeof(f->out)) f->out[f->out_len++] = data[i];
    }
    return len;
}

static bool fl_connected(void *ctx)
{
    (void)ctx;
    return true;
}

void fake_link_init(FakeLink *f, const unsigned char *in, int in_len)
{
    f->in = in;
    f->in_len = in_len;
    f->in_pos = 0;
    f->out_len = 0;
}

cui_transport_t fake_link_make(FakeLink *f)
{
    cui_transport_t t;
    t.rx_ready = fl_rx_ready;
    t.rx_byte = fl_rx_byte;
    t.send = fl_send;
    t.is_connected = fl_connected;
    t.ctx = f;
    return t;
}
