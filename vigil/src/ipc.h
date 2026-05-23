#ifndef VIGIL_IPC_H
#define VIGIL_IPC_H

#include "config.h"
#include "storage.h"
#include "conn.h"

#define VIGIL_SOCK_DEFAULT "/tmp/vigil.sock"

int ipc_start(const char *socket_path);
void ipc_stop(void);
void ipc_set_context(storage_ctx_t *s, conn_table_t *c);

#endif
