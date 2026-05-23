# vigil — Product Specification

> **Product**: vigil
> **Company**: WireStack
> **Type**: Network monitoring daemon
> **Language**: C (C99 standard)
> **Platform**: Linux (primary), Windows via Npcap (secondary)
> **Storage**: SQLite via C API
> **Testing**: Check framework + shell integration tests

---

## What is vigil?

vigil is WireStack's core network monitoring daemon. It captures live packet data from network interfaces, tracks active connections, analyzes protocol distribution, fires configurable alerts when thresholds are exceeded, and stores historical traffic data for reporting. It runs as a background daemon and exposes a companion CLI tool (`vigil-cli`) for live querying and control.

---

## Project Structure

```
vigil/
├── Makefile
├── vigil.conf.example
├── README.md
├── PRODUCT.md
├── .gitignore
├── src/
│   ├── main.c                  # Entry point, daemon setup
│   ├── config.c / config.h     # Config file + CLI flag parsing
│   ├── capture.c / capture.h   # libpcap packet capture loop
│   ├── packet.c / packet.h     # Packet parsing and dissection
│   ├── connection.c / conn.h   # Connection tracking table
│   ├── protocol.c / proto.h    # Protocol analysis (TCP/UDP/ICMP/DNS/HTTP)
│   ├── alert.c / alert.h       # Threshold alerting + script execution
│   ├── stats.c / stats.h       # Traffic statistics collection
│   ├── storage.c / storage.h   # SQLite persistence layer
│   ├── logger.c / logger.h     # Log file writer with rotation
│   ├── report.c / report.h     # Report generation
│   ├── multiface.c / mface.h   # Multi-interface management
│   ├── anomaly.c / anomaly.h   # Anomaly detection engine
│   └── util.c / util.h         # Shared utilities
├── cli/
│   ├── main.c                  # vigil-cli entry point
│   ├── query.c / query.h       # Query vigil daemon via Unix socket
│   └── display.c / display.h   # Terminal output formatting
├── tests/
│   ├── test_config.c
│   ├── test_packet.c
│   ├── test_connection.c
│   ├── test_protocol.c
│   ├── test_alert.c
│   ├── test_stats.c
│   ├── test_storage.c
│   ├── test_report.c
│   ├── test_multiface.c
│   ├── test_anomaly.c
│   └── integration/
│       ├── test_capture.sh
│       ├── test_alert_script.sh
│       └── test_cli_query.sh
├── data/
│   └── vigil.db                # SQLite database (gitignored)
└── logs/
    └── vigil.log               # Log output (gitignored)
```

---

## Build System

### Makefile

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -D_GNU_SOURCE
LDFLAGS = -lpcap -lsqlite3 -lpthread -lm

# Linux
LIBS = -lpcap -lsqlite3 -lpthread -lm

# Windows (Npcap): override with
# LIBS = -lwpcap -lsqlite3 -lpthread -lm -lws2_32

