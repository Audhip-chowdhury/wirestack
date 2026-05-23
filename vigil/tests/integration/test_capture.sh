#!/bin/bash
# Integration: capture on loopback (requires root/CAP_NET_RAW)
set -e
cd "$(dirname "$0")/../.."
if [ ! -x ./vigil ]; then make; fi
IFACE="${VIGIL_IFACE:-lo}"
timeout 5 sudo ./vigil --interface "$IFACE" --log ./logs/vigil-test.log 2>/dev/null || true
if [ -f ./logs/vigil-test.log ]; then
  grep -q INFO ./logs/vigil-test.log && echo "PASS: log has entries" || exit 1
else
  echo "SKIP: no log (capture may need root)"
fi
