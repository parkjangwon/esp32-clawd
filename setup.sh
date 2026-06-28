#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"

echo "============================================"
echo " ESP32 Clawd — Setup"
echo "============================================"
echo ""

# ── 1. Get WiFi info ──
echo "[1/4] WiFi settings"
echo "  This is needed so the ESP32 can connect to your network."
read -p "  WiFi SSID: " WIFI_SSID
read -sp "  WiFi Password: " WIFI_PASS
echo ""
echo ""

# ── 2. Flash firmware ──
echo "[2/4] Building & flashing firmware..."
cd "$ROOT/firmware"

# Inject WiFi credentials into the firmware
cat > main/wifi_config.h << EOF
#pragma once
#define WIFI_SSID "$WIFI_SSID"
#define WIFI_PASS "$WIFI_PASS"
EOF

# Source ESP-IDF
if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
    source "$HOME/esp/esp-idf/export.sh" > /dev/null 2>&1
else
    echo "Error: ESP-IDF not found. Install it first."
    exit 1
fi

# Build and flash
idf.py build flash 2>&1 | tail -3
echo ""

# ── 3. Flash assets ──
echo "[3/4] Flashing Clawd animation assets..."
python3 scripts/convert_clawd.py 2>&1 | tail -1
python3 "$IDF_PATH/components/spiffs/spiffsgen.py" 0xC00000 assets build/spiffs.bin
PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -1 || echo "/dev/cu.usbmodem101")
python3 "$IDF_PATH/components/esptool_py/esptool/esptool.py" --chip esp32s3 --port "$PORT" --baud 921600 write_flash --flash_size detect 0x320000 build/spiffs.bin 2>&1 | tail -3
echo ""

# ── 4. Build & start bridge ──
echo "[4/4] Building & starting bridge..."
cd "$ROOT/bridge"
npm install --silent 2>&1 | tail -1
npm run build 2>&1 | tail -1
npm link > /dev/null 2>&1

# Kill existing bridge if any
pkill -f "node.*dist/index.js" 2>/dev/null || true
sleep 1

# Start bridge in background
node dist/index.js > /tmp/clawd-bridge.log 2>&1 &
sleep 3

echo ""
echo "============================================"
echo " Setup complete!"
echo "============================================"
echo ""
echo "  ESP32 is now connecting to: $WIFI_SSID"
echo "  Bridge is running in background"
echo ""
echo "  Try: clawd state thinking"
echo "       clawd state happy"
echo ""
echo "  Change WiFi anytime:"
echo "       clawd wifi --ssid 'NewWiFi' --pass 'newpass'"
echo ""
echo "  Bridge log: tail -f /tmp/clawd-bridge.log"
