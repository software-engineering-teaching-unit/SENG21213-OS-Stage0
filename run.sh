#!/bin/bash
# =============================================================================
# SENG 21213 — Stage 0 QEMU launch script
#
# Usage:
#   ./run.sh          — build (if needed) and run normally
#   ./run.sh debug    — run in debug mode (QEMU pauses, GDB on port 1234)
#   ./run.sh clean    — clean and rebuild, then run
# =============================================================================

set -e

DISK_IMG="seng21213.img"

# Build if image doesn't exist or sources are newer
if [ ! -f "$DISK_IMG" ] || [ "$1" = "clean" ]; then
    echo "Building..."
    make clean 2>/dev/null || true
    make
fi

if [ "$1" = "debug" ]; then
    echo "Starting QEMU in debug mode — connect GDB with:"
    echo "  gdb -ex 'file build/kernel.elf' -ex 'target remote localhost:1234' -ex 'break kernel_main' -ex 'continue'"
    qemu-system-i386 \
        -drive format=raw,file="$DISK_IMG",if=ide \
        -m 32M \
        -no-reboot \
        -serial stdio \
        -s -S
else
    echo "Starting QEMU..."
    qemu-system-i386 \
        -drive format=raw,file="$DISK_IMG",if=ide \
        -m 32M \
        -no-reboot \
        -serial stdio
fi
