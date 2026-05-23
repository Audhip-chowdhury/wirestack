#!/bin/bash
set -e
cd "$(dirname "$0")/../.."
if [ ! -x ./vigil-cli ]; then make; fi
if [ ! -S /tmp/vigil.sock ]; then
  echo "SKIP: daemon not running"
  exit 0
fi
./vigil-cli status | grep -q status && echo "PASS: cli status" || exit 1
