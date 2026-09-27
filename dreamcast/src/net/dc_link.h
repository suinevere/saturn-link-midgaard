#ifndef DC_LINK_H
#define DC_LINK_H
#include "cui_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

const cui_transport_t *dc_link_open(void);

void dc_link_close(void);

#ifdef __cplusplus
}
#endif
#endif
