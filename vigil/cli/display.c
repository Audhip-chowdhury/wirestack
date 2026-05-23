#include "display.h"
#include <stdio.h>
#include <string.h>

void display_status(const char *json) {
    printf("=== vigil status ===\n%s\n", json);
}

void display_stats(const char *json) {
    printf("=== traffic stats ===\n%s\n", json);
}

void display_connections(const char *json) {
    printf("=== connections ===\n%s\n", json);
}

void display_generic(const char *json) {
    printf("%s\n", json);
}
