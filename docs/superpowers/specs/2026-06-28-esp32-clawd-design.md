# ESP32 Clawd — Hardware State Renderer

## Overview

ESP32-S3 + 240x240 LCD를 Clawd on Desk의 물리적 캐릭터 렌더러로 확장한다.
ESP32는 AI를 실행하지 않고, PC의 State Engine으로부터 상태를 받아 애니메이션만 재생한다.

## Architecture

```
Claude Code Hook
      │  clawd state <name> (CLI)
      ▼
clawd-bridge (Node.js daemon)
      │  WebSocket JSON
      ▼
ESP32-S3 (Waveshare 1.54")
```

## Components

### 1. clawd-bridge (Node.js CLI Daemon)

- Claude Code Hook에서 `clawd state thinking` CLI 호출을 수신 (Unix socket 또는 stdin)
- ESP32와의 WebSocket 연결 관리
- mDNS로 `clawd.local` 자동 발견, `--host` fallback
- 연결 끊김 시 큐잉 + 자동 재연결
- State debounce (같은 상태 연속 전송 방지)

### 2. ESP32 Firmware (C / ESP-IDF v5.5)

- WiFi 연결 + mDNS advertiser (`clawd.local`)
- WebSocket client → Bridge에 지속 연결
- SPIFFS에 저장된 GIF 프레임을 순차 렌더링
- 12개 상태 각각에 대응하는 애니메이션
- Touch 입력 처리 (쓰다듬기 → idle 리액션)
- Speaker 효과음 (선택적)

### 3. Hook Integration

Claude Code hook 설정에 clawd CLI 호출을 추가:
- `PreToolUse` → `clawd state typing`
- `PostToolUse` → `clawd state idle`
- `PreThink` → `clawd state thinking`
- `PostThink` → `clawd state idle`
- `Error` → `clawd state error`
- `Stop` → `clawd state sleeping`
- `Idle 60s` → `clawd state sleeping`

## Communication Protocol

Bridge → ESP32 WebSocket JSON:

```json
{"state": "idle"}
{"state": "thinking"}
{"state": "typing"}
{"state": "building"}
{"state": "error"}
{"state": "happy"}
{"state": "sleeping"}
{"state": "subagent"}
{"state": "notification"}
{"state": "sweeping"}
{"state": "carrying"}
{"state": "subagent_multi"}
```

## State → Animation Mapping

ESP32는 수신한 state 문자열을 SPIFFS의 GIF 파일에 매핑:

| State | File |
|-------|------|
| idle | /spiffs/idle.gif |
| thinking | /spiffs/thinking.gif |
| typing | /spiffs/typing.gif |
| building | /spiffs/building.gif |
| error | /spiffs/error.gif |
| happy | /spiffs/happy.gif |
| sleeping | /spiffs/sleeping.gif |
| subagent | /spiffs/subagent.gif |
| notification | /spiffs/notification.gif |
| sweeping | /spiffs/sweeping.gif |
| carrying | /spiffs/carrying.gif |
| subagent_multi | /spiffs/subagent_multi.gif |

## Hardware Target

- Board: Waveshare ESP32-S3-Touch-LCD-1.54
- Display: 240x240 TFT LCD (ST7789)
- Touch: CST816S I2C capacitive
- WiFi: ESP32-S3 built-in
- Speaker: PWM output

## Project Structure

```
esp32-clawd/
├── firmware/           # ESP-IDF project
│   ├── main/
│   │   ├── main.cc
│   │   ├── wifi.cc / wifi.h
│   │   ├── websocket.cc / websocket.h
│   │   ├── renderer.cc / renderer.h
│   │   ├── touch.cc / touch.h
│   │   ├── speaker.cc / speaker.h
│   │   └── state_machine.cc / state_machine.h
│   ├── assets/         # GIF files → flashed to SPIFFS
│   └── CMakeLists.txt
├── bridge/             # Node.js project
│   ├── src/
│   │   ├── index.ts        # Entry: CLI + WebSocket server
│   │   ├── ws-server.ts    # WebSocket server for ESP32
│   │   ├── mdns.ts         # mDNS discovery
│   │   ├── cli-receiver.ts # Unix socket / stdin receiver
│   │   └── state-cache.ts  # Debounce + last-state
│   ├── package.json
│   └── tsconfig.json
├── hooks/              # Claude Code hook config
│   └── claude-hooks.json
└── docs/
    └── superpowers/specs/
```

## Key Decisions

- **새 펌웨어**: xiaozhi 기반이 아닌 순수 Clawd 전용 펌웨어. 코드베이스 단순, 디버깅 용이.
- **GIF 애셋 재사용**: Clawd on Desk 테마에서 GIF 추출 → 240x240 리사이즈 → SPIFFS 저장.
- **Node.js Bridge**: clawd-on-desk와 동일 생태계, Hook 코드 재활용.
- **mDNS + CLI fallback**: 기본 자동 발견, 장애 시 `--host` 수동 지정.
- **ESP-IDF v5.5**: Waveshare 1.54" 보드와의 호환성 및 ST7789 드라이버 지원.
