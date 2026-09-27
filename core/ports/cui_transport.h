#ifndef CUI_TRANSPORT_H
#define CUI_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct cui_transport {

    bool (*rx_ready)(void* ctx);

    uint8_t (*rx_byte)(void* ctx);

    int (*send)(void* ctx, const uint8_t* data, int len);

    bool (*is_connected)(void* ctx);

    void* ctx;

} cui_transport_t;

static inline bool cui_transport_rx_ready(const cui_transport_t* t)
{
    return (t && t->rx_ready) ? t->rx_ready(t->ctx) : false;
}

static inline uint8_t cui_transport_rx_byte(const cui_transport_t* t)
{
    return (t && t->rx_byte) ? t->rx_byte(t->ctx) : 0;
}

static inline int cui_transport_send(const cui_transport_t* t,
                                     const uint8_t* data, int len)
{
    return (t && t->send) ? t->send(t->ctx, data, len) : -1;
}

static inline bool cui_transport_is_connected(const cui_transport_t* t)
{
    if (!t) return false;
    return t->is_connected ? t->is_connected(t->ctx) : true;
}

#endif
