# vigil

WireStack network monitoring daemon (C99, libpcap, SQLite). This repository is a **developer training** codebase for systems-style work: daemons, capture, persistence, IPC, and incremental fixes.

**Developers:** start at [DEVELOPERS.md](DEVELOPERS.md).

## Quickstart

```bash
sudo apt install libpcap-dev libsqlite3-dev check gcc make
cd vigil
make
cp vigil.conf.example vigil.conf
# Edit interface name in vigil.conf
sudo ./vigil --config vigil.conf
./vigil-cli status
make test
```

On **WSL**, open an Ubuntu terminal and use:

```bash
cd "/mnt/e/projects X/wirestack/vigil"
```

Use two terminals: one for `sudo ./vigil`, one for `./vigil-cli`.

## Platform

- **Linux (primary)**: gcc, libpcap, SQLite3, Check framework
- **Windows**: Install Npcap, SQLite, Check; use `mingw32-make` and override `LIBS` in Makefile for `-lwpcap -lws2_32`

## Developer training

This codebase includes **21 intentional engineering defects** marked `// BUG VG-XXX` for review and fix exercises. See [DEVELOPERS.md](DEVELOPERS.md) for workflows and [PRODUCT.md](PRODUCT.md) for the defect catalog.

Keep intentional defects on the shared **training** branch; submit real fixes on a separate branch with tests.

## Integration tests

Require root or `CAP_NET_RAW` for packet capture:

```bash
chmod +x tests/integration/*.sh
sudo tests/integration/test_capture.sh
```
