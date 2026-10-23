#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
RULE="$REPO_ROOT/tools/linkserver/lib/udev/rules.d/85-linkserver_26.9.130.rules"

case "${1:-}" in
  --check|--dry-run)
    if [[ -f "$RULE" ]]; then
      echo "udev rule found: $RULE"
    else
      echo "udev rule not found: $RULE" >&2
      exit 1
    fi
    if command -v udevadm >/dev/null; then
      echo "udevadm available: $(command -v udevadm)"
    else
      echo "udevadm is not installed" >&2
      exit 1
    fi
    echo "Dry run only; no system files were modified."
    exit 0
    ;;
  -h|--help)
    echo "Usage: install_udev_rules.sh [--check|--dry-run]"
    exit 0
    ;;
  "")
    ;;
  *)
    echo "Unknown option: $1" >&2
    exit 2
    ;;
esac

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

