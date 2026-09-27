#ifndef TEXT_SURFACE_H
#define TEXT_SURFACE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct text_surface {
    int  (*cols)(void *ctx);
    int  (*rows)(void *ctx);
    void (*put)(void *ctx, int x, int y, const char *s,
                const unsigned char *attrs);
    void (*clear)(void *ctx);
    void (*present)(void *ctx);
    void *ctx;
} text_surface_t;

#ifdef __cplusplus
}
#endif
#endif
