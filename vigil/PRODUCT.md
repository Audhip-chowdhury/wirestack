# vigil — Product Overview

**Company**: WireStack  
**Type**: Network monitoring daemon  
**Language**: C99  
**Platform**: Linux (primary), Windows via Npcap (secondary)

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

## Appendix: Intentional Bug Manifest (QA Training)

| ID | Phase | Severity | File | One-liner |
|----|-------|----------|------|-----------|
| VG-001 | 1 | Critical | src/capture.c | Missing caplen bounds check; bytes counted from caplen not len |
| VG-002 | 1 | High | src/storage.c | SQL injection via formatted interface name in sqlite3_exec |
| VG-003 | 1 | Medium | src/logger.c | vsnprintf truncation silent |
| VG-004 | 2 | Critical | src/connection.c | Mutex released before connection dereference (UAF race) |
| VG-005 | 2 | Critical | src/connection.c | Timeout thread iterates table without lock |
| VG-006 | 2 | Medium | src/connection.c | CLOSED TCP connections not removed from table |
| VG-007 | 3 | High | src/protocol.c | Port comparison without ntohs() on x86 |
| VG-008 | 3 | Medium | src/packet.c | IPv6 addresses truncated to 32 bits |
| VG-009 | 4 | Critical | src/alert.c | system() with unsanitized interface/message |
| VG-010 | 4 | Medium | src/alert.c | Single global cooldown for all alert types/interfaces |
| VG-011 | 5 | High | src/logger.c | Log rotation renames in wrong order |
| VG-012 | 5 | Medium | src/report.c | Top talkers ORDER BY packets not bytes |
| VG-013 | 5 | Low | src/report.c | Report period may miss last unflushed interval |
| VG-014 | 6 | Critical | src/config.c | strncpy interface without null terminator; sscanf truncates values |
| VG-015 | 6 | High | src/config.c | Non-atomic memcpy on config reload |
| VG-016 | 6 | Medium | src/config.c | atoi() for uint64 threshold_bps |
| VG-017 | 7 | High | src/multiface.c | pthread_create passes strtok token pointer |
| VG-018 | 7 | Medium | src/multiface.c | Aggregate stats read without lock |
| VG-019 | 8 | Critical | src/anomaly.c | Ring buffer count unbounded → mean → 0 |
| VG-020 | 8 | High | src/anomaly.c | baseline_ready not reset on reload |
| VG-021 | 8 | Medium | src/anomaly.c | stddev=0 division by zero in anomaly_score |

### Severity distribution

| Severity | Count |
|----------|-------|
| Critical | 6 |
| High | 7 |
| Medium | 7 |
| Low | 1 |

### Reproduction hints

- **VG-001**: Send malformed/truncated frames; compare byte counters vs wire length.
- **VG-002**: Insert stats with interface name containing SQL metacharacters.
- **VG-007**: Capture DNS on lo; protocol stats stay UNKNOWN for network-order ports.
- **VG-009**: Craft alert message/interface with shell metacharacters in config path.
- **VG-017**: Set `interfaces = eth0,eth1` — both threads capture the last token.
- **VG-019**: Run daemon >1 hour with anomaly enabled; observe alert storm.

All bugs are marked in source with `// BUG VG-XXX` or `/* BUG VG-XXX */` comments.
