#!/usr/bin/env bash

# Source this file:
#   source /home/jxy/NXP/MCXN947/开发计划/scripts/env.sh

export MCXN_ROOT="${MCXN_ROOT:-/home/jxy/NXP/MCXN947}"
export MCXN_SDK="${MCXN_SDK:-$MCXN_ROOT/mcuxpresso-sdk}"
export ARMGCC_DIR="${ARMGCC_DIR:-$MCXN_ROOT/toolchains/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi}"
export MCXN_LINKSERVER_DIR="${MCXN_LINKSERVER_DIR:-$MCXN_ROOT/tools/linkserver/usr/local/LinkServer_26.9.130}"
export PATH="$ARMGCC_DIR/bin:$MCXN_ROOT/.venv/bin:$MCXN_LINKSERVER_DIR:$PATH"

command -v west >/dev/null || echo "west not found after setting PATH" >&2
command -v arm-none-eabi-gcc >/dev/null || echo "arm-none-eabi-gcc not found after setting PATH" >&2
command -v LinkServer >/dev/null || echo "LinkServer not found after setting PATH" >&2

echo "MCXN_ROOT=$MCXN_ROOT"
echo "MCXN_SDK=$MCXN_SDK"
echo "ARMGCC_DIR=$ARMGCC_DIR"
echo "MCXN_LINKSERVER_DIR=$MCXN_LINKSERVER_DIR"

