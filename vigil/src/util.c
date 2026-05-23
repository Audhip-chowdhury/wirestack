#include "util.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>

char *strtrim(char *s) {
    if (!s) return s;
    while (*s == ' ' || *s == '\t') s++;
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\n' || s[n - 1] == '\r'))
        s[--n] = '\0';
    return s;
}

uint64_t vigil_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

int vigil_mkdir_p(const char *path) {
    char tmp[512];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    return mkdir(tmp, 0755);
}

const char *ip_to_str(uint32_t ip, char *buf, size_t len) {
    struct in_addr a;
    a.s_addr = htonl(ip);
    inet_ntop(AF_INET, &a, buf, (socklen_t)len);
    return buf;
}
