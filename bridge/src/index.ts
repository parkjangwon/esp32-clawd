#!/usr/bin/env node

import { createServer, Socket } from "net";
import { unlinkSync, existsSync } from "fs";
import { Command } from "commander";
import { CliReceiver, isValidState, ClawdState } from "./cli-receiver.js";
import { StateCache } from "./state-cache.js";

const SOCKET_PATH = "/tmp/clawd.sock";

function startServer() {
  // Clean up stale socket file
  if (existsSync(SOCKET_PATH)) {
    unlinkSync(SOCKET_PATH);
  }

  const receiver = new CliReceiver();
  const cache = new StateCache();

  receiver.onState((state: ClawdState) => {
    console.log(`[state] ${state}`);
  });

  const server = createServer((socket: Socket) => {
    let buffer = "";

    socket.on("data", (chunk: Buffer) => {
      buffer += chunk.toString();

      const lines = buffer.split("\n");
      // Keep the last incomplete line in the buffer
      buffer = lines.pop() ?? "";

      for (const line of lines) {
        const trimmed = line.trim();
        if (!trimmed) continue;

        let rawState: string;

        try {
          const parsed = JSON.parse(trimmed);
          if (parsed && typeof parsed.state === "string") {
            rawState = parsed.state;
          } else {
            // JSON but missing "state" field — try as raw string
            continue;
          }
        } catch {
          // Not JSON — treat as raw state string
          rawState = trimmed;
        }

        const state = rawState.toLowerCase().trim();

        if (!isValidState(state)) {
          console.error(`[error] invalid state: "${rawState}"`);
          continue;
        }

        if (cache.isDuplicate(state)) {
          continue; // Debounced — same state within 200ms
        }

        cache.setState(state);
        receiver.emit("state", state);
      }
    });

    socket.on("error", (err: Error) => {
      console.error(`[error] socket error: ${err.message}`);
    });
  });

  server.listen(SOCKET_PATH, () => {
    console.log(`[bridge] listening on ${SOCKET_PATH}`);
  });

  server.on("error", (err: Error) => {
    console.error(`[error] server error: ${err.message}`);
    process.exit(1);
  });

  // Graceful shutdown
  const shutdown = () => {
    console.log("[bridge] shutting down...");
    server.close(() => {
      if (existsSync(SOCKET_PATH)) {
        unlinkSync(SOCKET_PATH);
      }
      process.exit(0);
    });
  };

  process.on("SIGINT", shutdown);
  process.on("SIGTERM", shutdown);
}

function sendState(state: string) {
  const socket = new Socket();
  socket.connect(SOCKET_PATH, () => {
    const payload = JSON.stringify({ state });
    socket.write(payload + "\n");
    socket.end();
  });

  socket.on("error", (err: Error) => {
    console.error(`[error] cannot connect to bridge: ${err.message}`);
    process.exit(1);
  });
}

const program = new Command();

program
  .name("clawd")
  .description("ESP32 Clawd Bridge — relays agent state to hardware")
  .version("0.1.0");

program
  .command("state")
  .description("Send a state change to the bridge")
  .argument("<state>", "one of: idle, thinking, typing, building, error, happy, sleeping, subagent, notification, sweeping, carrying, subagent_multi")
  .action((state: string) => {
    const normalized = state.toLowerCase().trim();
    if (!isValidState(normalized)) {
      console.error(`[error] invalid state: "${state}"`);
      console.error(`[error] valid states: idle, thinking, typing, building, error, happy, sleeping, subagent, notification, sweeping, carrying, subagent_multi`);
      process.exit(1);
    }
    sendState(normalized);
  });

// If no arguments, start the server; otherwise run CLI
if (process.argv.length <= 2) {
  startServer();
} else {
  program.parse(process.argv);
}
