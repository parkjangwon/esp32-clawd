#!/usr/bin/env bash
set -euo pipefail

STATES=(idle thinking typing building error happy sleeping subagent notification sweeping carrying subagent_multi)

echo "=== ESP32 Clawd State Test ==="
echo "Sending all 12 states with 2-second intervals..."
echo ""

for state in "${STATES[@]}"; do
    echo "[test] sending state: $state"
    echo "{\"state\":\"$state\"}" | nc -U /tmp/clawd.sock 2>/dev/null || echo "  (bridge not running, skipping)"
    sleep 2
done

echo ""
echo "=== Test complete ==="
