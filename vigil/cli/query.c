#include "query.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int query_send(const char *socket_path, const char *json_cmd, char *response, size_t resp_len) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    send(fd, json_cmd, strlen(json_cmd), 0);
    ssize_t n = recv(fd, response, resp_len - 1, 0);
    close(fd);
    if (n <= 0) return -1;
    response[n] = '\0';
    return 0;
}
