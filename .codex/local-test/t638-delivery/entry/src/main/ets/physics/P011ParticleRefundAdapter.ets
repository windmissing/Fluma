// P-011 D1 R9 / rev2 (2026-10-24). Isolated adapter storage binding;
// production WaterSandField binding belongs to D2. No legacy refund calls.
import { P011ParticleBudgetTicket } from './P011ParticleField';

export interface P011ParticleRefundLedger {
  localRemaining: Float64Array; localUsed: Float64Array;
  totalRemaining: number; totalUsed: number;
}
export interface P011ParticleRefundReceipt {
  ticketId: number; requestKey: string; substepId: number;
  requested: number; amount: number; totalReserved: number; totalRefunded: number;
  localCount: number; localColumns: Int32Array;
  localReserved: Float64Array; localRefunded: Float64Array;
  netLocalUsed: Float64Array; netLocalDelta: Float64Array;
  netTotalUsed: number; netTotalDelta: number; netDeltaValid: boolean;
  bottomReserved: number; bottomRefunded: number;
  status: 'REFUNDED' | 'CLOSED';
}
export interface P011ParticleTicketRefundAdapter {
  refundParticleTicket(ticketId: number, amount: number, out: P011ParticleRefundReceipt): void;
  closeParticleTicket(ticketId: number, committed: number, out: P011ParticleRefundReceipt): void;
}
// Allocate outside the transaction loop; a receipt is caller-owned scratch.
export class P011ParticleRefundReceiptBuffer implements P011ParticleRefundReceipt {
  ticketId = 0; requestKey = ''; substepId = -1;
  requested = 0; amount = 0; totalReserved = 0; totalRefunded = 0;
  localCount = 0; localColumns = new Int32Array(2);
  localReserved = new Float64Array(2); localRefunded = new Float64Array(2);
  netLocalUsed = new Float64Array(2); netLocalDelta = new Float64Array(2);
  netTotalUsed = 0; netTotalDelta = 0; netDeltaValid = false;
  bottomReserved = 0; bottomRefunded = 0;
  status: 'REFUNDED' | 'CLOSED' = 'REFUNDED';
  copyFrom(source: P011ParticleRefundReceipt): void {
    this.ticketId = source.ticketId; this.requestKey = source.requestKey; this.substepId = source.substepId;
    this.requested = source.requested; this.amount = source.amount;
    this.totalReserved = source.totalReserved; this.totalRefunded = source.totalRefunded;
    this.localCount = source.localCount; this.localColumns.set(source.localColumns);
    this.localReserved.set(source.localReserved); this.localRefunded.set(source.localRefunded);
    this.netLocalUsed.set(source.netLocalUsed); this.netLocalDelta.set(source.netLocalDelta);
    this.netTotalUsed = source.netTotalUsed; this.netTotalDelta = source.netTotalDelta;
    this.netDeltaValid = source.netDeltaValid; this.bottomReserved = source.bottomReserved;
    this.bottomRefunded = source.bottomRefunded; this.status = source.status;
  }
}

export class P011ParticleRefundAdapter implements P011ParticleTicketRefundAdapter {
  private readonly ledger: P011ParticleRefundLedger;
  private readonly capacity: number;
  private readonly ids: Float64Array;
  private readonly steps: Float64Array;
  private readonly sources: Int32Array;
  private readonly targets: Int32Array;
  private readonly reserved: Float64Array;
  private readonly refunded: Float64Array;
  private readonly requests: Float64Array;
  private readonly keys: string[];
  private readonly closed: Uint8Array;
  private substepId: number;
  private maxId = 0;

