#ifndef TEST_UTIL_H
#define TEST_UTIL_H
#include <stdio.h>
#include <string.h>

static int t_failures;
static const char *t_current = "";

#define TEST(name) static void name(void)
#define RUN(name)  do { t_current = #name; name(); } while (0)

#define CHECK(cond) do { \
    if (!(cond)) { \
        printf("FAIL %s:%d  %s  (%s)\n", __FILE__, __LINE__, t_current, #cond); \
        t_failures++; \
    } \
} while (0)

#define CHECK_INT(got, want) do { \
    long g_ = (long)(got), w_ = (long)(want); \
    if (g_ != w_) { \
        printf("FAIL %s:%d  %s  got %ld want %ld\n", \
               __FILE__, __LINE__, t_current, g_, w_); \
        t_failures++; \
    } \
} while (0)

#define CHECK_STR(got, want) do { \
    if (strcmp((got), (want)) != 0) { \
        printf("FAIL %s:%d  %s\n  got  \"%s\"\n  want \"%s\"\n", \
               __FILE__, __LINE__, t_current, (got), (want)); \
        t_failures++; \
    } \
} while (0)

#define CHECK_MEM(got, want, n) do { \
    if (memcmp((got), (want), (size_t)(n)) != 0) { \
        int i_; \
        printf("FAIL %s:%d  %s  bytes differ:\n", __FILE__, __LINE__, t_current); \
        printf("  got "); \
        for (i_ = 0; i_ < (int)(n); i_++) printf(" %02x", ((const unsigned char *)(got))[i_]); \
        printf("\n  want"); \
        for (i_ = 0; i_ < (int)(n); i_++) printf(" %02x", ((const unsigned char *)(want))[i_]); \
        printf("\n"); \
        t_failures++; \
    } \
} while (0)

#define TEST_MAIN_END() do { \
    if (t_failures == 0) { printf("ok   %s\n", __FILE__); return 0; } \
    printf("FAILED %s: %d failure(s)\n", __FILE__, t_failures); return 1; \
} while (0)

#endif
