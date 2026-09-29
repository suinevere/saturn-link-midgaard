#include <kos.h>
#include <dc/sound/sound.h>
#include <dc/sound/sfxmgr.h>
#include <string.h>
#include "dc_sound.h"

#define DC_SOUND_SLOTS   48
#define DC_SOUND_VOICES  8
#define DC_SOUND_BUDGET  (1536 * 1024)
#define DC_SOUND_DIR     "/cd/sounds/"

typedef struct {
    char     name[32];
    sfxhnd_t handle;
    int      missing;
    uint32   bytes;
    uint32   ms;
    uint64   used;
} DcSoundSlot;

typedef struct {
    int    slot;
    int    chn;
    int    volume;
    int    loops;
    int    priority;
    uint64 ends;
} DcSoundVoice;

static DcSoundSlot  g_slots[DC_SOUND_SLOTS];
static DcSoundVoice g_voices[DC_SOUND_VOICES];
static int          g_voice_count;
static uint32       g_loaded_bytes;
static cui_sound_t  g_port;

static uint32 le32(const unsigned char *p)
{
    return (uint32)p[0] | ((uint32)p[1] << 8) | ((uint32)p[2] << 16) | ((uint32)p[3] << 24);
}

static int read_wav_shape(const char *path, uint32 *bytes, uint32 *ms)
{
    unsigned char h[8], fmt[16];
    uint32 rate = 0, frame = 0, data = 0;
    file_t f = fs_open(path, O_RDONLY);

    if (f == FILEHND_INVALID) return 0;
    if (fs_read(f, h, 8) != 8 || memcmp(h, "RIFF", 4) != 0) { fs_close(f); return 0; }
    if (fs_read(f, h, 4) != 4 || memcmp(h, "WAVE", 4) != 0) { fs_close(f); return 0; }
    while (fs_read(f, h, 8) == 8) {
        uint32 len = le32(h + 4);
        if (memcmp(h, "fmt ", 4) == 0 && len >= 16) {
            if (fs_read(f, fmt, 16) != 16) break;
            rate = le32(fmt + 4);
            frame = (uint32)fmt[12] | ((uint32)fmt[13] << 8);
            if (len > 16) fs_seek(f, (off_t)(len - 16 + (len & 1)), SEEK_CUR);
        } else if (memcmp(h, "data", 4) == 0) {
            data = len;
            break;
        } else {
            fs_seek(f, (off_t)(len + (len & 1)), SEEK_CUR);
        }
    }
    fs_close(f);
    if (rate == 0 || frame == 0 || data == 0) return 0;
    *bytes = data;
    *ms = (uint32)(((uint64)data * 1000) / ((uint64)rate * frame));
    return 1;
}

static int slot_playing(int s)
{
    int v;
    for (v = 0; v < g_voice_count; v++) {
        if (g_voices[v].slot == s) return 1;
    }
    return 0;
}

static int evict_one(void)
{
    int s, best = -1;
    for (s = 0; s < DC_SOUND_SLOTS; s++) {
        if (!g_slots[s].handle || slot_playing(s)) continue;
        if (best < 0 || g_slots[s].used < g_slots[best].used) best = s;
    }
    if (best < 0) return 0;
    snd_sfx_unload(g_slots[best].handle);
    g_loaded_bytes -= g_slots[best].bytes;
    memset(&g_slots[best], 0, sizeof g_slots[best]);
    return 1;
}

static int free_slot(void)
{
    int s, oldest = -1;
    for (s = 0; s < DC_SOUND_SLOTS; s++) {
        if (g_slots[s].name[0] == '\0') return s;
    }
    for (s = 0; s < DC_SOUND_SLOTS; s++) {
        if (slot_playing(s)) continue;
        if (oldest < 0 || g_slots[s].used < g_slots[oldest].used) oldest = s;
    }
    if (oldest < 0) return -1;
    if (g_slots[oldest].handle) {
        snd_sfx_unload(g_slots[oldest].handle);
        g_loaded_bytes -= g_slots[oldest].bytes;
    }
    memset(&g_slots[oldest], 0, sizeof g_slots[oldest]);
    return oldest;
}

