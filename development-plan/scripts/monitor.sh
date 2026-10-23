#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/env.sh"

mapfile -t PORTS < <(find /dev -maxdepth 1 \( -name 'ttyACM*' -o -name 'ttyUSB*' \) -printf '%p\n' 2>/dev/null | sort)

if (( ${#PORTS[@]} == 0 )); then
  echo "No /dev/ttyACM* or /dev/ttyUSB* found. Attach the MCU-Link USB device to WSL first." >&2
  exit 1
fi

if (( ${#PORTS[@]} == 1 )); then
  PORT="${PORTS[0]}"
else
  echo "Multiple serial ports found:"
  printf '  %s\n' "${PORTS[@]}"
  read -r -p "Enter port path: " PORT
fi

exec "$MCXN_ROOT/.venv/bin/python" -m serial.tools.miniterm "$PORT" 115200

