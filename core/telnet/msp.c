#include "msp.h"

static const char SOUND_OPENER[] = "!!SOUND(";
static const char MUSIC_OPENER[] = "!!MUSIC(";

void msp_init(MspFilter *m)
{
    m->held = 0;
    m->in_body = 0;
    m->music = 0;
}

static int is_prefix(const char *held, int n, const char *opener)
{
    int i;
    for (i = 0; i < n; i++) {
        if (held[i] != opener[i]) return 0;
    }
    return 1;
}

static int opener_prefix(const MspFilter *m)
{
    return is_prefix(m->buf, m->held, SOUND_OPENER) || is_prefix(m->buf, m->held, MUSIC_OPENER);
}

static void give_back(MspFilter *m, int n, msp_text_fn text, void *ctx)
{
    int i;
    for (i = 0; i < n; i++) text(ctx, m->buf[i], m->attr[i]);
    for (i = n; i < m->held; i++) {
        m->buf[i - n] = m->buf[i];
        m->attr[i - n] = m->attr[i];
    }
    m->held -= n;
}

static int lower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
}

static int ends_with(const char *s, int len, const char *tail)
{
    int n = 0, i;
    while (tail[n] != '\0') n++;
    if (len < n) return 0;
    for (i = 0; i < n; i++) {
        if (lower(s[len - n + i]) != tail[i]) return 0;
    }
    return 1;
}

static int read_int(const char *s, int *out)
{
    int neg = 0, v = 0, any = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') {
        if (v < 100000) v = v * 10 + (*s - '0');
        any = 1;
        s++;
    }
    if (!any || (*s != '\0' && *s != ' ')) return 0;
    *out = neg ? -v : v;
    return 1;
}

int msp_parse(const char *body, int music, MspCue *out)
{
    const char *p = body;
    const char *start, *end, *base;
    int len, i;

    out->music = music;
    out->volume = 100;
    out->loops = 1;
    out->priority = 50;
    out->name[0] = '\0';

    while (*p == ' ') p++;
    start = p;
    while (*p != '\0' && *p != ' ') p++;
    end = p;

    base = start;
    for (i = 0; start + i < end; i++) {
        if (start[i] == '/' || start[i] == '\\') base = start + i + 1;
    }
    len = (int)(end - base);
    if (ends_with(base, len, ".wav") || ends_with(base, len, ".mid") || ends_with(base, len, ".mp3") ||
        ends_with(base, len, ".ogg")) {
        len -= 4;
    } else if (ends_with(base, len, ".midi")) {
        len -= 5;
    }

    while (*p != '\0') {
        int value;
        while (*p == ' ') p++;
        if ((p[0] == 'V' || p[0] == 'v' || p[0] == 'L' || p[0] == 'l' || p[0] == 'P' || p[0] == 'p') &&
            p[1] == '=' && read_int(p + 2, &value)) {
            int key = lower(p[0]);
            if (key == 'v') out->volume = value < 0 ? 0 : (value > 100 ? 100 : value);
            else if (key == 'l') out->loops = value;
            else out->priority = value;
        }
        while (*p != '\0' && *p != ' ') p++;
    }

    if (len <= 0 || len > MSP_NAME_MAX) return 0;
    for (i = 0; i < len; i++) {
        int c = lower(base[i]);
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) return 0;
        out->name[i] = (char)c;
    }
    out->name[len] = '\0';
    return 1;
}

static void finish_cue(MspFilter *m, msp_cue_fn cue, void *ctx)
{
    MspCue c;
    m->buf[m->held] = '\0';
    if (msp_parse(m->buf + MSP_OPENER_LEN, m->music, &c) && cue) cue(ctx, &c);
    m->held = 0;
    m->in_body = 0;
}

void msp_feed(MspFilter *m, char c, unsigned char attr,
              msp_text_fn text, msp_cue_fn cue, void *ctx)
{
    if (m->in_body) {
        if (c == ')') { finish_cue(m, cue, ctx); return; }
        if (c == '\n' || m->held >= MSP_OPENER_LEN + MSP_BODY_MAX - 1) {
            m->in_body = 0;
            give_back(m, m->held, text, ctx);
            text(ctx, c, attr);
            return;
        }
        m->buf[m->held] = c;
        m->attr[m->held++] = attr;
        return;
    }

    m->buf[m->held] = c;
    m->attr[m->held++] = attr;
    while (m->held > 0 && !opener_prefix(m)) give_back(m, 1, text, ctx);
    if (m->held == MSP_OPENER_LEN) {
        m->in_body = 1;
        m->music = m->buf[2] == 'M';
    }
}
