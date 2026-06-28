const DEBOUNCE_MS = 200;

export interface StateRecord {
  state: string;
  timestamp: number;
}

export class StateCache {
  private last: StateRecord | null = null;

  /**
   * Returns true if the given state is a duplicate of the last recorded
   * state within the debounce window (200ms).
   */
  isDuplicate(state: string): boolean {
    if (!this.last) return false;
    if (this.last.state !== state) return false;
    return Date.now() - this.last.timestamp < DEBOUNCE_MS;
  }

  /**
   * Records a new state with the current timestamp.
   */
  setState(state: string): void {
    this.last = { state, timestamp: Date.now() };
  }

  /**
   * Returns the last recorded state and timestamp, or null if none.
   */
  getLast(): StateRecord | null {
    return this.last;
  }
}
