# ESP32 Clawd

A physical hardware companion for Claude Code (and other AI coding agents). The ESP32-S3 with a 240x240 LCD sits on your desk and displays the Clawd mascot, reacting in real-time to your AI agent's state.

```
Claude Code → Hook → Bridge → WiFi → ESP32 → LCD + Speaker
```

## Hardware

- **ESP32-S3** (Waveshare Touch LCD 1.54")
- 240×240 TFT LCD (ST7789)
- WiFi, Touch, Speaker, Battery

## How It Works

The ESP32 doesn't run AI — it's a pure **state renderer**. Your coding agent sends state changes via a lightweight bridge, and the ESP32 plays the corresponding animation.

```
You code → Claude Code processes → Hook fires → Bridge relays → ESP32 animates
```

**12 animated states:**

| State | Animation | Trigger |
|-------|-----------|---------|
| `idle` | Clawd resting, eye tracking | Default, session start |
| `thinking` | Thinking/processing | Claude Code processes prompt |
| `typing` | Keyboard typing | Tool execution (read, write, bash) |
| `building` | Construction/work | Build/compile activity |
| `error` | Error/shock | Tool failure, API error |
| `happy` | Celebration/jumping | Task completion (Stop hook) |
| `sleeping` | Sleeping/dozing | 60s idle timeout |
| `subagent` | Headphones groove | 1 subagent launched |
| `subagent_multi` | Juggling | 2+ subagents launched |
| `notification` | Alert/attention | Notifications, permission requests |
| `sweeping` | Cleaning/sweeping | Context compaction |
| `carrying` | Carrying objects | File operations, worktree create |

## Quick Start

```bash
git clone https://github.com/parkjangwon/esp32-clawd.git
cd esp32-clawd
./setup.sh
```

The setup script will:
1. Ask for your WiFi credentials
2. Auto-detect your Mac's IP address
3. Build and flash the firmware
4. Flash Clawd animation assets
5. Start the bridge

**Done!** The ESP32 will connect to your WiFi and the bridge. Start using Claude Code — Clawd will react automatically.

## Daily Use

```bash
./start.sh          # Start the bridge (ESP32 connects automatically)
clawd state happy   # Manually trigger any state
clawd wifi --ssid "MyWiFi" --pass "password"  # Change WiFi
```

## Commands

| Command | Description |
|---------|-------------|
| `./setup.sh` | First-time setup (flash + WiFi config + bridge) |
| `./start.sh` | Start the bridge |
| `./stop.sh` | Stop the bridge |
| `./uninstall.sh` | Stop bridge + remove hooks |
| `clawd state <name>` | Send a state to ESP32 |
| `clawd wifi --ssid --pass` | Change WiFi credentials |

## Button Controls

| Button | Short Press | Long Press (1s) | Very Long (3s) |
|--------|-------------|-----------------|----------------|
| Center (Power) | Display on/off | Restart | Power off |
| Right (Vol+) | Brightness + | — | — |
| Left (Vol-) | Brightness - | — | — |

## Status Bar

The top bar shows:
- **Left:** Current time (KST, 24h)
- **Right:** WiFi signal bars + Battery percentage

## Architecture

```
esp32-clawd/
├── firmware/        # ESP-IDF project (C++)
│   ├── main/        # WiFi, display, WebSocket, animations
│   ├── assets/      # Clawd animation frames (240x240 RGB565)
│   └── scripts/     # Asset conversion + flash tools
├── bridge/          # Node.js bridge (TypeScript)
│   └── src/         # Unix socket server, WebSocket, mDNS
├── setup.sh         # One-shot setup script
├── start.sh         # Bridge launcher
└── uninstall.sh     # Cleanup
```

## Requirements

- **ESP32:** Waveshare ESP32-S3-Touch-LCD-1.54 (or compatible)
- **Mac:** ESP-IDF v5.5+, Node.js 20+
- **Network:** ESP32 and Mac on the same WiFi

## License

MIT — see [LICENSE](LICENSE)

Clawd character artwork © respective rights holders (clawd-on-desk project).
