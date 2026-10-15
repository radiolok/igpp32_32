#!/bin/sh
# Verify that the scan interrupt lives in IRAM and calls nothing in flash.
# Runs inside the ESP-IDF environment after `idf.py build`:
#   tools/check_scan_iram.sh fw-esp32/build/igpp.elf
set -e
ELF=$1
NM=xtensa-esp32s3-elf-nm
OBJDUMP=xtensa-esp32s3-elf-objdump

line=$($NM -S "$ELF" | grep -E ' [tT] .*on_column' | head -n1)
[ -n "$line" ] || { echo "scan ISR symbol not found"; exit 1; }
addr=$(echo "$line" | cut -d' ' -f1)
size=$(echo "$line" | cut -d' ' -f2)
start=$((0x$addr))
end=$((0x$addr + 0x$size))
echo "on_column at 0x$addr, $((0x$size)) bytes"

# ESP32-S3 internal SRAM (IRAM) instruction bus: 0x40370000..0x403DFFFF
if [ $start -lt $((0x40370000)) ] || [ $start -ge $((0x403E0000)) ]; then
    echo "::error::scan ISR is not in IRAM (0x$addr)"
    exit 1
fi

calls=$($OBJDUMP -d --start-address=$start --stop-address=$end "$ELF" | grep -E '\scall(x)?[0-9]+\s' || true)
echo "$calls"
bad=$(echo "$calls" | grep -oE '\b42[0-9a-f]{6}\b' || true)
if [ -n "$bad" ]; then
    echo "::error::scan ISR calls code in flash: $bad"
    exit 1
fi
echo "::notice::scan ISR in IRAM at 0x$addr ($((0x$size)) bytes), no flash calls"
