#include "ipc.h"
#include "logger.h"
#include "config.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

static int g_ipc_fd = -1;
static pthread_t g_ipc_thread;
static volatile int g_ipc_running;
static storage_ctx_t *g_storage;
static conn_table_t *g_conn;

extern volatile int g_running;

void ipc_set_context(storage_ctx_t *s, conn_table_t *c) {
    g_storage = s;
    g_conn = c;
}

static void handle_command(int client, const char *req) {
    char response[4096];
    if (strstr(req, "\"stop\"") || strstr(req, "stop")) {
        g_running = 0;
        snprintf(response, sizeof(response), "{\"status\":\"ok\",\"message\":\"stopping\"}");
    } else if (strstr(req, "\"reload\"") || strstr(req, "reload")) {
        config_reload(&g_config, g_config_path, g_storage);
        snprintf(response, sizeof(response), "{\"status\":\"ok\",\"message\":\"reloaded\"}");
    } else if (strstr(req, "\"status\"") || strstr(req, "status")) {
        snprintf(response, sizeof(response),
            "{\"status\":\"ok\",\"data\":{\"uptime\":%ld,\"running\":%d}}",
            (long)time(NULL), g_running);
    } else if (strstr(req, "\"stats\"") || strstr(req, "stats")) {
        snprintf(response, sizeof(response),
            "{\"status\":\"ok\",\"data\":{\"packets_in\":0,\"bytes_in\":0}}");
        (void)g_storage;
    } else if (strstr(req, "\"connections\"") || strstr(req, "connections")) {
        uint32_t count = g_conn ? g_conn->count : 0;
        snprintf(response, sizeof(response),
            "{\"status\":\"ok\",\"data\":{\"active_connections\":%u}}", count);
    } else if (strstr(req, "\"protocols\"") || strstr(req, "protocols")) {
        snprintf(response, sizeof(response), "{\"status\":\"ok\",\"data\":{}}");
    } else if (strstr(req, "\"alerts\"") || strstr(req, "alerts")) {
        snprintf(response, sizeof(response), "{\"status\":\"ok\",\"data\":[]}");
    } else if (strstr(req, "\"report\"") || strstr(req, "report")) {
        snprintf(response, sizeof(response), "{\"status\":\"ok\",\"data\":{}}");
    } else {
        snprintf(response, sizeof(response),
            "{\"status\":\"error\",\"message\":\"Unknown command\"}");
    }

    send(client, response, strlen(response), 0);
}

static void *ipc_accept_loop(void *arg) {
    (void)arg;
    while (g_ipc_running) {
        int client = accept(g_ipc_fd, NULL, NULL);
        if (client < 0) continue;

        char buf[2048];
        ssize_t n = recv(client, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = '\0';
            handle_command(client, buf);
        }
        close(client);
    }
    return NULL;
}

int ipc_start(const char *socket_path) {
    g_ipc_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (g_ipc_fd < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    unlink(socket_path);

    if (bind(g_ipc_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(g_ipc_fd);
        return -1;
    }
    chmod(socket_path, 0666);
    if (listen(g_ipc_fd, 8) < 0) {
        close(g_ipc_fd);
        return -1;
    }

    g_ipc_running = 1;
    pthread_create(&g_ipc_thread, NULL, ipc_accept_loop, NULL);
    log_write(LOG_INFO, "IPC listening on %s", socket_path);
    return 0;
}

void ipc_stop(void) {
    g_ipc_running = 0;
    if (g_ipc_fd >= 0) {
        close(g_ipc_fd);
        g_ipc_fd = -1;
    }
}
