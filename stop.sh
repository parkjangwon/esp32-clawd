#!/usr/bin/env bash
set -euo pipefail

echo "Stopping Clawd Bridge..."

if pkill -f "node.*dist/index.js" 2>/dev/null; then
    echo "Bridge stopped."
else
    echo "Bridge was not running."
fi
