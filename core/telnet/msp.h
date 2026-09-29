#ifndef MSP_H
#define MSP_H

#define MSP_NAME_MAX 24
#define MSP_BODY_MAX 128
#define MSP_OPENER_LEN 8

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int  music;
    char name[MSP_NAME_MAX + 1];
    int  volume;
    int  loops;
    int  priority;
} MspCue;

typedef void (*msp_text_fn)(void *ctx, char c, unsigned char attr);
typedef void (*msp_cue_fn)(void *ctx, const MspCue *cue);

typedef struct {
    int           held;
    int           in_body;
    int           music;
    char          buf[MSP_OPENER_LEN + MSP_BODY_MAX];
    unsigned char attr[MSP_OPENER_LEN + MSP_BODY_MAX];
} MspFilter;

void msp_init(MspFilter *m);

void msp_feed(MspFilter *m, char c, unsigned char attr,
              msp_text_fn text, msp_cue_fn cue, void *ctx);

int msp_parse(const char *body, int music, MspCue *out);

#ifdef __cplusplus
}
#endif
#endif
