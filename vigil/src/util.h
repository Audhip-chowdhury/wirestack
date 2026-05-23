#ifndef VIGIL_UTIL_H
#define VIGIL_UTIL_H

#include <stdint.h>
#include <time.h>

char *strtrim(char *s);
uint64_t vigil_timestamp_us(void);
int vigil_mkdir_p(const char *path);
const char *ip_to_str(uint32_t ip, char *buf, size_t len);

#endif
