// GestureEvent/BaseEvent timestamp is ns since boot (API26 common.d.ts).
// Explicit units prevent magnitude guessing and mixed wall/monotonic clocks.
export class P011GestureRateClock {
  private active: boolean = false;
  private previousMs: number = -1;
  begin(): void { this.active = true; this.previousMs = -1; }
  stop(): void { this.active = false; this.previousMs = -1; }
  suspend(): void { this.previousMs = -1; }
  sample(timestamp: number, unit: 'ns' | 'ms'): number {
    if (!this.active) return 0;
    const now = unit === 'ns' ? timestamp / 1000000 : timestamp;
    if (!Number.isFinite(now) || now < 0) { this.suspend(); return 0; }
    const previous = this.previousMs;
    if (previous >= 0 && now <= previous) return 0;
    this.previousMs = now;
    if (previous < 0) return 0;
    return Math.min(1, (now - previous) / 1000);
  }
}
