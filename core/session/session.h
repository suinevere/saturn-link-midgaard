#ifndef SESSION_H
#define SESSION_H
#include "text_surface.h"
#include "cui_transport.h"
#include "key_event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct session_platform {
    const text_surface_t *surface;
    int text_rows;
    int margin_top;
    int margin_bottom;
    CmudKeyEvent (*poll_key)(void);
    void (*wait_frame)(void);
    const cui_transport_t *(*open)(void);
    void (*close)(void);
    int  (*on_key)(const CmudKeyEvent *ev);
    void (*align_geometry)(int *wide, int *gutter);
    void (*recolour)(void);
} session_platform_t;

void session_init(const session_platform_t *p);

void session_say(const char *msg);

void session_splash(void);

void session_present(void);

void session_resize(int cols);

void session_set_margins(int top, int bottom, int text_rows);

void session_handle_key(const CmudKeyEvent *ev);

void session_terminal(const cui_transport_t *tr);

void session_run(void);

#ifdef __cplusplus
}
#endif
#endif