static int find_or_load(const char *name)
{
    char path[64];
    uint32 bytes, ms;
    int s;

    for (s = 0; s < DC_SOUND_SLOTS; s++) {
        if (g_slots[s].name[0] != '\0' && strcmp(g_slots[s].name, name) == 0) {
            g_slots[s].used = timer_ms_gettime64();
            return g_slots[s].missing ? -1 : s;
        }
    }

    s = free_slot();
    if (s < 0) return -1;
    strncpy(g_slots[s].name, name, sizeof g_slots[s].name - 1);
    g_slots[s].used = timer_ms_gettime64();

    snprintf(path, sizeof path, DC_SOUND_DIR "%s.wav", name);
    if (!read_wav_shape(path, &bytes, &ms)) { g_slots[s].missing = 1; return -1; }
    while (g_loaded_bytes + bytes > DC_SOUND_BUDGET && evict_one()) {}
    g_slots[s].handle = snd_sfx_load(path);
    if (!g_slots[s].handle && evict_one()) g_slots[s].handle = snd_sfx_load(path);
    if (!g_slots[s].handle) { memset(&g_slots[s], 0, sizeof g_slots[s]); return -1; }
    g_slots[s].bytes = bytes;
    g_slots[s].ms = ms;
    g_loaded_bytes += bytes;
    return s;
}

static void drop_voice(int v)
{
    g_voices[v] = g_voices[--g_voice_count];
}

static int start_voice(DcSoundVoice *voice)
{
    int chn = snd_sfx_play(g_slots[voice->slot].handle, voice->volume, 128);
    if (chn < 0) return 0;
    voice->chn = chn;
    voice->ends = timer_ms_gettime64() + g_slots[voice->slot].ms;
    return 1;
}

static void play(void *ctx, const char *name, int volume, int loops, int priority)
{
    DcSoundVoice voice;
    int s, v, weakest = -1;

    (void)ctx;
    if (loops == 0) return;
    s = find_or_load(name);
    if (s < 0) return;

    if (g_voice_count == DC_SOUND_VOICES) {
        for (v = 0; v < g_voice_count; v++) {
            if (weakest < 0 || g_voices[v].priority < g_voices[weakest].priority ||
                (g_voices[v].priority == g_voices[weakest].priority && g_voices[v].ends < g_voices[weakest].ends)) {
                weakest = v;
            }
        }
        if (g_voices[weakest].priority > priority) return;
        snd_sfx_stop(g_voices[weakest].chn);
        drop_voice(weakest);
    }

    voice.slot = s;
    voice.volume = volume * 255 / 100;
    voice.loops = loops;
    voice.priority = priority;
    if (start_voice(&voice)) g_voices[g_voice_count++] = voice;
}

static void stop(void *ctx)
{
    (void)ctx;
    snd_sfx_stop_all();
    g_voice_count = 0;
}

static void service(void *ctx)
{
    uint64 now = timer_ms_gettime64();
    int v = 0;

    (void)ctx;
    while (v < g_voice_count) {
        DcSoundVoice *voice = &g_voices[v];
        if (now < voice->ends) { v++; continue; }
        if (voice->loops == 1) { drop_voice(v); continue; }
        if (voice->loops > 1) voice->loops--;
        if (!start_voice(voice)) { drop_voice(v); continue; }
        v++;
    }
}

const cui_sound_t *dc_sound_init(void)
{
    if (snd_init() < 0) return 0;
    memset(g_slots, 0, sizeof g_slots);
    g_voice_count = 0;
    g_loaded_bytes = 0;
    g_port.play = play;
    g_port.stop = stop;
    g_port.service = service;
    g_port.ctx = 0;
    return &g_port;
}
