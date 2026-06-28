#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT/bridge"

# Kill existing bridge
pkill -f "node.*dist/index.js" 2>/dev/null || true
sleep 1

npm run build --silent 2>&1 | tail -1
node dist/index.js > /tmp/clawd-bridge.log 2>&1 &

sleep 2
echo "Clawd Bridge started."
echo "  Log: tail -f /tmp/clawd-bridge.log"
echo "  Try: clawd state thinking"