SRC_DIR = src
CLI_DIR = cli
TEST_DIR = tests
BUILD_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
CLI_SRCS = $(wildcard $(CLI_DIR)/*.c)
DAEMON_SRCS = $(filter-out $(SRC_DIR)/main.c, $(SRCS))

DAEMON_OBJ = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
CLI_OBJ = $(CLI_SRCS:$(CLI_DIR)/%.c=$(BUILD_DIR)/cli_%.o)

.PHONY: all clean test install

all: vigil vigil-cli

vigil: $(DAEMON_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

vigil-cli: $(CLI_OBJ) $(DAEMON_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/cli_%.o: $(CLI_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: all
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/test_runner \
		$(TEST_DIR)/test_*.c \
		$(DAEMON_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o) \
		$(LIBS) -lcheck -lm -lpthread -lrt
	./$(BUILD_DIR)/test_runner

clean:
	rm -rf $(BUILD_DIR) vigil vigil-cli

install:
	cp vigil /usr/local/bin/
	cp vigil-cli /usr/local/bin/
	cp vigil.conf.example /etc/vigil.conf
```

---

## Dependencies

### Linux
```bash
sudo apt install libpcap-dev libsqlite3-dev check gcc make
```

### Windows
1. Install [Npcap](https://npcap.com) with WinPcap compatibility mode
2. Install [SQLite amalgamation](https://sqlite.org/amalgamation.html)
3. Install Check via vcpkg or MinGW
4. Use `mingw32-make` instead of `make`

---

## Configuration File Format

### vigil.conf.example
```ini
# vigil configuration file
# CLI flags override these values

[capture]
interface = eth0
snaplen = 65535
promiscuous = true
timeout_ms = 1000

[storage]
db_path = ./data/vigil.db
max_connections = 10000
flush_interval_sec = 30

[logging]
log_path = ./logs/vigil.log
log_level = INFO        # DEBUG, INFO, WARN, ERROR
max_log_size_mb = 100
rotate_count = 5

[alerts]
enabled = true
threshold_pps = 10000           # packets per second
threshold_bps = 104857600       # bytes per second (100 Mbps)
alert_command = /etc/vigil/alert.sh
cooldown_sec = 60

[reporting]
interval_sec = 300
output_path = ./logs/reports/

[interfaces]
# comma-separated for multi-interface (Phase 7)
# interfaces = eth0,eth1,wlan0

[anomaly]
# Phase 8
enabled = false
baseline_window_sec = 3600
deviation_threshold = 3.0
```

---

## CLI Interface

### vigil daemon
```bash
# Start daemon
sudo vigil --config /etc/vigil.conf
sudo vigil --interface eth0 --log-level DEBUG

# Flags (override config file)
--interface <iface>       Network interface to capture on
--config <path>           Config file path (default: ./vigil.conf)
--db <path>               SQLite database path
--log <path>              Log file path
--log-level <level>       DEBUG|INFO|WARN|ERROR
--daemon                  Fork to background
--pid-file <path>         PID file when running as daemon
--snaplen <bytes>         Packet snapshot length
--promiscuous             Enable promiscuous mode
```

### vigil-cli
```bash
# Query live daemon
vigil-cli status                          # daemon health + uptime
vigil-cli stats                           # current traffic statistics
vigil-cli stats --interface eth0          # per-interface stats
vigil-cli connections                     # active connection table
vigil-cli connections --top 20            # top 20 by packet count
vigil-cli protocols                       # protocol breakdown
vigil-cli alerts                          # recent alert history
vigil-cli alerts --since 1h              # last 1 hour
vigil-cli report --from 2025-01-01 --to 2025-01-31   # historical report
vigil-cli reload                          # reload config without restart
vigil-cli stop                            # graceful shutdown
```

---

## Database Schema

All SQLite tables created by vigil on first run:

```sql
-- Captured packet summary (not raw packets — summaries per interval)
CREATE TABLE IF NOT EXISTS traffic_stats (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    interface TEXT NOT NULL,
    timestamp INTEGER NOT NULL,         -- Unix timestamp
    interval_sec INTEGER NOT NULL,
    packets_in INTEGER NOT NULL DEFAULT 0,
    packets_out INTEGER NOT NULL DEFAULT 0,
    bytes_in INTEGER NOT NULL DEFAULT 0,
    bytes_out INTEGER NOT NULL DEFAULT 0,
    dropped_packets INTEGER NOT NULL DEFAULT 0
);

CREATE INDEX IF NOT EXISTS idx_stats_time ON traffic_stats(timestamp DESC);
CREATE INDEX IF NOT EXISTS idx_stats_iface ON traffic_stats(interface, timestamp DESC);

-- Connection tracking
CREATE TABLE IF NOT EXISTS connections (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    src_ip TEXT NOT NULL,
    dst_ip TEXT NOT NULL,
    src_port INTEGER,
    dst_port INTEGER,
    protocol TEXT NOT NULL,
    first_seen INTEGER NOT NULL,
    last_seen INTEGER NOT NULL,
    packets INTEGER NOT NULL DEFAULT 0,
    bytes INTEGER NOT NULL DEFAULT 0,
    state TEXT NOT NULL DEFAULT 'ACTIVE',  -- ACTIVE, CLOSED, TIMEOUT
    interface TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_conn_src ON connections(src_ip, timestamp DESC);
CREATE INDEX IF NOT EXISTS idx_conn_active ON connections(state) WHERE state = 'ACTIVE';

-- Protocol distribution per interval
CREATE TABLE IF NOT EXISTS protocol_stats (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    interface TEXT NOT NULL,
    timestamp INTEGER NOT NULL,
    protocol TEXT NOT NULL,
    packet_count INTEGER NOT NULL DEFAULT 0,
    byte_count INTEGER NOT NULL DEFAULT 0
);

CREATE INDEX IF NOT EXISTS idx_proto_time ON protocol_stats(timestamp DESC);

-- Alert history
CREATE TABLE IF NOT EXISTS alerts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp INTEGER NOT NULL,
    interface TEXT NOT NULL,
    alert_type TEXT NOT NULL,   -- 'pps_threshold', 'bps_threshold', 'anomaly', 'port_scan'
    threshold_value INTEGER NOT NULL,
    actual_value INTEGER NOT NULL,
    message TEXT NOT NULL,
    script_exit_code INTEGER,
    resolved_at INTEGER
);

-- Reports
CREATE TABLE IF NOT EXISTS reports (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    generated_at INTEGER NOT NULL,
    period_start INTEGER NOT NULL,
    period_end INTEGER NOT NULL,
    interface TEXT NOT NULL,
    total_packets INTEGER NOT NULL,
    total_bytes INTEGER NOT NULL,
    peak_pps INTEGER NOT NULL,
    peak_bps INTEGER NOT NULL,
    alert_count INTEGER NOT NULL,
    top_talkers TEXT,    -- JSON array
    protocol_breakdown TEXT  -- JSON object
);

-- Config snapshots (audit trail)
CREATE TABLE IF NOT EXISTS config_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp INTEGER NOT NULL,
    config_key TEXT NOT NULL,
    old_value TEXT,
    new_value TEXT NOT NULL,
    changed_by TEXT NOT NULL  -- 'file', 'cli', 'reload'
);
```

---

## Phase Overview

| Phase | Name | New Files | Bugs |
|-------|------|-----------|------|
| 1 | Packet Capture & Counting | capture.c, packet.c, stats.c, storage.c, logger.c | VG-001 to VG-003 |
| 2 | Connection Tracking | connection.c | VG-004 to VG-006 |
| 3 | Protocol Analysis | protocol.c | VG-007 to VG-008 |
| 4 | Traffic Threshold Alerting | alert.c | VG-009 to VG-010 |
| 5 | Log Rotation & Reporting | report.c (extend logger.c) | VG-011 to VG-013 |
| 6 | Config File Management | config.c (extend all) | VG-014 to VG-016 |
| 7 | Multi-Interface Support | multiface.c (extend all) | VG-017 to VG-018 |
| 8 | Performance Metrics & Anomaly Detection | anomaly.c | VG-019 to VG-021 |

---

## Phase 1 — Packet Capture & Counting

### Goal
Set up the full project. vigil opens a network interface via libpcap, captures packets in a loop, counts them per interval, writes stats to SQLite, and logs to file.

### What to Build
- `Makefile` — full build system as specified above
- `src/main.c` — argument parsing, initialization, main loop
- `src/capture.c` — libpcap setup, packet capture callback
- `src/packet.c` — basic packet parsing (Ethernet → IP → Transport)
- `src/stats.c` — per-interval counter aggregation
- `src/storage.c` — SQLite init, schema creation, insert/query helpers
- `src/logger.c` — log file writer (INFO/WARN/ERROR/DEBUG levels)
- `src/util.c` — timestamp helpers, string utilities, safe string functions
- `tests/test_packet.c` — Check unit tests for packet parsing
- `tests/test_storage.c` — Check unit tests for SQLite operations
- `vigil.conf.example`

### Key Data Structures

```c
// src/packet.h
typedef struct {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t  protocol;     // IPPROTO_TCP, IPPROTO_UDP, IPPROTO_ICMP
    uint32_t length;
    uint64_t timestamp;    // microseconds since epoch
    char     interface[16];
    uint8_t  direction;    // 0=in, 1=out
} packet_info_t;

// src/stats.h
typedef struct {
    uint64_t packets_in;
    uint64_t packets_out;
    uint64_t bytes_in;
    uint64_t bytes_out;
    uint64_t dropped;
    time_t   interval_start;
    int      interval_sec;
    char     interface[16];
} traffic_stats_t;

// src/storage.h
typedef struct {
    sqlite3 *db;
    char     db_path[256];
    int      flush_interval;
    time_t   last_flush;
} storage_ctx_t;
```

### Bugs

**🐛 VG-001 — INJECT THIS (Critical)**
In `src/capture.c`, the packet capture callback:

```c
void packet_callback(u_char *user_data, const struct pcap_pkthdr *header,
                     const u_char *packet) {
    traffic_stats_t *stats = (traffic_stats_t *)user_data;
    
    // BUG VG-001: Missing bounds check on header->caplen
    // If caplen > snaplen (shouldn't happen but can with malformed packets),
    // parse_packet reads beyond the packet buffer → buffer over-read
    // Correct: if (header->caplen < sizeof(struct ether_header)) return;
    
    packet_info_t info;
    parse_packet(packet, header->caplen, &info);  // no bounds check passed
    
    stats->packets_in++;
    stats->bytes_in += header->caplen;  // BUG: should be header->len (actual length)
    // caplen is the CAPTURED length (may be truncated by snaplen)
    // len is the ACTUAL packet length on the wire
    // Using caplen underreports bytes for large packets that were truncated
}
```

**🐛 VG-002 — INJECT THIS (High)**
In `src/storage.c`, the SQLite insert:

```c
int storage_insert_stats(storage_ctx_t *ctx, const traffic_stats_t *stats) {
    // BUG VG-002: SQL statement is not using prepared statements
    // String formatting interface name directly into SQL → SQL injection possible
    // Also: buffer is fixed 512 bytes but interface name + values could exceed it
    
    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO traffic_stats (interface, timestamp, interval_sec, "
        "packets_in, packets_out, bytes_in, bytes_out) VALUES "
        "('%s', %ld, %d, %lu, %lu, %lu, %lu)",
        stats->interface,        // BUG: not sanitized, directly formatted
        stats->interval_start,
        stats->interval_sec,
        stats->packets_in,
        stats->packets_out,
        stats->bytes_in,
        stats->bytes_out
    );
    
    char *err_msg = NULL;
    int rc = sqlite3_exec(ctx->db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);  // BUG: err_msg freed but never logged
        return -1;
    }
    return 0;
}
```

**🐛 VG-003 — INJECT THIS (Medium)**
In `src/logger.c`:

```c
#define LOG_BUFFER_SIZE 1024

void log_write(log_level_t level, const char *fmt, ...) {
    char buffer[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    
    // BUG VG-003: vsnprintf return value not checked
    // If message > 1024 bytes, it is silently truncated
    // No warning is logged about truncation
    // In high-traffic scenarios, long packet dump messages are truncated
    // and the log appears to show incomplete data
    
    vsnprintf(buffer, LOG_BUFFER_SIZE, fmt, args);  // truncation silent
    va_end(args);
    
    fprintf(log_file, "[%s] %s\n", level_str[level], buffer);
    fflush(log_file);
}
```

### Tests to Write (Phase 1)
- `test_packet.c`: parse valid Ethernet/IP/TCP packet, parse truncated packet, parse ICMP, assert field extraction correctness
- `test_storage.c`: init DB creates schema, insert stats, query stats, interface name with special chars (exposes VG-002)
- `integration/test_capture.sh`: run vigil on loopback, ping localhost, assert log file has entries

---

## Phase 2 — Connection Tracking

### Goal
vigil tracks individual TCP/UDP connections — 5-tuple (src IP, dst IP, src port, dst port, protocol), packet/byte counts per connection, connection state for TCP, and timeout-based cleanup of stale connections.

### What to Build
- `src/connection.c / conn.h` — hash table of active connections, state machine for TCP, timeout scanner
- Extend `src/storage.c` — persist connections to SQLite
- Extend `vigil-cli` — `vigil-cli connections` command
- `tests/test_connection.c`

### Key Data Structures

```c
// src/conn.h
#define CONN_TABLE_SIZE 65536  // power of 2 for bit masking

typedef enum {
    TCP_STATE_SYN_SENT,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT,
    TCP_STATE_CLOSED
} tcp_state_t;

typedef struct connection {
    uint32_t     src_ip;
    uint32_t     dst_ip;
    uint16_t     src_port;
    uint16_t     dst_port;
    uint8_t      protocol;
    uint64_t     packets;
    uint64_t     bytes;
    time_t       first_seen;
    time_t       last_seen;
    tcp_state_t  tcp_state;
    char         interface[16];
    struct connection *next;  // chaining for hash collisions
} connection_t;

typedef struct {
    connection_t *table[CONN_TABLE_SIZE];
    uint32_t      count;
    pthread_mutex_t lock;
    int           timeout_sec;    // seconds before ACTIVE → TIMEOUT
} conn_table_t;
```

### Connection Hash Function

```c
uint32_t conn_hash(uint32_t src_ip, uint32_t dst_ip,
                   uint16_t src_port, uint16_t dst_port,
                   uint8_t proto) {
    // Simple FNV-1a style hash
    uint32_t hash = 2166136261u;
    hash ^= src_ip;   hash *= 16777619u;
    hash ^= dst_ip;   hash *= 16777619u;
    hash ^= src_port; hash *= 16777619u;
    hash ^= dst_port; hash *= 16777619u;
    hash ^= proto;    hash *= 16777619u;
    return hash & (CONN_TABLE_SIZE - 1);
}
```

### Bugs

**🐛 VG-004 — INJECT THIS (Critical)**
In `src/connection.c`, the connection update function:

```c
connection_t *conn_find_or_create(conn_table_t *table,
                                   const packet_info_t *pkt) {
    uint32_t idx = conn_hash(pkt->src_ip, pkt->dst_ip,
                              pkt->src_port, pkt->dst_port,
                              pkt->protocol);
    
    // BUG VG-004: Mutex is acquired but conn is accessed AFTER unlock
    // In the find path, we unlock, then dereference conn — race condition
    // Another thread could free conn between unlock and dereference
    
    pthread_mutex_lock(&table->lock);
    connection_t *conn = table->table[idx];
    while (conn) {
        if (conn->src_ip == pkt->src_ip && conn->dst_ip == pkt->dst_ip &&
            conn->src_port == pkt->src_port && conn->dst_port == pkt->dst_port) {
            pthread_mutex_unlock(&table->lock);  // BUG: unlock too early
            // conn can be freed by timeout thread here
            conn->packets++;      // use-after-free potential
            conn->bytes += pkt->length;
            conn->last_seen = time(NULL);
            return conn;
        }
        conn = conn->next;
    }
    
    // Create new connection
    connection_t *new_conn = malloc(sizeof(connection_t));
    // BUG VG-004 continues: malloc return not checked
    memset(new_conn, 0, sizeof(connection_t));
    new_conn->src_ip   = pkt->src_ip;
    new_conn->dst_ip   = pkt->dst_ip;
    new_conn->src_port = pkt->src_port;
    new_conn->dst_port = pkt->dst_port;
    new_conn->protocol = pkt->protocol;
    new_conn->packets  = 1;
    new_conn->bytes    = pkt->length;
    new_conn->first_seen = time(NULL);
    new_conn->last_seen  = time(NULL);
    strncpy(new_conn->interface, pkt->interface, 15);
    // BUG: no null terminator guarantee if interface is exactly 15 chars
    
    new_conn->next = table->table[idx];
    table->table[idx] = new_conn;
    table->count++;
    pthread_mutex_unlock(&table->lock);
    return new_conn;
}
```

**🐛 VG-005 — INJECT THIS (High)**
In `src/connection.c`, the timeout cleanup thread:

```c
void *conn_timeout_worker(void *arg) {
    conn_table_t *table = (conn_table_t *)arg;
    
    while (1) {
        sleep(30);  // run every 30 seconds
        time_t now = time(NULL);
        
        // BUG VG-005: Iterates the entire table WITHOUT holding the lock
        // While scanning, the capture thread may be modifying the same bucket
        // This leads to corrupted next pointers → infinite loop or crash
        
        for (int i = 0; i < CONN_TABLE_SIZE; i++) {
            connection_t *conn = table->table[i];  // no lock held
            connection_t *prev = NULL;
            
            while (conn) {
                if (now - conn->last_seen > table->timeout_sec) {
                    connection_t *to_free = conn;
                    if (prev) prev->next = conn->next;
                    else table->table[i] = conn->next;
                    conn = conn->next;
                    free(to_free);  // freeing while capture thread may use it
                    table->count--;
                } else {
                    prev = conn;
                    conn = conn->next;
                }
            }
        }
    }
    return NULL;
}
```

**🐛 VG-006 — INJECT THIS (Medium)**
In `src/connection.c`, TCP state machine:

```c
void conn_update_tcp_state(connection_t *conn, const struct tcphdr *tcp) {
    uint8_t flags = tcp->th_flags;
    
    // BUG VG-006: FIN and RST handling sets state to CLOSED
    // but the connection is NOT removed from the table
    // CLOSED connections accumulate indefinitely until timeout
    // In high-connection-rate environments, table fills with CLOSED entries
    // and legitimate new connections start colliding in hash buckets
    
    if (flags & TH_SYN) {
        conn->tcp_state = TCP_STATE_SYN_SENT;
    } else if (flags & TH_SYN && flags & TH_ACK) {
        conn->tcp_state = TCP_STATE_ESTABLISHED;
    } else if (flags & TH_FIN || flags & TH_RST) {
        conn->tcp_state = TCP_STATE_CLOSED;
        // Missing: schedule for immediate removal or mark for next cleanup pass
    }
}
```

### Tests to Write (Phase 2)
- `test_connection.c`: hash function distribution, find or create, duplicate connection, timeout cleanup, TCP state transitions
- Thread safety test: simultaneous insert + lookup + timeout (exposes VG-004, VG-005)

---

## Phase 3 — Protocol Analysis

### Goal
Dissect captured packets to identify application-layer protocols. Classify traffic by protocol, track per-protocol statistics, and persist protocol distribution to SQLite.

### Supported Protocols
- **Layer 3**: IPv4, IPv6 (basic), ARP
- **Layer 4**: TCP, UDP, ICMP
- **Layer 7** (port-based heuristic): DNS (53), HTTP (80/8080), HTTPS (443), SSH (22), FTP (20/21), SMTP (25/587), NTP (123)

### Key Data Structures

```c
// src/proto.h
typedef enum {
    PROTO_UNKNOWN = 0,
    PROTO_ARP,
    PROTO_ICMP,
    PROTO_TCP,
    PROTO_UDP,
    PROTO_DNS,
    PROTO_HTTP,
    PROTO_HTTPS,
    PROTO_SSH,
    PROTO_FTP,
    PROTO_SMTP,
    PROTO_NTP,
    PROTO_COUNT
} protocol_id_t;

typedef struct {
    protocol_id_t proto;
    uint64_t      packet_count;
    uint64_t      byte_count;
    char          interface[16];
    time_t        last_seen;
} proto_stats_t;
```

### Bugs

**🐛 VG-007 — INJECT THIS (High)**
In `src/protocol.c`, DNS detection:

```c
protocol_id_t detect_application_protocol(uint16_t src_port, uint16_t dst_port,
                                            uint8_t l4_proto,
                                            const uint8_t *payload,
                                            uint32_t payload_len) {
    // BUG VG-007: Port comparison uses host byte order
    // Network packets arrive in network byte order (big-endian)
    // On little-endian systems (x86), ntohs() must be called first
    // Without it, port 53 (DNS) in network order is 0x3500 = 13568 in host order
    // DNS traffic is never classified → always shows as PROTO_UNKNOWN
    
    if (src_port == 53 || dst_port == 53) {  // BUG: should be ntohs(src_port)
        return PROTO_DNS;
    }
    if (src_port == 80 || dst_port == 80 ||
        src_port == 8080 || dst_port == 8080) {
        return PROTO_HTTP;
    }
    if (src_port == 443 || dst_port == 443) {
        return PROTO_HTTPS;
    }
    if (src_port == 22 || dst_port == 22) {
        return PROTO_SSH;
    }
    return PROTO_UNKNOWN;
}
```

**🐛 VG-008 — INJECT THIS (Medium)**
In `src/protocol.c`, IPv6 handling:

```c
int parse_ip_header(const uint8_t *packet, uint32_t len,
                    packet_info_t *info) {
    if (len < 20) return -1;
    
    uint8_t version = (packet[0] >> 4) & 0xF;
    
    if (version == 4) {
        // IPv4 parsing (correct)
        struct ip *iph = (struct ip *)packet;
        info->src_ip = ntohl(iph->ip_src.s_addr);
        info->dst_ip = ntohl(iph->ip_dst.s_addr);
        info->protocol = iph->ip_p;
        return (iph->ip_hl * 4);
    } else if (version == 6) {
        // BUG VG-008: IPv6 is declared as "handled" but src/dst IPs
        // are stored as uint32_t — truncates 128-bit IPv6 to 32 bits
        // The last 4 bytes of the IPv6 address are stored as if it's IPv4
        // IPv6 traffic statistics are completely wrong
        // Protocol field read from wrong offset for IPv6
        
        struct ip6_hdr *ip6h = (struct ip6_hdr *)packet;
        // Incorrectly copies only last 4 bytes of 16-byte IPv6 address
        memcpy(&info->src_ip, &ip6h->ip6_src.s6_addr[12], 4);
        memcpy(&info->dst_ip, &ip6h->ip6_dst.s6_addr[12], 4);
        info->protocol = ip6h->ip6_nxt;
        return 40;  // fixed IPv6 header size
    }
    
    return -1;
}
```

### Tests to Write (Phase 3)
- `test_protocol.c`: detect DNS, HTTP, HTTPS, SSH, UNKNOWN; byte order test on known port values (exposes VG-007); IPv6 parse test (exposes VG-008)

---

## Phase 4 — Traffic Threshold Alerting

### Goal
vigil monitors per-interval traffic rates and fires alerts when configured thresholds (packets/sec, bytes/sec) are exceeded. Alerts are logged, stored in SQLite, and optionally trigger an external shell script.

### What to Build
- `src/alert.c / alert.h` — threshold check, alert record creation, script execution
- Extend `src/stats.c` — compute rate per interval
- Extend `vigil-cli` — `vigil-cli alerts` command
- `tests/test_alert.c`
- `tests/integration/test_alert_script.sh`

### Key Data Structures

```c
// src/alert.h
typedef enum {
    ALERT_PPS_THRESHOLD,
    ALERT_BPS_THRESHOLD,
    ALERT_ANOMALY,
    ALERT_PORT_SCAN
} alert_type_t;

typedef struct {
    alert_type_t  type;
    char          interface[16];
    uint64_t      threshold_value;
    uint64_t      actual_value;
    char          message[512];
    time_t        timestamp;
    int           script_exit_code;
} alert_t;

typedef struct {
    uint64_t  pps_threshold;
    uint64_t  bps_threshold;
    char      alert_command[512];
    int       cooldown_sec;
    time_t    last_alert_time;
    int       enabled;
} alert_config_t;
```

### Alert Script Contract

When vigil executes the alert script, it passes these environment variables:

```bash
VIGIL_ALERT_TYPE=pps_threshold
VIGIL_ALERT_INTERFACE=eth0
VIGIL_ALERT_THRESHOLD=10000
VIGIL_ALERT_ACTUAL=15423
VIGIL_ALERT_MESSAGE="PPS threshold exceeded on eth0"
VIGIL_ALERT_TIMESTAMP=1711234567
```

### Bugs

**🐛 VG-009 — INJECT THIS (Critical)**
In `src/alert.c`, script execution:

```c
int alert_execute_script(const alert_config_t *cfg, const alert_t *alert) {
    if (!cfg->alert_command[0]) return 0;
    
    // BUG VG-009: alert->message is user-influenced (contains IP addresses
    // and interface names from packets). It is formatted directly into
    // the system() call without sanitization.
    // An attacker sending packets with crafted source IPs that encode
    // shell metacharacters could inject commands.
    // Example: if interface name from config is "eth0; rm -rf /tmp/vigil"
    // (edge case: interface name read from config, not sanitized on load)
    
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s \"%s\" \"%s\" %lu %lu",
        cfg->alert_command,
        alert->interface,       // BUG: not sanitized for shell metacharacters
        alert->message,         // BUG: contains packet-derived data
        alert->threshold_value,
        alert->actual_value
    );
    
    int ret = system(cmd);  // BUG: system() is dangerous with unsanitized input
    return WEXITSTATUS(ret);
}
```

**🐛 VG-010 — INJECT THIS (Medium)**
In `src/alert.c`, cooldown logic:

```c
int alert_should_fire(alert_config_t *cfg, alert_type_t type) {
    time_t now = time(NULL);
    
    // BUG VG-010: Cooldown uses a single global last_alert_time
    // for ALL alert types and ALL interfaces
    // If eth0 fires a PPS alert, the BPS alert on eth1 is suppressed
    // for the entire cooldown period even though they are different events
    // Correct: separate cooldown per (interface, alert_type) pair
    
    if (now - cfg->last_alert_time < cfg->cooldown_sec) {
        return 0;  // suppressed — but suppresses wrong alerts too
    }
    
    cfg->last_alert_time = now;
    return 1;
}
```

### Tests to Write (Phase 4)
- `test_alert.c`: threshold exceeded fires alert, cooldown suppresses repeat, script execution (mock), VG-010 — different interface not suppressed but bug makes it suppressed
- `integration/test_alert_script.sh`: run vigil with low threshold on loopback, generate traffic, verify script is called with correct env vars

---

## Phase 5 — Log Rotation & Reporting

### Goal
vigil manages its own log files — rotating them when they exceed the configured size, keeping a configurable number of rotated files. A reporting subsystem generates periodic traffic summaries and stores them in SQLite.

### What to Build
- Extend `src/logger.c` — size check, rotation, compressed archive option
- `src/report.c / report.h` — periodic report generation, top talkers, protocol breakdown
- Extend `vigil-cli` — `vigil-cli report` command
- `tests/test_report.c`

### Log Rotation Behavior
```
vigil.log           ← current log (actively written)
vigil.log.1         ← previous log
vigil.log.2         ← one before that
...
vigil.log.N         ← oldest kept (N = rotate_count from config)
vigil.log.N+1       ← deleted
```

### Bugs

**🐛 VG-011 — INJECT THIS (High)**
In `src/logger.c`, log rotation:

```c
int logger_rotate(logger_ctx_t *ctx) {
    fclose(ctx->log_file);
    
    // BUG VG-011: Rotation renames files in WRONG ORDER
    // Should rename from highest to lowest: N→N+1, N-1→N, ..., 1→2, current→1
    // Bug renames from lowest to highest: 1→2 first, then 2→3
    // But after renaming 1→2, the old 2 is OVERWRITTEN when we rename 2→3
    // Net effect: only 2 log files ever exist regardless of rotate_count
    
    char old_path[512], new_path[512];
    for (int i = 1; i < ctx->rotate_count; i++) {  // BUG: wrong direction
        snprintf(old_path, sizeof(old_path), "%s.%d", ctx->log_path, i);
        snprintf(new_path, sizeof(new_path), "%s.%d", ctx->log_path, i + 1);
        rename(old_path, new_path);  // overwrites i+1 with i's content
    }
    
    // Rename current → .1
    rename(ctx->log_path, old_path);  // old_path still has ".N" value here!
    // BUG: old_path was last set to "vigil.log.N" not "vigil.log.1"
    // So current log gets renamed to vigil.log.N instead of vigil.log.1
    
    ctx->log_file = fopen(ctx->log_path, "a");
    ctx->current_size = 0;
    return 0;
}
```

**🐛 VG-012 — INJECT THIS (Medium)**
In `src/report.c`, top talkers calculation:

```c
void report_compute_top_talkers(storage_ctx_t *storage,
                                 time_t period_start,
                                 time_t period_end,
                                 char *output_json,
                                 size_t output_len) {
    // BUG VG-012: Query orders by packets but the report documentation
    // says top talkers are ranked by BYTES (bandwidth usage)
    // High-packet-count but low-bandwidth connections (e.g., DNS queries)
    // appear at the top, displacing actual bandwidth consumers
    
    const char *sql =
        "SELECT src_ip, SUM(packets) as total_packets, SUM(bytes) as total_bytes "
        "FROM connections "
        "WHERE first_seen >= ? AND last_seen <= ? "
        "GROUP BY src_ip "
        "ORDER BY total_packets DESC "  // BUG: should be total_bytes DESC
        "LIMIT 10";
    
    // ... execute and format as JSON
}
```

**🐛 VG-013 — INJECT THIS (Low)**
In `src/report.c`, period boundary:

```c
void report_generate_periodic(storage_ctx_t *storage,
                               report_config_t *cfg) {
    time_t now = time(NULL);
    
    // BUG VG-013: period_end is set to now, period_start is now - interval
    // But traffic_stats records are written at interval_start timestamps
    // The last interval's stats may not yet be flushed to DB when report runs
    // (flush happens every flush_interval_sec, report may run between flushes)
    // Last interval is missing from every report
    
    time_t period_end   = now;
    time_t period_start = now - cfg->report_interval_sec;
    
    // Should be:
    // period_end = last completed flush timestamp
    // period_start = period_end - cfg->report_interval_sec
    
    report_t report = {0};
    storage_query_period(storage, period_start, period_end, &report);
    storage_insert_report(storage, &report);
}
```

### Tests to Write (Phase 5)
- `test_report.c`: top talkers sorted by bytes (VG-012 fails), period coverage (VG-013), rotation order (VG-011 — only 2 files exist)

---

## Phase 6 — Config File Management

### Goal
vigil reads a full INI-format config file on startup. CLI flags override config values. Config changes can be applied at runtime via `vigil-cli reload` without restarting the daemon. All config changes are recorded in the SQLite config_history table.

### What to Build
- `src/config.c / config.h` — INI parser, CLI flag override, validation, reload handler
- Extend all modules to read from `config_t` struct
- `tests/test_config.c`

### Key Data Structures

```c
// src/config.h
#define MAX_CONFIG_STR 512

typedef struct {
    // [capture]
    char     interface[64];
    int      snaplen;
    int      promiscuous;
    int      timeout_ms;
    
    // [storage]
    char     db_path[MAX_CONFIG_STR];
    int      max_connections;
    int      flush_interval_sec;
    
    // [logging]
    char     log_path[MAX_CONFIG_STR];
    int      log_level;
    int      max_log_size_mb;
    int      rotate_count;
    
    // [alerts]
    int      alerts_enabled;
    uint64_t threshold_pps;
    uint64_t threshold_bps;
    char     alert_command[MAX_CONFIG_STR];
    int      cooldown_sec;
    
    // [reporting]
    int      report_interval_sec;
    char     report_output_path[MAX_CONFIG_STR];
    
    // [anomaly]
    int      anomaly_enabled;
    int      baseline_window_sec;
    double   deviation_threshold;
} config_t;
```

### Bugs

**🐛 VG-014 — INJECT THIS (Critical)**
In `src/config.c`, INI file parser:

```c
int config_parse_file(const char *path, config_t *cfg) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    
    char line[256];
    char key[128], value[128];
    
    while (fgets(line, sizeof(line), f)) {
        // Skip comments and section headers
        if (line[0] == '#' || line[0] == '[' || line[0] == '\n') continue;
        
        if (sscanf(line, "%127[^=]=%127[^\n]", key, value) == 2) {
            // Trim whitespace from key
            char *k = strtrim(key);
            char *v = strtrim(value);
            
            if (strcmp(k, "interface") == 0) {
                // BUG VG-014: strncpy without explicit null termination
                // If value is exactly 64 chars, no null terminator is written
                // cfg->interface is NOT null-terminated → undefined behavior
                // in all string operations that follow
                
                strncpy(cfg->interface, v, 64);  // BUG: should be 63, then cfg->interface[63]='\0'
            }
            else if (strcmp(k, "alert_command") == 0) {
                // BUG VG-014 continued: MAX_CONFIG_STR is 512 but sscanf value buffer is 128
                // Values longer than 128 chars are silently truncated during parse
                // alert_command paths can easily exceed 128 chars
                // No error or warning is emitted
                
                strncpy(cfg->alert_command, v, MAX_CONFIG_STR - 1);
                cfg->alert_command[MAX_CONFIG_STR - 1] = '\0';
                // Looks correct, but v is already truncated to 128 by sscanf above
            }
            // ... other keys
        }
    }
    
    fclose(f);
    return 0;
}
```

**🐛 VG-015 — INJECT THIS (High)**
In `src/config.c`, reload handler:

```c
void config_reload(config_t *cfg, const char *config_path,
                   storage_ctx_t *storage) {
    config_t new_cfg;
    memset(&new_cfg, 0, sizeof(config_t));
    
    if (config_parse_file(config_path, &new_cfg) != 0) {
        log_write(LOG_ERROR, "Failed to reload config");
        return;
    }
    
    // BUG VG-015: Config is replaced non-atomically
    // Between memcpy and modules reading new values,
    // other threads see a PARTIALLY updated config
    // e.g., threshold_pps is updated but cooldown_sec is not yet
    // An alert fires with new threshold but old cooldown
    // Should use a config version counter + RCU or double-buffer swap
    
    // No lock held during copy
    memcpy(cfg, &new_cfg, sizeof(config_t));  // non-atomic, partial reads possible
    
    log_write(LOG_INFO, "Config reloaded");
}
```

**🐛 VG-016 — INJECT THIS (Medium)**
In `src/config.c`, integer parsing:

```c
uint64_t parse_uint64(const char *value, const char *key) {
    // BUG VG-016: Uses atoi() which returns int (32-bit)
    // threshold_bps can easily exceed 2^31 (2 Gbps threshold overflows)
    // For a 10 Gbps network, threshold of 5000000000 (5 Gbps) wraps to
    // a negative or small positive number → alert never fires
    // Should use strtoull()
    
    return (uint64_t)atoi(value);  // BUG: atoi max is ~2.1 billion
}
```

### Tests to Write (Phase 6)
- `test_config.c`: parse valid config, missing keys use defaults, long interface name (exposes VG-014), reload (exposes VG-015 partial update), large bps threshold (exposes VG-016)

---

## Phase 7 — Multi-Interface Support

### Goal
vigil monitors multiple network interfaces simultaneously, each in its own capture thread. Stats, connections, protocols, and alerts are tracked per-interface and can be aggregated.

### What to Build
- `src/multiface.c / mface.h` — interface list management, per-interface thread lifecycle
- Extend all modules for per-interface keying
- Extend `vigil-cli` — per-interface filtering on all commands
- `tests/test_multiface.c`

### Key Data Structures

```c
// src/mface.h
#define MAX_INTERFACES 16

typedef struct {
    char           name[64];
    pcap_t        *handle;
    pthread_t      thread;
    traffic_stats_t stats;
    int            active;
    int            thread_started;
} interface_ctx_t;

typedef struct {
    interface_ctx_t interfaces[MAX_INTERFACES];
    int             count;
    pthread_mutex_t lock;
} multiface_ctx_t;
```

### Bugs

**🐛 VG-017 — INJECT THIS (High)**
In `src/multiface.c`, interface thread startup:

```c
int multiface_start(multiface_ctx_t *mctx, config_t *cfg) {
    char *iface_list = strdup(cfg->interface);
    char *token = strtok(iface_list, ",");
    
    while (token && mctx->count < MAX_INTERFACES) {
        interface_ctx_t *ictx = &mctx->interfaces[mctx->count];
        strncpy(ictx->name, token, 63);
        ictx->name[63] = '\0';
        
        // BUG VG-017: Passes pointer to stack-local loop variable as thread arg
        // By the time the thread starts, `token` has been advanced by strtok
        // All threads receive the LAST interface name, not their own
        // Only the last interface is actually monitored — all threads capture the same one
        
        pthread_create(&ictx->thread, NULL, capture_thread, token);  // BUG: token is shared
        // Correct: pass ictx (the per-interface context) as arg
        
        mctx->count++;
        token = strtok(NULL, ",");
    }
    
    free(iface_list);
    return 0;
}
```

**🐛 VG-018 — INJECT THIS (Medium)**
In `src/multiface.c`, aggregate stats:

```c
void multiface_get_aggregate_stats(multiface_ctx_t *mctx,
                                    traffic_stats_t *agg) {
    memset(agg, 0, sizeof(traffic_stats_t));
    
    for (int i = 0; i < mctx->count; i++) {
        // BUG VG-018: Reads per-interface stats without holding any lock
        // Stats are updated by capture threads concurrently
        // Aggregate may read half-updated values: old packets_in + new bytes_in
        // Results in inconsistent aggregate statistics
        
        agg->packets_in  += mctx->interfaces[i].stats.packets_in;
        agg->packets_out += mctx->interfaces[i].stats.packets_out;
        agg->bytes_in    += mctx->interfaces[i].stats.bytes_in;
        agg->bytes_out   += mctx->interfaces[i].stats.bytes_out;
    }
    
    strncpy(agg->interface, "aggregate", 15);
}
```

### Tests to Write (Phase 7)
- `test_multiface.c`: start two interfaces, verify separate stats, aggregate stats, thread argument passing (exposes VG-017), concurrent stat update (exposes VG-018)

---

## Phase 8 — Performance Metrics & Anomaly Detection

### Goal
vigil builds a traffic baseline over a configurable window, then uses statistical deviation to detect anomalous traffic patterns. Anomalies trigger the same alert pipeline as threshold alerts.

### What to Build
- `src/anomaly.c / anomaly.h` — baseline computation, standard deviation, anomaly scoring
- Extend `src/alert.c` — ALERT_ANOMALY type
- Extend `vigil-cli` — anomaly status and history
- `tests/test_anomaly.c`

### Algorithm

```
Baseline: sliding window of N seconds of per-interval packet rates
  mean = sum(rates) / count
  stddev = sqrt(sum((rate - mean)^2) / count)

Anomaly if: current_rate > mean + (deviation_threshold * stddev)
```

### Key Data Structures

```c
// src/anomaly.h
#define BASELINE_MAX_SAMPLES 3600  // 1 hour at 1-second intervals

typedef struct {
    double   samples[BASELINE_MAX_SAMPLES];
    int      head;           // ring buffer head
    int      count;          // number of valid samples
    double   mean;
    double   stddev;
    int      window_sec;
    double   deviation_threshold;
    int      baseline_ready; // 1 once enough samples collected
    char     interface[16];
} anomaly_ctx_t;
```

### Bugs

**🐛 VG-019 — INJECT THIS (Critical)**
In `src/anomaly.c`, ring buffer management:

```c
void anomaly_add_sample(anomaly_ctx_t *ctx, double rate) {
    // BUG VG-019: Ring buffer head wraps correctly but count is
    // incremented without bound — never capped at BASELINE_MAX_SAMPLES
    // After 3600 samples, count = 3601, 3602, ...
    // Mean calculation divides by count not min(count, BASELINE_MAX_SAMPLES)
    // Mean approaches 0 as count grows → anomaly threshold approaches 0
    // → every packet triggers an anomaly alert after 1 hour of operation
    
    ctx->samples[ctx->head] = rate;
    ctx->head = (ctx->head + 1) % BASELINE_MAX_SAMPLES;
    ctx->count++;  // BUG: should be: if (ctx->count < BASELINE_MAX_SAMPLES) ctx->count++
    
    if (ctx->count >= ctx->window_sec) {
        ctx->baseline_ready = 1;
    }
}

void anomaly_recompute(anomaly_ctx_t *ctx) {
    if (ctx->count == 0) return;
    
    double sum = 0.0;
    int n = ctx->count;  // BUG: unbounded count used as divisor
    
    for (int i = 0; i < BASELINE_MAX_SAMPLES; i++) {
        sum += ctx->samples[i];
    }
    
    ctx->mean = sum / n;  // n grows without bound → mean → 0
    
    double variance = 0.0;
    for (int i = 0; i < BASELINE_MAX_SAMPLES; i++) {
        double diff = ctx->samples[i] - ctx->mean;
        variance += diff * diff;
    }
    ctx->stddev = sqrt(variance / n);
}
```

**🐛 VG-020 — INJECT THIS (High)**
In `src/anomaly.c`, baseline readiness:

```c
int anomaly_check(anomaly_ctx_t *ctx, double current_rate) {
    if (!ctx->baseline_ready) return 0;
    
    // BUG VG-020: Baseline is marked ready after window_sec TOTAL samples
    // But baseline_ready is never RESET after a config reload changes window_sec
    // If window_sec increases from 300 to 3600 after reload,
    // baseline_ready remains 1 even though only 300 samples exist
    // Anomaly detection runs on an insufficient baseline → false positives
    
    double threshold = ctx->mean + (ctx->deviation_threshold * ctx->stddev);
    
    if (current_rate > threshold) {
        return 1;  // anomaly detected
    }
    return 0;
}
```

**🐛 VG-021 — INJECT THIS (Medium)**
In `src/anomaly.c`, standard deviation edge case:

```c
double anomaly_score(anomaly_ctx_t *ctx, double current_rate) {
    if (ctx->stddev == 0.0) {
        // BUG VG-021: When traffic is perfectly uniform (stddev = 0),
        // any deviation from mean produces +Inf or NaN score
        // Division by zero returns +Inf on IEEE 754
        // +Inf > any threshold → permanent anomaly alert storm
        // Should return 0.0 (no deviation from uniform baseline) or
        // treat as "not enough variance to score"
        
        return (current_rate - ctx->mean) / ctx->stddev;  // division by zero → +Inf
    }
    return (current_rate - ctx->mean) / ctx->stddev;
}
```

### Tests to Write (Phase 8)
- `test_anomaly.c`: baseline builds correctly, ring buffer wrap, count cap (exposes VG-019), stddev=0 edge case (exposes VG-021), baseline reset on reload (exposes VG-020)

---

## Complete Bug Manifest

| ID | Phase | Severity | Discovery | Category | One-liner |
|----|-------|----------|-----------|----------|-----------|
| VG-001 | 1 | Critical | Medium | Memory | Missing bounds check on caplen; bytes counted from caplen not len |
| VG-002 | 1 | High | Medium | Security/SQL | SQL injection via unsanitized interface name in sqlite3_exec |
| VG-003 | 1 | Medium | Easy | Logging | vsnprintf truncation silent — long messages lost without warning |
| VG-004 | 2 | Critical | Hard | Concurrency | Mutex released before connection dereference → use-after-free race |
| VG-005 | 2 | Critical | Hard | Concurrency | Timeout thread iterates table without lock → corrupted list, crash |
| VG-006 | 2 | Medium | Medium | Logic | CLOSED TCP connections not removed → table fills with stale entries |
| VG-007 | 3 | High | Medium | Protocol | Port comparison without ntohs() → DNS/HTTP never detected on x86 |
| VG-008 | 3 | Medium | Hard | Protocol | IPv6 addresses truncated to 32 bits → wrong stats for IPv6 traffic |
| VG-009 | 4 | Critical | Hard | Security | system() called with packet-derived data → command injection |
| VG-010 | 4 | Medium | Easy | Logic | Single cooldown timer for all interfaces and types → alert suppression |
| VG-011 | 5 | High | Medium | Logic | Log rotation renames in wrong order → only 2 files ever exist |
| VG-012 | 5 | Medium | Easy | Reporting | Top talkers ranked by packets not bytes → wrong bandwidth leaders |
| VG-013 | 5 | Low | Easy | Reporting | Last interval missing from reports due to unflushed stats |
| VG-014 | 6 | Critical | Medium | Memory | strncpy without null termination; sscanf truncates long values |
| VG-015 | 6 | High | Hard | Concurrency | Config reloaded non-atomically → threads see partial config update |
| VG-016 | 6 | Medium | Easy | Parsing | atoi() for uint64 → threshold_bps > 2 Gbps silently wraps |
| VG-017 | 7 | High | Medium | Concurrency | Thread arg is strtok pointer → all threads capture last interface |
| VG-018 | 7 | Medium | Medium | Concurrency | Aggregate stats read without lock → inconsistent values |
| VG-019 | 8 | Critical | Hard | Logic | Ring buffer count unbounded → mean approaches 0 → alert storm |
| VG-020 | 8 | High | Medium | Logic | Baseline ready flag not reset on config reload → false positives |
| VG-021 | 8 | Medium | Medium | Math | stddev=0 causes division by zero → +Inf anomaly score |

### Bug Distribution

| Severity | Count |
|----------|-------|
| Critical | 6 |
| High | 7 |
| Medium | 7 |
| Low | 1 |

| Discovery | Count |
|-----------|-------|
| Hard | 5 |
| Medium | 11 |
| Easy | 5 |

| Category | Count |
|----------|-------|
| Concurrency | 6 |
| Memory/Buffer | 3 |
| Security | 2 |
| Protocol | 2 |
| Logic | 4 |
| Reporting | 2 |
| Math | 1 |
| Parsing | 1 |

---

## Data Flow

```
Network Interface (libpcap/Npcap)
  └── capture thread (per interface)
        └── packet_callback()
              ├── parse_packet() → packet_info_t
              ├── conn_find_or_create() → connection_t (hash table)
              ├── detect_protocol() → protocol_id_t
              └── stats_update() → traffic_stats_t
                    │
                    ├── Every flush_interval_sec:
                    │     └── storage_insert_stats() → SQLite: traffic_stats
                    │     └── storage_insert_connections() → SQLite: connections
                    │     └── storage_insert_proto_stats() → SQLite: protocol_stats
                    │
                    ├── Every interval:
                    │     └── alert_check_thresholds()
                    │           ├── If exceeded → alert_fire()
                    │           │     ├── storage_insert_alert() → SQLite: alerts
                    │           │     └── alert_execute_script() → external script
                    │           └── anomaly_check() (Phase 8)
                    │                 └── If anomaly → alert_fire()
                    │
                    └── Every report_interval_sec:
                          └── report_generate_periodic() → SQLite: reports

vigil-cli (separate process)
  └── connects via Unix socket to daemon
        ├── vigil-cli stats → reads SQLite: traffic_stats
        ├── vigil-cli connections → reads SQLite: connections
        ├── vigil-cli alerts → reads SQLite: alerts
        ├── vigil-cli report → reads SQLite: reports
        └── vigil-cli reload → sends SIGHUP to daemon
```

---

## vigil-cli to Daemon Communication

vigil creates a Unix domain socket at `/tmp/vigil.sock` (or configurable path). `vigil-cli` connects to this socket and sends simple JSON commands:

```json
// Request
{"command": "stats", "interface": "eth0", "limit": 10}

// Response
{"status": "ok", "data": { ... }}

// Error
{"status": "error", "message": "Interface not found"}
```

---

## Feature Roadmap Summary

| Phase | Feature | Status |
|-------|---------|--------|
| 1 | Packet capture, counting, SQLite storage, logging | Planned |
| 2 | Connection tracking with TCP state machine | Planned |
| 3 | Protocol analysis (TCP/UDP/ICMP/DNS/HTTP) | Planned |
| 4 | Threshold alerting + external script execution | Planned |
| 5 | Log rotation + periodic traffic reports | Planned |
| 6 | INI config file + runtime reload | Planned |
| 7 | Multi-interface simultaneous capture | Planned |
| 8 | Baseline + statistical anomaly detection | Planned |

---

## Getting Started (Developer Quickstart)

```bash
# Clone
git clone https://github.com/WireStack/vigil.git
cd vigil

# Install dependencies (Ubuntu)
sudo apt install libpcap-dev libsqlite3-dev check gcc make

# Build
make

# Configure
cp vigil.conf.example vigil.conf
# Edit vigil.conf — set your interface name

# Run (requires root for packet capture)
sudo ./vigil --config vigil.conf

# In another terminal
./vigil-cli status
./vigil-cli stats
./vigil-cli connections --top 20

# Run tests
make test
```

---

## Notes for Developers

This codebase contains **21 intentional defects** (`VG-001` through `VG-021`) across 8 phases for **developer training**: code review, reproduction, implementation from this spec, and fixes with tests. Each defect is marked in source:

```c
// BUG VG-XXX: description
```

Topics include memory safety, concurrency, protocol parsing, security, logic errors, and numerical edge cases.

- **Onboarding and workflows:** [vigil/DEVELOPERS.md](vigil/DEVELOPERS.md)
- **Defect index:** [vigil/PRODUCT.md](vigil/PRODUCT.md) (appendix)
- **Build and run:** [vigil/README.md](vigil/README.md)

Keep intentional defects on the shared **training** branch. Submit real fixes on a separate branch with unit or integration tests. Lab sequencing and guided reproduction are handled by your training bot—not duplicated here.