  constructor(ledger: P011ParticleRefundLedger, substepId: number, capacity: number = 4096) {
    if (!Number.isInteger(capacity) || capacity <= 0) throw new Error('PARTICLE_INVALID_SIZE');
    this.ledger = ledger; this.capacity = capacity; this.substepId = substepId;
    this.ids = new Float64Array(capacity); this.steps = new Float64Array(capacity);
    this.sources = new Int32Array(capacity); this.targets = new Int32Array(capacity);
    this.reserved = new Float64Array(capacity); this.refunded = new Float64Array(capacity);
    this.requests = new Float64Array(capacity); this.keys = new Array<string>(capacity);
    this.keys.fill(''); this.closed = new Uint8Array(capacity);
  }
  beginSubstep(substepId: number): void {
    if (!Number.isSafeInteger(substepId) || substepId < 0) throw new Error('PARTICLE_INVALID_STEP');
    if (substepId <= this.substepId) throw new Error('PARTICLE_STALE_STEP');
    this.substepId = substepId;
  }
  registerTicket(ticket: P011ParticleBudgetTicket, isClosed: boolean = false): void {
    const id = ticket.ticketId;
    const a = ticket.reserved; const f = ticket.refunded;
    if (!Number.isSafeInteger(id) || id <= this.maxId || ticket.substepId !== this.substepId ||
      !Number.isFinite(a) || !Number.isFinite(f) || a < 0 || f < 0 || f > a ||
      !Number.isFinite(ticket.requested) || ticket.requested < a || !ticket.requestKey ||
      !Number.isInteger(ticket.source) || ticket.source < 0 || ticket.source >= this.ledger.localUsed.length ||
      !Number.isInteger(ticket.target) || ticket.target < -1 || ticket.target >= this.ledger.localUsed.length ||
      ticket.target === ticket.source) throw new Error('PARTICLE_INVALID_SIDECAR');
    const slot = id % this.capacity;
    if (this.ids[slot] !== 0 && this.steps[slot] === this.substepId) throw new Error('PARTICLE_TICKET_OVERFLOW');
    this.ids[slot] = id; this.steps[slot] = this.substepId;
    this.sources[slot] = ticket.source; this.targets[slot] = ticket.target;
    this.reserved[slot] = a; this.refunded[slot] = f; this.requests[slot] = ticket.requested;
    this.keys[slot] = ticket.requestKey; this.closed[slot] = isClosed ? 1 : 0; this.maxId = id;
  }
  // Cold checkpoint restore: no ledger mutation and no new buffers.
  resetForRestore(): void {
    this.ids.fill(0); this.closed.fill(0); this.maxId = 0;
  }
  private slot(ticketId: number): number {
    if (!Number.isSafeInteger(ticketId) || ticketId <= 0) throw new Error('PARTICLE_STALE_TICKET');
    const slot = ticketId % this.capacity;
    if (this.ids[slot] !== ticketId || this.steps[slot] !== this.substepId || this.closed[slot] !== 0) {
      throw new Error('PARTICLE_STALE_TICKET');
    }
    return slot;
  }
  private validateOut(out: P011ParticleRefundReceipt): void {
    if (out.localColumns.length !== 2 || out.localReserved.length !== 2 || out.localRefunded.length !== 2 ||
      out.netLocalUsed.length !== 2 || out.netLocalDelta.length !== 2 ||
      out.localReserved.buffer === out.localRefunded.buffer || out.netLocalUsed.buffer === out.netLocalDelta.buffer ||
      out.localReserved.buffer === out.netLocalUsed.buffer || out.localReserved.buffer === out.netLocalDelta.buffer ||
      out.localRefunded.buffer === out.netLocalUsed.buffer || out.localRefunded.buffer === out.netLocalDelta.buffer ||
      out.localColumns.buffer === out.localReserved.buffer || out.localColumns.buffer === out.localRefunded.buffer ||
      out.localColumns.buffer === out.netLocalUsed.buffer || out.localColumns.buffer === out.netLocalDelta.buffer ||
      out.localColumns.buffer === this.ledger.localUsed.buffer || out.localColumns.buffer === this.ledger.localRemaining.buffer ||
      out.localReserved.buffer === this.ledger.localUsed.buffer || out.localRefunded.buffer === this.ledger.localUsed.buffer ||
      out.netLocalUsed.buffer === this.ledger.localUsed.buffer || out.netLocalDelta.buffer === this.ledger.localUsed.buffer ||
      out.localReserved.buffer === this.ledger.localRemaining.buffer || out.localRefunded.buffer === this.ledger.localRemaining.buffer ||
      out.netLocalUsed.buffer === this.ledger.localRemaining.buffer || out.netLocalDelta.buffer === this.ledger.localRemaining.buffer) {
      throw new Error('PARTICLE_INVALID_RECEIPT_BUFFER');
    }
  }
  refundParticleTicket(ticketId: number, amount: number, out: P011ParticleRefundReceipt): void {
    const slot = this.slot(ticketId); this.validateOut(out);
    if (!Number.isFinite(amount) || amount < 0 || amount > this.reserved[slot] - this.refunded[slot]) {
      throw new Error('PARTICLE_OVER_REFUND');
    }
    const s = this.sources[slot]; const t = this.targets[slot];
    // All checks precede the first write. No clamp, callback or allocation.
    if (!Number.isFinite(this.ledger.totalUsed) || this.ledger.totalUsed < amount ||
      !Number.isFinite(this.ledger.totalRemaining + amount) ||
      !Number.isFinite(this.ledger.localUsed[s]) || this.ledger.localUsed[s] < amount ||
      !Number.isFinite(this.ledger.localRemaining[s] + amount) ||
      (t >= 0 && (!Number.isFinite(this.ledger.localUsed[t]) || this.ledger.localUsed[t] < amount ||
        !Number.isFinite(this.ledger.localRemaining[t] + amount)))) throw new Error('PARTICLE_REFUND_LEDGER_MISMATCH');
    this.ledger.localUsed[s] -= amount; this.ledger.localRemaining[s] += amount;
    if (t >= 0) { this.ledger.localUsed[t] -= amount; this.ledger.localRemaining[t] += amount; }
    this.ledger.totalUsed -= amount; this.ledger.totalRemaining += amount;
    this.refunded[slot] += amount;
    this.fill(slot, amount, 0, false, out);
  }
  closeParticleTicket(ticketId: number, committed: number, out: P011ParticleRefundReceipt): void {
    const slot = this.slot(ticketId); this.validateOut(out);
    const net = this.reserved[slot] - this.refunded[slot];
    if (!Number.isFinite(committed) || committed < 0 || Math.abs(net - committed) > 1e-9) {
      throw new Error('PARTICLE_TICKET_UNBALANCED');
    }
    this.closed[slot] = 1;
    this.fill(slot, 0, committed, true, out);
  }
  private fill(slot: number, amount: number, committed: number, closed: boolean,
    out: P011ParticleRefundReceipt): void {
    const t = this.targets[slot]; const a = this.reserved[slot]; const f = this.refunded[slot];
    out.ticketId = this.ids[slot]; out.requestKey = this.keys[slot]; out.substepId = this.steps[slot];
    out.requested = this.requests[slot]; out.amount = amount; out.totalReserved = a; out.totalRefunded = f;
    out.localCount = t < 0 ? 1 : 2;
    out.localColumns[0] = this.sources[slot]; out.localColumns[1] = t;
    for (let i = 0; i < 2; i++) {
      const active = i < out.localCount;
      out.localReserved[i] = active ? a : 0; out.localRefunded[i] = active ? f : 0;
      out.netLocalUsed[i] = active ? a - f : 0;
      out.netLocalDelta[i] = active && closed ? a - f - committed : 0;
    }
    out.netTotalUsed = a - f; out.netTotalDelta = closed ? a - f - committed : 0;
    out.netDeltaValid = closed; out.status = closed ? 'CLOSED' : 'REFUNDED';
    out.bottomReserved = 0; out.bottomRefunded = 0;
  }
}
