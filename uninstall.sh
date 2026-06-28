#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"

echo "============================================"
echo " ESP32 Clawd — Uninstall"
echo "============================================"

# Stop bridge
echo "[1/3] Stopping bridge..."
pkill -f "node.*dist/index.js" 2>/dev/null && echo "  Bridge stopped." || echo "  Bridge was not running."

# Remove Claude Code hooks from project settings
if [ -f "$ROOT/.claude/settings.json" ]; then
    echo "[2/3] Removing Claude Code hooks..."
    # Remove the clawd hook entries from settings.json
    cd "$ROOT"
    python3 -c "
import json, sys
try:
    with open('.claude/settings.json') as f:
        s = json.load(f)
    if 'hooks' in s:
        del s['hooks']
    with open('.claude/settings.json', 'w') as f:
        json.dump(s, f, indent=2)
    print('  Hooks removed from .claude/settings.json')
except Exception as e:
    print(f'  Note: {e}')
"
fi

# Unlink global clawd command
echo "[3/3] Removing clawd CLI..."
npm unlink -g clawd-bridge 2>/dev/null && echo "  clawd CLI removed." || echo "  clawd CLI was not linked."

echo ""
echo "============================================"
echo " Uninstall complete."
echo " ESP32 firmware is still on the device."
echo " Reflash to erase: idf.py -p /dev/cu.usbmodem* flash"
echo "============================================"
