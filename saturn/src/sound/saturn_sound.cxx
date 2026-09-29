#include <srl.hpp>
#include <string.h>
#include "saturn_sound.h"

#define SAT_SOUND_VOICES     4
#define SAT_SOUND_SLOTS      24
#define SAT_SOUND_BUDGET     (192 * 1024)
#define SAT_SOUND_SECTOR     2048
#define SAT_SOUND_MIN_BYTES  0x900
#define SAT_SOUND_ENTRY      36
#define SAT_SOUND_NAME       24
#define SAT_SOUND_MAX_COUNT  512
#define SAT_SOUND_GRACE      4

typedef struct {
    char     name[SAT_SOUND_NAME + 1];
    uint32_t sector;
    uint32_t bytes;
    uint16_t rate;
} SatSoundEntry;

typedef struct {
    int      entry;
    int8_t  *data;
    uint32_t alloc;
    uint32_t used;
} SatSoundSlot;

typedef struct {
    int      active;
    int      slot;
    int      loops;
    int      priority;
    int      age;
    PCM      pcm;
} SatSoundVoice;

static SatSoundEntry *g_index;
static int            g_count;
static SatSoundSlot   g_slots[SAT_SOUND_SLOTS];
static SatSoundVoice  g_voices[SAT_SOUND_VOICES];
static uint32_t       g_loaded;
static uint32_t       g_clock;
static cui_sound_t    g_port;

static const uint8_t VOICE_CHANNELS[SAT_SOUND_VOICES] = { 0, 2, 4, 6 };

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint32_t round_to_sector(uint32_t n)
{
    return (n + SAT_SOUND_SECTOR - 1) & ~(uint32_t)(SAT_SOUND_SECTOR - 1);
}

static uint16_t pitch_for(uint16_t rate)
{
    int32_t oct = 0;
    int32_t n = 44100L / ((int32_t)rate + 1);
    int32_t shift, fns;

    while (n > 0) { oct++; n >>= 1; }
    shift = 44100L >> oct;
    fns = (((int32_t)rate - shift) << 10) / shift;
    return (uint16_t)(((-oct) & 0xF) << 11 | (fns & 0x3FF));
}

static int load_index(void)
{
    SRL::Cd::File pak("SOUNDS.PAK");
    uint8_t *head;
    uint32_t count, bytes;
    int i;

    if (!pak.Exists()) return 0;
    head = new uint8_t[SAT_SOUND_SECTOR];
    if (!head) return 0;
    if (pak.LoadBytes(0, SAT_SOUND_SECTOR, head) < 8 || head[0] != 'S' || head[1] != 'N' ||
        head[2] != 'D' || head[3] != 'P') {
        delete[] head;
        return 0;
    }
    count = be32(head + 4);
    delete[] head;
    if (count == 0 || count > SAT_SOUND_MAX_COUNT) return 0;

    bytes = round_to_sector(8 + count * SAT_SOUND_ENTRY);
    head = new uint8_t[bytes];
    g_index = new SatSoundEntry[count];
    if (!head || !g_index) return 0;
    if (pak.LoadBytes(0, (int32_t)bytes, head) < (int32_t)(8 + count * SAT_SOUND_ENTRY)) {
        delete[] head;
        return 0;
    }
    for (i = 0; i < (int)count; i++) {
        const uint8_t *e = head + 8 + i * SAT_SOUND_ENTRY;
        int k;
        for (k = 0; k < SAT_SOUND_NAME; k++) g_index[i].name[k] = (char)e[k];
        g_index[i].name[SAT_SOUND_NAME] = '\0';
        g_index[i].sector = be32(e + 24);
        g_index[i].bytes = be32(e + 28);
        g_index[i].rate = (uint16_t)((e[32] << 8) | e[33]);
    }
    delete[] head;
    g_count = (int)count;
    return 1;
}

