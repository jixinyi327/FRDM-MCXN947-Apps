#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "$APP_DIR/../.." && pwd)"
BUILD_DIR="${OSC_BUILD_DIR:-$REPO_ROOT/mcuxpresso-sdk/build/oscilloscope}"
PRIMARY_ELF="$BUILD_DIR/oscilloscope/oscilloscope_cm33_core0.elf"
SECONDARY_ELF="$BUILD_DIR/oscilloscope_secondary/oscilloscope_secondary_cm33_core1.elf"

source "$REPO_ROOT/development-plan/scripts/env.sh" >/dev/null

for elf in "$PRIMARY_ELF" "$SECONDARY_ELF"; do
  if [[ ! -f "$elf" ]]; then
    printf 'ELF not found: %s\n' "$elf" >&2
    exit 1
  fi
done

shared_start="0x$(arm-none-eabi-nm "$PRIMARY_ELF" |
  awk '$3 == "__RPMSG_SH_MEM_START__" {value = $1} END {print value}')"
core1_image_start="0x$(arm-none-eabi-nm "$PRIMARY_ELF" |
  awk '$3 == "core1_image_start" {value = $1} END {print value}')"
core1_image_end="0x$(arm-none-eabi-nm "$PRIMARY_ELF" |
  awk '$3 == "core1_image_end" {value = $1} END {print value}')"
core1_vectors="0x$(arm-none-eabi-nm "$SECONDARY_ELF" |
  awk '$3 == "__Vectors" {value = $1} END {print value}')"

expected_shared_start="0x2004c000"
expected_vectors="0x2004e000"
expected_core1_image_start="0x000c0000"

[[ "$shared_start" == "$expected_shared_start" ]] || {
  printf 'Unexpected shared-memory start: %s\n' "$shared_start" >&2
  exit 1
}
[[ "$core1_vectors" == "$expected_vectors" ]] || {
  printf 'Unexpected core1 vector address: %s\n' "$core1_vectors" >&2
  exit 1
}
[[ "$core1_image_start" == "$expected_core1_image_start" ]] || {
  printf 'Unexpected embedded core1 image address: %s\n' "$core1_image_start" >&2
  exit 1
}
core1_image_size=$((core1_image_end - core1_image_start))
(( core1_image_size <= 0x00040000 )) || {
  printf 'Core1 image exceeds the reserved 256 KiB flash region.\n' >&2
  exit 1
}

printf 'PASS  shared-state base: %s\n' "$shared_start"
printf 'PASS  core1 boot vector: %s\n' "$core1_vectors"
printf 'PASS  core1 image: %s, size %d bytes\n' \
  "$core1_image_start" "$core1_image_size"
