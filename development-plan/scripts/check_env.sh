#!/usr/bin/env bash
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/env.sh"

printf '\n== Tool versions ==\n'
arm-none-eabi-gcc --version | head -1
west --version
cmake --version | head -1
ninja --version
node --version
npm --version
LinkServer --version

printf '\n== SDK board ==\n'
cd "$MCXN_SDK"
west boards -b frdmmcxn947

printf '\n== Built hello_world ==\n'
find "$MCXN_SDK/build/hello_world_core0" -maxdepth 1 \
  \( -name 'hello_world*.elf' -o -name 'hello_world*.bin' \) \
  -printf '%p %s bytes\n' 2>/dev/null || true

printf '\n== Built touchscreen_tester ==\n'
find "$MCXN_SDK/build/touchscreen_tester_core0" -maxdepth 1 \
  \( -name 'touchscreen_tester*.elf' -o -name 'touchscreen_tester*.bin' \) \
  -printf '%p %s bytes\n' 2>/dev/null || true

printf '\n== LinkServer target support ==\n'
LinkServer devices 2>/dev/null | grep -m1 'FRDM-MCXN947' || true

printf '\n== USB ==\n'
if command -v lsusb >/dev/null; then
  lsusb | grep -i -E 'nxp|mcu-link|cmsis|daplink' || echo 'No NXP/MCU-Link USB device visible in WSL.'
else
  echo 'lsusb is not installed.'
fi

printf '\n== Serial ports ==\n'
find /dev -maxdepth 1 \( -name 'ttyACM*' -o -name 'ttyUSB*' \) -printf '%p\n' 2>/dev/null || true