static int find_entry(const char *name)
{
    int lo = 0, hi = g_count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int c = strcmp(g_index[mid].name, name);
        if (c == 0) return mid;
        if (c < 0) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

static int slot_playing(int s)
{
    int v;
    for (v = 0; v < SAT_SOUND_VOICES; v++) {
        if (g_voices[v].active && g_voices[v].slot == s) return 1;
    }
    return 0;
}

static void free_slot(int s)
{
    delete[] g_slots[s].data;
    g_loaded -= g_slots[s].alloc;
    g_slots[s].data = 0;
    g_slots[s].alloc = 0;
    g_slots[s].entry = -1;
}

static int evict_one(void)
{
    int s, best = -1;
    for (s = 0; s < SAT_SOUND_SLOTS; s++) {
        if (!g_slots[s].data || slot_playing(s)) continue;
        if (best < 0 || g_slots[s].used < g_slots[best].used) best = s;
    }
    if (best < 0) return 0;
    free_slot(best);
    return 1;
}

static int load_slot(int entry)
{
    SRL::Cd::File pak("SOUNDS.PAK");
    uint32_t alloc, k;
    int s;

    for (s = 0; s < SAT_SOUND_SLOTS; s++) {
        if (g_slots[s].data && g_slots[s].entry == entry) {
            g_slots[s].used = ++g_clock;
            return s;
        }
    }

    alloc = round_to_sector(g_index[entry].bytes < SAT_SOUND_MIN_BYTES ? SAT_SOUND_MIN_BYTES : g_index[entry].bytes);
    while (g_loaded + alloc > SAT_SOUND_BUDGET && evict_one()) {}
    for (s = 0; s < SAT_SOUND_SLOTS && g_slots[s].data; s++) {}
    if (s == SAT_SOUND_SLOTS) {
        if (!evict_one()) return -1;
        for (s = 0; s < SAT_SOUND_SLOTS && g_slots[s].data; s++) {}
    }

    g_slots[s].data = new int8_t[alloc];
    if (!g_slots[s].data) return -1;
    if (pak.LoadBytes(g_index[entry].sector, (int32_t)round_to_sector(g_index[entry].bytes), g_slots[s].data) <
        (int32_t)g_index[entry].bytes) {
        delete[] g_slots[s].data;
        g_slots[s].data = 0;
        return -1;
    }
    for (k = g_index[entry].bytes; k < alloc; k++) g_slots[s].data[k] = 0;
    g_slots[s].entry = entry;
    g_slots[s].alloc = alloc;
    g_slots[s].used = ++g_clock;
    g_loaded += alloc;
    return s;
}

static uint32_t play_length(int s)
{
    uint32_t n = g_index[g_slots[s].entry].bytes;
    return n < SAT_SOUND_MIN_BYTES ? SAT_SOUND_MIN_BYTES : n;
}

static int start_voice(SatSoundVoice *v)
{
    v->age = 0;
    return slPCMOn(&v->pcm, g_slots[v->slot].data, play_length(v->slot)) >= 0;
}

static void stop_voice(SatSoundVoice *v)
{
    if (v->active) slPCMOff(&v->pcm);
    v->active = 0;
}

static void play(void *ctx, const char *name, int volume, int loops, int priority)
{
    SatSoundVoice *v = 0;
    int entry, s, i;

    (void)ctx;
    if (loops == 0 || g_count == 0) return;
    entry = find_entry(name);
    if (entry < 0) return;

    for (i = 0; i < SAT_SOUND_VOICES; i++) {
        if (!g_voices[i].active) { v = &g_voices[i]; break; }
    }
    if (!v) {
        for (i = 0; i < SAT_SOUND_VOICES; i++) {
            if (!v || g_voices[i].priority < v->priority) v = &g_voices[i];
        }
        if (v->priority > priority) return;
        stop_voice(v);
    }

    s = load_slot(entry);
    if (s < 0) return;

    v->slot = s;
    v->loops = loops;
    v->priority = priority;
    v->pcm.mode = _Mono | _PCM8Bit;
    v->pcm.channel = VOICE_CHANNELS[v - g_voices];
    v->pcm.level = (uint8_t)(volume * 127 / 100);
    v->pcm.pan = 0;
    v->pcm.pitch = pitch_for(g_index[entry].rate);
    v->pcm.eflevelR = 0;
    v->pcm.efselectR = 0;
    v->pcm.eflevelL = 0;
    v->pcm.efselectL = 0;
    v->active = start_voice(v);
}

static void stop(void *ctx)
{
    int i;
    (void)ctx;
    for (i = 0; i < SAT_SOUND_VOICES; i++) stop_voice(&g_voices[i]);
}

static void service(void *ctx)
{
    int i;
    (void)ctx;
    for (i = 0; i < SAT_SOUND_VOICES; i++) {
        SatSoundVoice *v = &g_voices[i];
        if (!v->active) continue;
        if (v->age < SAT_SOUND_GRACE) { v->age++; continue; }
        if (slPCMStat(&v->pcm)) continue;
        if (v->loops == 1) { v->active = 0; continue; }
        if (v->loops > 1) v->loops--;
        v->active = start_voice(v);
    }
}

const cui_sound_t *saturn_sound_init(void)
{
    int i;

    g_count = 0;
    g_loaded = 0;
    g_clock = 0;
    for (i = 0; i < SAT_SOUND_SLOTS; i++) {
        g_slots[i].data = 0;
        g_slots[i].alloc = 0;
        g_slots[i].entry = -1;
    }
    for (i = 0; i < SAT_SOUND_VOICES; i++) g_voices[i].active = 0;
    if (!load_index()) return 0;

    g_port.play = play;
    g_port.stop = stop;
    g_port.service = service;
    g_port.ctx = 0;
    return &g_port;
}
