// P-011 peak protocol v1 §§1-5. Independent diagnostic; no release authority.
import { P011EndpointClock, P011EndpointCommitKey } from './P011EndpointMetrics';
export const P011_PEAK_FIELD_REVISION = 'p011-peak-fields-v1';
export interface P011PeakInputEvent {
  eventId: string; runId: string; peakInputCommitOrder: number; effectiveElapsedMs: number;
  endpointClockId: string; value: number; requestedValue: number;
  kind: 'SHAKE_ON' | 'SHAKE_OFF' | 'SHAKE_UPDATE';
  shakeOnEventId: string | null; caseOccurrenceId: string;
}
export interface P011PeakConsumption {
  eventId: string | null; shakeOnEventId: string | null; value: number;
}
export interface P011PeakMasses { DH: number; DL: number; UH: number; UL: number; PH: number; PL: number; EH: number; EL: number; }
export interface P011PeakSummary {
  peakFieldRevision: string; peakMethodId: string; peakProtocolId: string; peakProtocolRevision: string;
  peakAuthority: string; peakProtocolEffective: boolean; evidenceClassification: string;
  peakWindowStartElapsedMs: number | null; peakWindowEndElapsedMs: number | null;
  shakeOnEventId: string | null; shakeOffEventId: string | null;
  shakeOnEffectiveElapsedMs: number | null; shakeOffEffectiveElapsedMs: number | null;
  peakWindowSampleCount: number; peakNonDepositedFraction: number | null;
  peakNonDepositedHeavyFraction: number | null; peakNonDepositedLightFraction: number | null;
  peakCommitKey: P011EndpointCommitKey | null; peakStockMass: number | null;
  peakReferenceMass: number | null; peakSampleElapsedMs: number | null;
  peakInputCommitOrder: number; peakStatus: string; peakScore: number | null;
  legacyPeakHistoricalReference: number | null; legacyPeakLiveFormula: number | null;
  sameReferenceSuspOnlyPeak: number | null;
}
export class P011PeakSampler {
  readonly runId: string; readonly clock: P011EndpointClock;
  private events: P011PeakInputEvent[] = [];
  private order = 0; private value = 0; private lastElapsed = -1;
  private current: P011PeakInputEvent | null = null;
  private on: P011PeakInputEvent | null = null; private off: P011PeakInputEvent | null = null;
  private activeRoot: string | null = null;
  private count = 0; private peak = -1; private peakH = -1; private peakL = -1;
  private stock = 0; private reference = 0; private elapsed = 0;
  private key: P011EndpointCommitKey | null = null;
  private historical = -1; private live = -1; private sameSusp = -1;
  private eligible = true; private lastCommitOrder = 0; private peakOrder = 0;
  constructor(runId: string, clock: P011EndpointClock) { this.runId = runId; this.clock = clock; }
  private nextOrder(): number {
    if (!Number.isSafeInteger(this.order + 1)) throw new Error('PEAK_ORDER_EXHAUSTED');
    return ++this.order;
  }
  accept(value: number, requested: number, occurrenceId: string): void {
    const elapsed = this.clock.nowElapsedMs();
    if (!Number.isFinite(elapsed) || elapsed < 0 || elapsed < this.lastElapsed) throw new Error('PEAK_CLOCK_INVALID');
    this.lastElapsed = elapsed;
    const order = this.nextOrder(); const id = this.runId + ':shake:' + order;
    let kind: 'SHAKE_ON' | 'SHAKE_OFF' | 'SHAKE_UPDATE' = 'SHAKE_UPDATE';
    if (value === 1 && this.value !== 1) {
      kind = 'SHAKE_ON'; this.activeRoot = id; this.off = null;
      this.count = 0; this.peakOrder = 0; this.peak = -1; this.peakH = -1; this.peakL = -1;
      this.historical = -1; this.live = -1; this.sameSusp = -1; this.key = null; this.eligible = true;
    } else if (value === 0 && this.value !== 0) kind = 'SHAKE_OFF';
    const event: P011PeakInputEvent = { eventId: id, runId: this.runId, peakInputCommitOrder: order,
      effectiveElapsedMs: elapsed, endpointClockId: this.clock.clockId, value: value,
      requestedValue: requested, kind: kind, shakeOnEventId: this.activeRoot, caseOccurrenceId: occurrenceId };
    this.events.push(event); this.current = event;
    if (kind === 'SHAKE_ON') this.on = event;
    if (kind === 'SHAKE_OFF') { this.off = event; this.activeRoot = null; }
    if (this.on !== null && elapsed < this.on.effectiveElapsedMs + 3000 && value !== 1) this.eligible = false;
    this.value = value;
  }
  invalidate(): void { this.eligible = false; }
  consume(): P011PeakConsumption {
    return { eventId: this.current === null ? null : this.current.eventId,
      shakeOnEventId: this.activeRoot, value: this.value };
  }
  commit(key: P011EndpointCommitKey, elapsed: number, input: P011PeakConsumption, mass: P011PeakMasses): number {
    const order = this.nextOrder(); this.lastCommitOrder = order;
    if (!Number.isFinite(elapsed) || elapsed < this.lastElapsed) throw new Error('PEAK_CLOCK_INVALID');
    this.lastElapsed = elapsed;
    const on = this.on;
    if (on === null || elapsed < on.effectiveElapsedMs || elapsed >= on.effectiveElapsedMs + 3000 ||
      input.shakeOnEventId !== on.eventId || input.value !== 1) return order;
    const e = mass.EH + mass.EL, u = mass.UH + mass.UL, p = mass.PH + mass.PL;
    const d = mass.DH + mass.DL, m = u + p;
    if (![mass.DH,mass.DL,mass.UH,mass.UL,mass.PH,mass.PL,mass.EH,mass.EL].every(
      (v: number): boolean => Number.isFinite(v) && v >= 0) || e <= 0) { this.eligible = false; return order; }
    this.count++;
    if (d + u > 0) this.historical = Math.max(this.historical, u / (d + u));
    this.live = Math.max(this.live, u / Math.max(d + u + p, 1e-9));
    this.sameSusp = Math.max(this.sameSusp, u / e);
    if (mass.EH > 0) this.peakH = Math.max(this.peakH, (mass.UH + mass.PH) / mass.EH);
    if (mass.EL > 0) this.peakL = Math.max(this.peakL, (mass.UL + mass.PL) / mass.EL);
    if (m / e > this.peak) {
      this.peakOrder = order; this.peak = m / e; this.stock = m; this.reference = e; this.elapsed = elapsed;
      this.key = { runId: key.runId, sequence: key.sequence, frameId: key.frameId, substepId: key.substepId,
        attemptId: key.attemptId, committedOrdinal: key.committedOrdinal };
    }
    return order;
  }
  inputEvents(): P011PeakInputEvent[] { return this.events.map((e: P011PeakInputEvent): P011PeakInputEvent =>
    ({ eventId: e.eventId, runId: e.runId, peakInputCommitOrder: e.peakInputCommitOrder,
      effectiveElapsedMs: e.effectiveElapsedMs, endpointClockId: e.endpointClockId, value: e.value,
      requestedValue: e.requestedValue, kind: e.kind, shakeOnEventId: e.shakeOnEventId, caseOccurrenceId: e.caseOccurrenceId })); }
  summary(): P011PeakSummary {
    const on = this.on, off = this.off;
    const complete = on !== null && off !== null && off.effectiveElapsedMs >= on.effectiveElapsedMs + 3000 &&
      this.lastElapsed >= on.effectiveElapsedMs + 3000 && this.eligible && this.count > 0;
    const score = this.peak < 0 ? null : this.peak >= 0.9 ? 10 : this.peak >= 0.75 ? 8 :
      this.peak >= 0.5 ? 6 : this.peak >= 0.35 ? 4 : this.peak >= 0.2 ? 2 : 0;
    return { peakFieldRevision: P011_PEAK_FIELD_REVISION, peakMethodId: 'peakNonDepositedFraction',
      peakProtocolId: 'p011-ws08-peak-nondeposited', peakProtocolRevision: 'ws08-peak-nondeposited-v1',
      peakAuthority: 'APPROVED', peakProtocolEffective: false, evidenceClassification: 'CANDIDATE_DIAGNOSTIC',
      peakWindowStartElapsedMs: on === null ? null : on.effectiveElapsedMs,
      peakWindowEndElapsedMs: on === null ? null : on.effectiveElapsedMs + 3000,
      shakeOnEventId: on === null ? null : on.eventId, shakeOffEventId: off === null ? null : off.eventId,
      shakeOnEffectiveElapsedMs: on === null ? null : on.effectiveElapsedMs,
      shakeOffEffectiveElapsedMs: off === null ? null : off.effectiveElapsedMs,
      peakWindowSampleCount: this.count, peakNonDepositedFraction: this.peak < 0 ? null : this.peak,
      peakNonDepositedHeavyFraction: this.peakH < 0 ? null : this.peakH,
      peakNonDepositedLightFraction: this.peakL < 0 ? null : this.peakL,
      peakCommitKey: this.key, peakStockMass: this.key === null ? null : this.stock,
      peakReferenceMass: this.key === null ? null : this.reference,
      peakSampleElapsedMs: this.key === null ? null : this.elapsed, peakInputCommitOrder: this.peakOrder,
      peakStatus: complete ? this.stock >= 0.5 * this.reference ? 'PASS' : 'FAIL' : 'INCOMPLETE', peakScore: score,
      legacyPeakHistoricalReference: this.historical < 0 ? null : this.historical,
      legacyPeakLiveFormula: this.live < 0 ? null : this.live, sameReferenceSuspOnlyPeak: this.sameSusp < 0 ? null : this.sameSusp };
  }
}
