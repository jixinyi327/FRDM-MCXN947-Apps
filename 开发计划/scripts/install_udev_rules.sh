#!/usr/bin/env bash
set -euo pipefail

RULE="$HOME/NXP/MCXN947/tools/linkserver/lib/udev/rules.d/85-linkserver_26.9.130.rules"

if [[ ! -f "$RULE" ]]; then
  echo "udev rule not found: $RULE" >&2
  exit 1
fi

sudo install -m 0644 "$RULE" /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
echo "Installed $(basename "$RULE")"

