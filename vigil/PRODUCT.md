# vigil — Product Overview

**Company**: WireStack  
**Type**: Network monitoring daemon  
**Language**: C99  
**Platform**: Linux (primary), Windows via Npcap (secondary)

**Developers:** start at [DEVELOPERS.md](DEVELOPERS.md).

vigil captures live packets, tracks connections, analyzes protocols, fires threshold and anomaly alerts, persists history in SQLite, and exposes `vigil-cli` over a Unix domain socket at `/tmp/vigil.sock`.

## Features (Phases 1–8)

| Phase | Capability |
|-------|------------|
| 1 | Packet capture, counting, SQLite stats, logging |
| 2 | Connection tracking (5-tuple hash, TCP states, timeout thread) |
| 3 | Protocol classification (L3/L4 + port heuristics) |
| 4 | PPS/BPS threshold alerts + external script |
| 5 | Log rotation + periodic reports |
| 6 | INI config + runtime reload |
| 7 | Multi-interface capture |
| 8 | Baseline anomaly detection |

## CLI

`vigil-cli status | stats | connections | protocols | alerts | report | reload | stop`

## Configuration

See `vigil.conf.example` for all sections: `[capture]`, `[storage]`, `[logging]`, `[alerts]`, `[reporting]`, `[interfaces]`, `[anomaly]`.

---

## Appendix: Engineering defect catalog (VG-001–VG-021)

Intentional defects on the training branch for developer review and fix exercises. Each is marked in source with `// BUG VG-XXX` or `/* BUG VG-XXX */`.

For guided reproduction and lab sequencing, use your **training bot** or facilitator assignments.

| ID | Phase | File | Summary |
|----|-------|------|---------|
| VG-001 | 1 | src/capture.c | Missing caplen bounds check; bytes counted from caplen not len |
| VG-002 | 1 | src/storage.c | SQL injection via formatted interface name in sqlite3_exec |
| VG-003 | 1 | src/logger.c | vsnprintf truncation silent |
| VG-004 | 2 | src/connection.c | Mutex released before connection dereference (UAF race) |
| VG-005 | 2 | src/connection.c | Timeout thread iterates table without lock |
| VG-006 | 2 | src/connection.c | CLOSED TCP connections not removed from table |
| VG-007 | 3 | src/protocol.c | Port comparison without ntohs() on x86 |
| VG-008 | 3 | src/packet.c | IPv6 addresses truncated to 32 bits |
| VG-009 | 4 | src/alert.c | system() with unsanitized interface/message |
| VG-010 | 4 | src/alert.c | Single global cooldown for all alert types/interfaces |
| VG-011 | 5 | src/logger.c | Log rotation renames in wrong order |
| VG-012 | 5 | src/report.c | Top talkers ORDER BY packets not bytes |
| VG-013 | 5 | src/report.c | Report period may miss last unflushed interval |
| VG-014 | 6 | src/config.c | strncpy interface without null terminator; sscanf truncates values |
| VG-015 | 6 | src/config.c | Non-atomic memcpy on config reload |
| VG-016 | 6 | src/config.c | atoi() for uint64 threshold_bps |
| VG-017 | 7 | src/multiface.c | pthread_create passes strtok token pointer |
| VG-018 | 7 | src/multiface.c | Aggregate stats read without lock |
| VG-019 | 8 | src/anomaly.c | Ring buffer count unbounded → mean → 0 |
| VG-020 | 8 | src/anomaly.c | baseline_ready not reset on reload |
| VG-021 | 8 | src/anomaly.c | stddev=0 division by zero in anomaly_score |

Do not merge fixes for these defects into the shared training branch unless your program explicitly ends the exercise.
