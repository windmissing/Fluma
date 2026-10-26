// P-011 D1-R3 S3 / hot contract §3.1. Numeric R9 sidecar and fixed outs.
// Per-end actual debit is refunded in full; total once, bottom always zero.
import { P011ParticleHotWorkspace, P011ParticleHotPlanBuffer, allocateParticleHotLease,
  validateParticleHotRequest, sameRequestHot, copyRequestRowHot } from './P011ParticleHot';

export class P011ParticleHotTicketReceipt {
  ownerId = 0; stepEpoch = 0; ticketToken = 0; requestOrdinal = 0;
  requested = 0; localAllowed = 0; reserved = 0; source = -1; target = -1;
  copyFromHot(a: P011ParticleHotTicketReceipt): void {
    this.ownerId = a.ownerId; this.stepEpoch = a.stepEpoch; this.ticketToken = a.ticketToken; this.requestOrdinal = a.requestOrdinal;
    this.requested = a.requested; this.localAllowed = a.localAllowed; this.reserved = a.reserved; this.source = a.source; this.target = a.target;
  }
}
export class P011ParticleHotRefundReceipt {
  ownerId = 0; stepEpoch = 0; ticketToken = 0; requestOrdinal = 0;
  requested = 0; amount = 0; totalReserved = 0; totalRefunded = 0;
  localCount = 0; netTotalUsed = 0; netTotalDelta = 0; netDeltaValid = 0;
  bottomReserved = 0; bottomRefunded = 0; status = 0;
  readonly localColumns = new Int32Array(2);
  readonly localReserved = new Float64Array(2); readonly localRefunded = new Float64Array(2);
  readonly netLocalUsed = new Float64Array(2); readonly netLocalDelta = new Float64Array(2);
  private readonly storage: ArrayBuffer[];
  constructor() {
    this.storage = [this.localColumns.buffer, this.localReserved.buffer, this.localRefunded.buffer,
      this.netLocalUsed.buffer, this.netLocalDelta.buffer];
  }
  storageValidHot(): boolean {
    return this.localColumns.buffer === this.storage[0] && this.localColumns.length === 2 &&
      this.localReserved.buffer === this.storage[1] && this.localReserved.length === 2 &&
      this.localRefunded.buffer === this.storage[2] && this.localRefunded.length === 2 &&
      this.netLocalUsed.buffer === this.storage[3] && this.netLocalUsed.length === 2 &&
      this.netLocalDelta.buffer === this.storage[4] && this.netLocalDelta.length === 2;
  }
  coldBuffers(): ArrayBuffer[] { return this.storage; }
  copyFromHot(a: P011ParticleHotRefundReceipt): void {
    this.ownerId = a.ownerId; this.stepEpoch = a.stepEpoch; this.ticketToken = a.ticketToken; this.requestOrdinal = a.requestOrdinal;
    this.requested = a.requested; this.amount = a.amount; this.totalReserved = a.totalReserved; this.totalRefunded = a.totalRefunded;
    this.localCount = a.localCount; this.netTotalUsed = a.netTotalUsed; this.netTotalDelta = a.netTotalDelta;
    this.netDeltaValid = a.netDeltaValid; this.bottomReserved = a.bottomReserved; this.bottomRefunded = a.bottomRefunded; this.status = a.status;
    this.localColumns.set(a.localColumns); this.localReserved.set(a.localReserved); this.localRefunded.set(a.localRefunded);
    this.netLocalUsed.set(a.netLocalUsed); this.netLocalDelta.set(a.netLocalDelta);
  }
}
export interface P011ParticleHotBudgetSink {
  reserveInto(expectedStepEpoch: number, plan: P011ParticleHotPlanBuffer, row: number,
    receipt: P011ParticleHotTicketReceipt): number;
  refundInto(ticketToken: number, amount: number, receipt: P011ParticleHotRefundReceipt): number;
  closeInto(ticketToken: number, committed: number, receipt: P011ParticleHotRefundReceipt): number;
}
export class P011ParticleHotBudgetAdapter implements P011ParticleHotBudgetSink {
  private readonly storage: ArrayBuffer[];
  readonly work: P011ParticleHotWorkspace;
  readonly localRemaining: Float64Array; readonly localUsed: Float64Array;
  readonly beforeRemaining: Float64Array; readonly beforeUsed: Float64Array;
  totalRemaining = 0; totalUsed = 0; readonly bottomUsed = 0;
  private beforeTotalRemaining = 0; private beforeTotalUsed = 0;
  private epoch = 0; private ticketCount = 0; private openCount = 0;
  get diagnosticEpoch(): number {return this.epoch;}
  get diagnosticOpenCount(): number {return this.openCount;}
  readonly tokens: Float64Array; readonly reserved: Float64Array; readonly refunded: Float64Array;
  readonly closed: Uint8Array; readonly requests: P011ParticleHotPlanBuffer;
  denominator = 0;
  scale: number;
  constructor(work: P011ParticleHotWorkspace, scale: number = 1) {
    this.work = work;
    this.scale = scale;
    if (!Number.isFinite(scale) || scale < 0 || scale > 1) throw new Error('PARTICLE_INVALID_BUDGET');
    this.localRemaining = new Float64Array(work.cols); this.localUsed = new Float64Array(work.cols);
    this.beforeRemaining = new Float64Array(work.cols); this.beforeUsed = new Float64Array(work.cols);
    this.tokens = new Float64Array(work.requestCapacity); this.reserved = new Float64Array(work.requestCapacity);
    this.refunded = new Float64Array(work.requestCapacity); this.closed = new Uint8Array(work.requestCapacity);
    this.requests = new P011ParticleHotPlanBuffer(work.requestCapacity, 1, 2 * work.requestCapacity);
    this.storage = [this.localRemaining.buffer, this.localUsed.buffer, this.beforeRemaining.buffer, this.beforeUsed.buffer,
      this.tokens.buffer, this.reserved.buffer, this.refunded.buffer, this.closed.buffer];
  }
  coldBuffers(): ArrayBuffer[] { return this.storage.concat(this.requests.coldBuffers()); }
  storageValidHot(): boolean {
    return this.localRemaining.buffer === this.storage[0] && this.localUsed.buffer === this.storage[1] &&
      this.beforeRemaining.buffer === this.storage[2] && this.beforeUsed.buffer === this.storage[3] &&
      this.tokens.buffer === this.storage[4] && this.reserved.buffer === this.storage[5] &&
      this.refunded.buffer === this.storage[6] && this.closed.buffer === this.storage[7] && this.requests.storageValidHot();
  }
  beginHot(epoch: number, dt: number): number {
    if (this.epoch !== 0 || !Number.isSafeInteger(epoch) || epoch <= 0 || !Number.isFinite(dt) || dt <= 0) return 4;
    if (!Number.isFinite(300 * dt)) return 2;
    this.localRemaining.fill(28 * dt); this.localUsed.fill(0); this.totalRemaining = 300 * dt; this.totalUsed = 0;
    this.beforeRemaining.set(this.localRemaining); this.beforeUsed.set(this.localUsed);
    this.beforeTotalRemaining = this.totalRemaining; this.beforeTotalUsed = this.totalUsed;
    this.tokens.fill(0); this.reserved.fill(0); this.refunded.fill(0); this.closed.fill(0);
    this.ticketCount = 0; this.openCount = 0; this.denominator = 0; this.epoch = epoch; return 0;
  }
  rollbackHot(epoch: number): number {
    if (this.epoch !== epoch) return 4;
    this.localRemaining.set(this.beforeRemaining); this.localUsed.set(this.beforeUsed);
    this.totalRemaining = this.beforeTotalRemaining; this.totalUsed = this.beforeTotalUsed;
    this.tokens.fill(0); this.reserved.fill(0); this.refunded.fill(0); this.closed.fill(0);
    this.ticketCount = 0; this.openCount = 0; this.denominator = 0; this.epoch = 0; return 0;
  }
  allClosedHot(): boolean { return this.openCount === 0; }
  auditHot(): number {
    if (!Number.isFinite(this.totalUsed) || !Number.isFinite(this.totalRemaining) || this.totalUsed < -1e-9 || this.totalRemaining < -1e-9 ||
      Math.abs(this.totalUsed + this.totalRemaining - this.beforeTotalUsed - this.beforeTotalRemaining) > 1e-9) return 13;
    for (let c = 0; c < this.work.cols; c++) if (!Number.isFinite(this.localUsed[c]) || !Number.isFinite(this.localRemaining[c]) ||
      this.localUsed[c] < -1e-9 || this.localRemaining[c] < -1e-9 ||
      Math.abs(this.localUsed[c] + this.localRemaining[c] - this.beforeUsed[c] - this.beforeRemaining[c]) > 1e-9) return 13;
    return 0;
  }
  finishHot(epoch: number): number {
    if (this.epoch !== epoch) return 4;
    if (this.openCount !== 0) return 5;
    this.epoch = 0; return 0;
  }
  reserveInto(expectedStepEpoch: number, plan: P011ParticleHotPlanBuffer, row: number,
    receipt: P011ParticleHotTicketReceipt): number {
    if (this.epoch !== expectedStepEpoch || this.epoch !== this.work.stepEpoch || this.work.active !== 1) return 4;
    if (!this.storageValidHot()) return 3;
    const valid = validateParticleHotRequest(this.work, plan, row); if (valid !== 0) return valid;
    if (!(receipt instanceof P011ParticleHotTicketReceipt)) return 3;
    const kind = plan.kind[row]; const c = plan.column[row]; const target = kind === 3 ? plan.targetColumn[row] : -1;
    if ((kind !== 2 && kind !== 3 && kind !== 4) || target < -1 || target >= this.work.cols || target === c ||
      !Number.isFinite(this.scale) || this.scale < 0 || this.scale > 1) return 2;
    for (let i = 0; i < this.ticketCount; i++) if (this.requests.ordinal[i] === plan.ordinal[row]) {
      return sameRequestHot(plan, row, this.requests, i) ? 15 : 16;
    }
    if (this.ticketCount >= this.tokens.length) return 11;
    const requested = plan.requested[row];
    const local = Math.min(requested, this.localRemaining[c], target >= 0 ? this.localRemaining[target] : Infinity);
    const amount = Math.min(local, local * this.scale, this.totalRemaining);
    if (!Number.isFinite(local) || local < 0 || !Number.isFinite(amount) || amount < 0 ||
      !Number.isFinite(this.localUsed[c] + amount) || !Number.isFinite(this.totalUsed + amount) ||
      (target >= 0 && !Number.isFinite(this.localUsed[target] + amount))) return 13;
    const token = allocateParticleHotLease(); if (token === 0) { this.work.tokenExhausted = 1; return 14; }
    const i = this.ticketCount++; this.tokens[i] = token; this.reserved[i] = amount; this.refunded[i] = 0;
    this.closed[i] = 0; this.openCount++; copyRequestRowHot(plan, row, this.requests, i);
    this.localRemaining[c] -= amount; this.localUsed[c] += amount;
    if (target >= 0) { this.localRemaining[target] -= amount; this.localUsed[target] += amount; }
    this.totalRemaining -= amount; this.totalUsed += amount;
    receipt.ownerId = this.work.ownerId; receipt.stepEpoch = this.epoch; receipt.ticketToken = token;
    receipt.requestOrdinal = plan.ordinal[row]; receipt.requested = requested; receipt.localAllowed = local;
    receipt.reserved = amount; receipt.source = c; receipt.target = target; return 0;
  }
  private slotHot(token: number): number {
    if (this.work.active !== 1 || this.epoch !== this.work.stepEpoch || !Number.isSafeInteger(token) || token <= 0) return -1;
    for (let i = 0; i < this.ticketCount; i++) if (this.tokens[i] === token && this.closed[i] === 0) return i;
    return -1;
  }
  refundInto(token: number, amount: number, out: P011ParticleHotRefundReceipt): number {
    const i = this.slotHot(token); if (i < 0) return 4;
    if (!this.storageValidHot()) return 3;
    if (!(out instanceof P011ParticleHotRefundReceipt) || !out.storageValidHot()) return 3;
    if (!Number.isFinite(amount) || amount < 0 || amount > this.reserved[i] - this.refunded[i]) return 2;
    const c = this.requests.column[i], t = this.requests.kind[i] === 3 ? this.requests.targetColumn[i] : -1;
    if (!Number.isFinite(this.totalUsed) || this.totalUsed < amount || !Number.isFinite(this.totalRemaining + amount) ||
      !Number.isFinite(this.localUsed[c]) || this.localUsed[c] < amount || !Number.isFinite(this.localRemaining[c] + amount) ||
      (t >= 0 && (!Number.isFinite(this.localUsed[t]) || this.localUsed[t] < amount || !Number.isFinite(this.localRemaining[t] + amount)))) return 13;
    this.localUsed[c] -= amount; this.localRemaining[c] += amount;
    if (t >= 0) { this.localUsed[t] -= amount; this.localRemaining[t] += amount; }
    this.totalUsed -= amount; this.totalRemaining += amount; this.refunded[i] += amount;
    this.fillHot(i, amount, 0, false, out); return 0;
  }
  closeInto(token: number, committed: number, out: P011ParticleHotRefundReceipt): number {
    const i = this.slotHot(token); if (i < 0) return 4;
    if (!this.storageValidHot()) return 3;
    if (!(out instanceof P011ParticleHotRefundReceipt) || !out.storageValidHot()) return 3;
    if (!Number.isFinite(committed) || committed < 0 || Math.abs(this.reserved[i] - this.refunded[i] - committed) > 1e-9) return 13;
    this.closed[i] = 1; this.openCount--; this.fillHot(i, 0, committed, true, out); return 0;
  }
  private fillHot(i: number, amount: number, committed: number, closed: boolean, out: P011ParticleHotRefundReceipt): void {
    const t = this.requests.kind[i] === 3 ? this.requests.targetColumn[i] : -1;
    const reserved = this.reserved[i], refunded = this.refunded[i];
    out.ownerId = this.work.ownerId; out.stepEpoch = this.epoch; out.ticketToken = this.tokens[i];
    out.requestOrdinal = this.requests.ordinal[i]; out.requested = this.requests.requested[i]; out.amount = amount;
    out.totalReserved = reserved; out.totalRefunded = refunded; out.localCount = t < 0 ? 1 : 2;
    out.localColumns[0] = this.requests.column[i]; out.localColumns[1] = t;
    for (let j = 0; j < 2; j++) {
      const active = j < out.localCount;
      out.localReserved[j] = active ? reserved : 0; out.localRefunded[j] = active ? refunded : 0;
      out.netLocalUsed[j] = active ? reserved - refunded : 0;
      out.netLocalDelta[j] = active && closed ? reserved - refunded - committed : 0;
    }
    out.netTotalUsed = reserved - refunded; out.netTotalDelta = closed ? reserved - refunded - committed : 0;
    out.netDeltaValid = closed ? 1 : 0; out.status = closed ? 2 : 1; out.bottomReserved = 0; out.bottomRefunded = 0;
  }
}
