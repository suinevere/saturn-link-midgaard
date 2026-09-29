#include "test_util.h"
#include "msp.h"

static char g_text[512];
static int  g_text_len;
static MspCue g_cues[8];
static int  g_cue_count;

static void text_sink(void *ctx, char c, unsigned char attr)
{
    (void)ctx;
    (void)attr;
    if (g_text_len < (int)sizeof(g_text) - 1) g_text[g_text_len++] = c;
    g_text[g_text_len] = '\0';
}

static void cue_sink(void *ctx, const MspCue *cue)
{
    (void)ctx;
    if (g_cue_count < 8) g_cues[g_cue_count++] = *cue;
}

static void reset(MspFilter *m)
{
    g_text_len = 0;
    g_text[0] = '\0';
    g_cue_count = 0;
    msp_init(m);
}

static void feed(MspFilter *m, const char *s)
{
    while (*s) msp_feed(m, *s++, 7, text_sink, cue_sink, 0);
}

TEST(plain_text_passes_through)
{
    MspFilter m;
    reset(&m);
    feed(&m, "You hit the orc!\n");
    CHECK_STR(g_text, "You hit the orc!\n");
    CHECK_INT(g_cue_count, 0);
}

TEST(a_sound_cue_is_removed_and_reported)
{
    MspFilter m;
    reset(&m);
    feed(&m, "You hit!!!SOUND(hit1.wav V=80 L=2 P=60 U=http://x/) the orc.\n");
    CHECK_STR(g_text, "You hit! the orc.\n");
    CHECK_INT(g_cue_count, 1);
    CHECK_INT(g_cues[0].music, 0);
    CHECK_STR(g_cues[0].name, "hit1");
    CHECK_INT(g_cues[0].volume, 80);
    CHECK_INT(g_cues[0].loops, 2);
    CHECK_INT(g_cues[0].priority, 60);
}

TEST(defaults_apply_when_settings_are_absent)
{
    MspFilter m;
    reset(&m);
    feed(&m, "!!SOUND(bark)");
    CHECK_INT(g_cue_count, 1);
    CHECK_STR(g_cues[0].name, "bark");
    CHECK_INT(g_cues[0].volume, 100);
    CHECK_INT(g_cues[0].loops, 1);
    CHECK_INT(g_cues[0].priority, 50);
}

TEST(a_music_cue_is_marked_as_music)
{
    MspFilter m;
    reset(&m);
    feed(&m, "!!MUSIC(theme.mid L=-1)");
    CHECK_INT(g_cue_count, 1);
    CHECK_INT(g_cues[0].music, 1);
    CHECK_STR(g_cues[0].name, "theme");
    CHECK_INT(g_cues[0].loops, -1);
}

TEST(a_cue_split_across_reads_is_still_found)
{
    MspFilter m;
    reset(&m);
    feed(&m, "A !!SOU");
    CHECK_STR(g_text, "A ");
    feed(&m, "ND(rain1 L=-1) B\n");
    CHECK_STR(g_text, "A  B\n");
    CHECK_INT(g_cue_count, 1);
    CHECK_STR(g_cues[0].name, "rain1");
}

TEST(a_near_miss_is_printed_as_text)
{
    MspFilter m;
    reset(&m);
    feed(&m, "Wow!! Sounds good!!!SOUNDS\n");
    CHECK_STR(g_text, "Wow!! Sounds good!!!SOUNDS\n");
    CHECK_INT(g_cue_count, 0);
}

TEST(an_extra_bang_before_a_cue_is_kept)
{
    MspFilter m;
    reset(&m);
    feed(&m, "!!!SOUND(bark)x");
    CHECK_STR(g_text, "!x");
    CHECK_INT(g_cue_count, 1);
}

TEST(a_cue_broken_by_a_newline_is_given_back_as_text)
{
    MspFilter m;
    reset(&m);
    feed(&m, "!!SOUND(bark\nnext\n");
    CHECK_STR(g_text, "!!SOUND(bark\nnext\n");
    CHECK_INT(g_cue_count, 0);
}

TEST(an_overlong_cue_is_given_back_as_text)
{
    MspFilter m;
    char big[MSP_BODY_MAX + 20];
    int i;
    reset(&m);
    feed(&m, "!!SOUND(");
    for (i = 0; i < MSP_BODY_MAX + 10; i++) big[i] = 'a';
    big[MSP_BODY_MAX + 10] = '\0';
    feed(&m, big);
    CHECK_INT(g_cue_count, 0);
    CHECK_INT(g_text_len, 8 + MSP_BODY_MAX + 10);
}

TEST(off_is_reported_so_playback_can_stop)
{
    MspFilter m;
    reset(&m);
    feed(&m, "!!SOUND(Off)");
    CHECK_INT(g_cue_count, 1);
    CHECK_STR(g_cues[0].name, "off");
}

TEST(names_lose_their_folder_extension_and_case)
{
    MspCue c;
    CHECK_INT(msp_parse("weather/Rain2.WAV", 0, &c), 1);
    CHECK_STR(c.name, "rain2");
    CHECK_INT(msp_parse("sfx\\Bark.mid", 0, &c), 1);
    CHECK_STR(c.name, "bark");
}

TEST(unsafe_or_empty_names_are_refused_but_still_stripped)
{
    MspFilter m;
    MspCue c;
    CHECK_INT(msp_parse("../..", 0, &c), 0);
    CHECK_INT(msp_parse("../../etc", 0, &c), 1);
    CHECK_STR(c.name, "etc");
    CHECK_INT(msp_parse("a b", 0, &c), 1);
    CHECK_STR(c.name, "a");
    CHECK_INT(msp_parse("", 0, &c), 0);
    CHECK_INT(msp_parse("thisnameismuchtoolongforanysoundfile", 0, &c), 0);
    reset(&m);
    feed(&m, "x!!SOUND(bad.name!)y");
    CHECK_STR(g_text, "xy");
    CHECK_INT(g_cue_count, 0);
}

TEST(volume_is_held_between_zero_and_a_hundred)
{
    MspCue c;
    msp_parse("bark V=250", 0, &c);
    CHECK_INT(c.volume, 100);
    msp_parse("bark V=-5", 0, &c);
    CHECK_INT(c.volume, 0);
}

int main(void)
{
    RUN(plain_text_passes_through);
    RUN(a_sound_cue_is_removed_and_reported);
    RUN(defaults_apply_when_settings_are_absent);
    RUN(a_music_cue_is_marked_as_music);
    RUN(a_cue_split_across_reads_is_still_found);
    RUN(a_near_miss_is_printed_as_text);
    RUN(an_extra_bang_before_a_cue_is_kept);
    RUN(a_cue_broken_by_a_newline_is_given_back_as_text);
    RUN(an_overlong_cue_is_given_back_as_text);
    RUN(off_is_reported_so_playback_can_stop);
    RUN(names_lose_their_folder_extension_and_case);
    RUN(unsafe_or_empty_names_are_refused_but_still_stripped);
    RUN(volume_is_held_between_zero_and_a_hundred);
    TEST_MAIN_END();
}
