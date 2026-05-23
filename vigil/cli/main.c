#include "query.h"
#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOCK_PATH "/tmp/vigil.sock"

static void usage(void) {
    printf("Usage: vigil-cli <command> [options]\n");
    printf("Commands: status, stats, connections, protocols, alerts, report, reload, stop\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
        return 1;
    }

    const char *cmd = argv[1];
    char json[512];
    char response[4096];

    if (strcmp(cmd, "report") == 0) {
        snprintf(json, sizeof(json), "{\"command\":\"report\"}");
    } else if (strcmp(cmd, "stats") == 0) {
        const char *iface = "";
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--interface") == 0 && i + 1 < argc)
                iface = argv[++i];
        }
        snprintf(json, sizeof(json), "{\"command\":\"stats\",\"interface\":\"%s\"}", iface);
    } else if (strcmp(cmd, "connections") == 0) {
        int top = 10;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--top") == 0 && i + 1 < argc)
                top = atoi(argv[++i]);
        }
        snprintf(json, sizeof(json), "{\"command\":\"connections\",\"limit\":%d}", top);
    } else {
        snprintf(json, sizeof(json), "{\"command\":\"%s\"}", cmd);
    }

    if (query_send(SOCK_PATH, json, response, sizeof(response)) != 0) {
        fprintf(stderr, "Failed to connect to daemon at %s\n", SOCK_PATH);
        return 1;
    }

    if (strcmp(cmd, "status") == 0) display_status(response);
    else if (strcmp(cmd, "stats") == 0) display_stats(response);
    else if (strcmp(cmd, "connections") == 0) display_connections(response);
    else display_generic(response);

    return 0;
}
