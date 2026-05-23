# vigil

WireStack network monitoring daemon (C99, libpcap, SQLite).

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

## Platform

- **Linux (primary)**: gcc, libpcap, SQLite3, Check framework
- **Windows**: Install Npcap, SQLite, Check; use `mingw32-make` and override `LIBS` in Makefile for `-lwpcap -lws2_32`

## QA training

This codebase contains **21 intentional bugs** marked `// BUG VG-XXX`. See [PRODUCT.md](PRODUCT.md) for the manifest. Do not fix them in the training branch.

## Integration tests

Require root or `CAP_NET_RAW` for packet capture:

```bash
chmod +x tests/integration/*.sh
sudo tests/integration/test_capture.sh
```
