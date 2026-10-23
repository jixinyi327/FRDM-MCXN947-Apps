#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/env.sh"

cd "$MCXN_SDK"
west build -b frdmmcxn947 \
  mcuxsdk/examples/demo_apps/hello_world \
  -d build/hello_world_core0 -p always \
  --config debug -Dcore_id=cm33_core0

find build/hello_world_core0 -maxdepth 1 \
  \( -name 'hello_world*.elf' -o -name 'hello_world*.bin' \) \
  -printf '%p %s bytes\n'

