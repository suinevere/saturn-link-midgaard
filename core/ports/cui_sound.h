#ifndef CUI_SOUND_H
#define CUI_SOUND_H

typedef struct cui_sound {

    void (*play)(void* ctx, const char* name, int volume, int loops, int priority);

    void (*stop)(void* ctx);

    void (*service)(void* ctx);

    void* ctx;

} cui_sound_t;

static inline void cui_sound_play(const cui_sound_t* s, const char* name,
                                  int volume, int loops, int priority)
{
    if (s && s->play) s->play(s->ctx, name, volume, loops, priority);
}

static inline void cui_sound_stop(const cui_sound_t* s)
{
    if (s && s->stop) s->stop(s->ctx);
}

static inline void cui_sound_service(const cui_sound_t* s)
{
    if (s && s->service) s->service(s->ctx);
}

#endif
