#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# Generate and flash SPIFFS asset image with animation frames.
#
# Prerequisites:
#   - ESP-IDF environment sourced (export.sh)
#   - ESP32-S3 connected via USB
#
# Usage:
#   bash scripts/flash_assets.sh
#   bash scripts/flash_assets.sh /dev/tty.usbmodemXXXX  # explicit port
# ---------------------------------------------------------------------------
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
ASSETS_DIR="${PROJECT_DIR}/assets"
BUILD_DIR="${PROJECT_DIR}/build"
SPIFFS_IMAGE="${BUILD_DIR}/spiffs.bin"

# Partition info from partitions.csv
PARTITION_OFFSET=0x320000
PARTITION_SIZE=0xC00000  # 12 MB

# Source IDF environment if not already set
if [ -z "${IDF_PATH:-}" ]; then
    if [ -f "${HOME}/esp/esp-idf/export.sh" ]; then
        echo "Sourcing IDF environment..."
        source "${HOME}/esp/esp-idf/export.sh" > /dev/null 2>&1
    else
        echo "Error: IDF_PATH not set and export.sh not found."
        exit 1
    fi
fi

# Determine serial port
PORT="${1:-}"
if [ -z "${PORT}" ]; then
    # Try to auto-detect ESP32-S3 serial port
    if command -v ls /dev/tty.usbmodem* > /dev/null 2>&1; then
        PORT=$(ls /dev/tty.usbmodem* 2>/dev/null | head -1)
    fi
    if [ -z "${PORT}" ]; then
        PORT=$(ls /dev/tty.wchusbserial* /dev/tty.SLAB_USBtoUART* /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -1)
    fi
    if [ -z "${PORT}" ]; then
        echo "Error: cannot detect serial port. Specify: $0 /dev/tty...."
        exit 1
    fi
    echo "Auto-detected port: ${PORT}"
fi

# --- Step 1: Generate frame assets ---
echo "=== Generating frame assets ==="
mkdir -p "${ASSETS_DIR}"
python3 "${SCRIPT_DIR}/gen_frames.py" "${ASSETS_DIR}"
echo ""

# --- Step 2: Create SPIFFS image ---
echo "=== Creating SPIFFS image ==="
mkdir -p "${BUILD_DIR}"
SPIFFS_GEN="${IDF_PATH}/components/spiffs/spiffsgen.py"
if [ ! -f "${SPIFFS_GEN}" ]; then
    echo "Error: spiffsgen.py not found at ${SPIFFS_GEN}"
    exit 1
fi
python3 "${SPIFFS_GEN}" "${PARTITION_SIZE}" "${ASSETS_DIR}" "${SPIFFS_IMAGE}"
echo "SPIFFS image created: ${SPIFFS_IMAGE}"
echo ""

# --- Step 3: Flash SPIFFS partition ---
echo "=== Flashing SPIFFS partition ==="
ESPTOOL_PY="${IDF_PATH}/components/esptool_py/esptool/esptool.py"
if [ ! -f "${ESPTOOL_PY}" ]; then
    echo "Error: esptool.py not found at ${ESPTOOL_PY}"
    exit 1
fi
python3 "${ESPTOOL_PY}" --chip esp32s3 --port "${PORT}" --baud 921600 \
    write_flash --flash_size detect "${PARTITION_OFFSET}" "${SPIFFS_IMAGE}"
echo ""

echo "=== Done ==="
echo "Assets flashed at offset ${PARTITION_OFFSET} on ${PORT}"
