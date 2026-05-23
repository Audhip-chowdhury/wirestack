#ifndef VIGIL_CLI_QUERY_H
#define VIGIL_CLI_QUERY_H

#include <stddef.h>

int query_send(const char *socket_path, const char *json_cmd, char *response, size_t resp_len);

#endif
