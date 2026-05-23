#include "packet.h"
#include <stdint.h>

/* Link-time stubs for daemon symbols defined in main.c */
volatile int g_running = 0;

void vigil_on_packet(packet_info_t *info, uint32_t caplen, uint32_t wire_len)
{
    (void)info;
    (void)caplen;
    (void)wire_len;
}
