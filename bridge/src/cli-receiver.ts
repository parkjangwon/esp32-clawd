import { EventEmitter } from "events";

export const VALID_STATES = [
  "idle",
  "thinking",
  "typing",
  "building",
  "error",
  "happy",
  "sleeping",
  "subagent",
  "notification",
  "sweeping",
  "carrying",
  "subagent_multi",
] as const;

export type ClawdState = (typeof VALID_STATES)[number];

export function isValidState(value: string): value is ClawdState {
  return (VALID_STATES as readonly string[]).includes(value);
}

export interface CliReceiverEvents {
  state: (state: ClawdState) => void;
}

export declare interface CliReceiver {
  on<Event extends keyof CliReceiverEvents>(
    event: Event,
    listener: CliReceiverEvents[Event]
  ): this;
  emit<Event extends keyof CliReceiverEvents>(
    event: Event,
    ...args: Parameters<CliReceiverEvents[Event]>
  ): boolean;
}

export class CliReceiver extends EventEmitter {
  /**
   * Register a handler for incoming state changes.
   */
  onState(handler: (state: ClawdState) => void): void {
    this.on("state", handler);
  }
}
