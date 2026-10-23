#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
RULE="$REPO_ROOT/tools/linkserver/lib/udev/rules.d/85-linkserver_26.9.130.rules"
if (( EUID == 0 )); then
  SUDO=()
else
  SUDO=(sudo)
fi

if [[ ! -f "$RULE" ]]; then
  echo "udev rule not found: $RULE" >&2
  exit 1
fi

"${SUDO[@]}" install -m 0644 "$RULE" /etc/udev/rules.d/
"${SUDO[@]}" udevadm control --reload-rules
"${SUDO[@]}" udevadm trigger
echo "Installed $(basename "$RULE")"

