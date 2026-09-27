#include "authproxy.h"

int authproxy_hello(unsigned char *out, int cap, const char *secret)
{
    int n = 0, i;

    while (secret[n] != '\0') n++;
    if (n == 0 || n > 255 || cap < 5 + n) return -1;

    out[0] = 'A';
    out[1] = 'U';
    out[2] = 'T';
    out[3] = 'H';
    out[4] = (unsigned char)n;
    for (i = 0; i < n; i++) out[5 + i] = (unsigned char)secret[i];
    return 5 + n;
}
