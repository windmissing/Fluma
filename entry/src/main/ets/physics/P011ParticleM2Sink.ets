// P-011 D2: production transaction sink. M2 owns canonical field writes; M1 only prepares pool patches.
import { P011ParticleHotWorkspace, P011ParticleHotPlanBuffer, P011ParticleHotResultBuffer,
  P011ParticleHotCommitSink, P011ParticleHotEventBuffer, validateParticleHotRequest,
  preparePatchHot, publishPatchHot, finishPatchHot, rollbackStepHot, auditParticleHot } from './P011ParticleHot';
import { P011ParticleHotBudgetAdapter, P011ParticleHotTicketReceipt,
  P011ParticleHotRefundReceipt } from './P011ParticleHotBudget';

export interface P011ParticleM2Host {
  internalParticleReserve(column: number, target: number, requested: number): number;
  internalParticleRefund(column: number, target: number, amount: number): void;
  internalParticleRestoreBudget(remaining: Float64Array, used: Float64Array,
    totalRemaining: number, totalUsed: number): void;
  internalParticleGuard(column: number, sign: number, requested: number): number;
}
export class P011ParticleM2BudgetAdapter extends P011ParticleHotBudgetAdapter {
  readonly host: P011ParticleM2Host;
  private readonly hostBeforeRemaining: Float64Array;
  private readonly hostBeforeUsed: Float64Array;
  private liveBound = false;
  private hostBeforeTotalRemaining = 0;
  private hostBeforeTotalUsed = 0;
  constructor(work: P011ParticleHotWorkspace, host: P011ParticleM2Host) {
    super(work); this.host = host;
    this.hostBeforeRemaining = new Float64Array(work.cols);
    this.hostBeforeUsed = new Float64Array(work.cols);
  }
  beginHot(epoch: number, dt: number): number { this.liveBound = false; return super.beginHot(epoch, dt); }
  bindLiveBudget(remaining: Float64Array, used: Float64Array, totalRemaining: number, totalUsed: number): void {
    this.liveBound = true;
    this.localRemaining.set(remaining); this.localUsed.set(used);
    this.totalRemaining = totalRemaining; this.totalUsed = totalUsed;
    this.hostBeforeRemaining.set(remaining); this.hostBeforeUsed.set(used);
    this.hostBeforeTotalRemaining = totalRemaining; this.hostBeforeTotalUsed = totalUsed;
  }
  auditHot(): number {
    if (!this.liveBound) return super.auditHot();
    if (!Number.isFinite(this.totalUsed) || !Number.isFinite(this.totalRemaining) ||
      this.totalUsed < -1e-9 || this.totalRemaining < -1e-9 ||
      Math.abs(this.totalUsed + this.totalRemaining - this.hostBeforeTotalUsed - this.hostBeforeTotalRemaining) > 1e-9) return 13;
    for (let c = 0; c < this.work.cols; c++) if (!Number.isFinite(this.localRemaining[c]) ||
      !Number.isFinite(this.localUsed[c]) || this.localRemaining[c] < -1e-9 || this.localUsed[c] < -1e-9 ||
      Math.abs(this.localRemaining[c] + this.localUsed[c] - this.hostBeforeRemaining[c] - this.hostBeforeUsed[c]) > 1e-9) return 13;
    return 0;
  }
  reserveInto(epoch: number, plan: P011ParticleHotPlanBuffer, row: number,
    receipt: P011ParticleHotTicketReceipt): number {
    const status = super.reserveInto(epoch, plan, row, receipt);
    if (status !== 0) return status;
    const actual = this.host.internalParticleReserve(receipt.source, receipt.target, receipt.requested);
    return actual === receipt.reserved ? 0 : 13;
  }
  refundInto(token: number, amount: number, out: P011ParticleHotRefundReceipt): number {
    const status = super.refundInto(token, amount, out);
    if (status !== 0) return status;
    this.host.internalParticleRefund(out.localColumns[0], out.localCount === 2 ? out.localColumns[1] : -1, amount);
    return 0;
  }
  rollbackHot(epoch: number): number {
    const status = super.rollbackHot(epoch);
    if (status === 0 && this.liveBound) this.host.internalParticleRestoreBudget(this.hostBeforeRemaining,
      this.hostBeforeUsed, this.hostBeforeTotalRemaining, this.hostBeforeTotalUsed);
    return status;
  }
}
export class P011ParticleHotGuardBuffer {
  count = 0; accepted = 0; rejected = 0; limitingEdge = -1; allLegalBefore = 1;
  readonly edgeId = new Int32Array(3); readonly residual = new Float64Array(3); readonly influence = new Float64Array(3);
  private readonly edgesBuffer = this.edgeId.buffer;
  private readonly residualBuffer = this.residual.buffer;
  private readonly influenceBuffer = this.influence.buffer;
  coldBuffers(): ArrayBuffer[] { return [this.edgesBuffer, this.residualBuffer, this.influenceBuffer]; }
  storageValidHot(): boolean {
    return this.edgeId.buffer === this.edgesBuffer && this.residual.buffer === this.residualBuffer &&
      this.influence.buffer === this.influenceBuffer && this.edgeId.length === 3 && this.residual.length === 3 && this.influence.length === 3;
  }
  copyFromHot(a: P011ParticleHotGuardBuffer): void {
    this.count = a.count; this.accepted = a.accepted; this.rejected = a.rejected;
    this.limitingEdge = a.limitingEdge; this.allLegalBefore = a.allLegalBefore;
    this.edgeId.set(a.edgeId); this.residual.set(a.residual); this.influence.set(a.influence);
  }
}
function sand(work: P011ParticleHotWorkspace, c: number): number {
  const f = work.field; return f === null ? NaN : f.bh[c] + f.bl[c] + f.mh[c] + f.ml[c];
}
export function guardAdmitInto(work: P011ParticleHotWorkspace, expectedStepEpoch: number, column: number,
  deltaSign: number, requested: number, out: P011ParticleHotGuardBuffer): number {
  if (work.active !== 1 || work.stepEpoch !== expectedStepEpoch) return 4;
  if (!(out instanceof P011ParticleHotGuardBuffer) || !out.storageValidHot()) return 3;
  if (!Number.isInteger(column) || column < 0 || column >= work.cols || (deltaSign !== -1 && deltaSign !== 1) ||
    !Number.isFinite(requested) || requested < 0) return 2;
  const hard = Math.tan(Math.PI / 6); let count = 0;
  if (column > 0) { out.edgeId[count] = column - 1; out.influence[count++] = deltaSign; }
  if (column + 1 < work.cols) { out.edgeId[count] = column; out.influence[count++] = 0 - deltaSign; }
  out.count = count; out.accepted = requested; out.rejected = 0; out.limitingEdge = -1; out.allLegalBefore = 1;
  let loBound = 0; let hiBound = requested;
  for (let i = 0; i < count; i++) {
    const edge = out.edgeId[i]; const d = sand(work, edge + 1) - sand(work, edge) - Math.tan(work.effectiveTiltRad);
    out.residual[i] = d;
    if (requested === 0) continue;
    const k = out.influence[i];
    if (!Number.isFinite(d)) { out.accepted = 0; out.rejected = requested; out.allLegalBefore = 0; out.limitingEdge = edge; return 0; }
    const legal = Math.abs(d) <= hard; if (!legal) out.allLegalBefore = 0;
    const bound = legal ? hard : Math.abs(d);
    let low = (-bound - d) / k; let high = (bound - d) / k;
    if (low > high) { const swap = low; low = high; high = swap; }
    if (low > loBound) { loBound = low; out.limitingEdge = edge; }
    if (high < hiBound) { hiBound = high; out.limitingEdge = edge; }
  }
  out.accepted = loBound > hiBound ? 0 : Math.max(0, Math.min(requested, hiBound));
  out.rejected = Math.max(0, requested - out.accepted); return 0;
}
function clearEventRow(out: P011ParticleHotEventBuffer, i: number): void {
  out.kind[i] = 0; out.phase[i] = 0; out.sourceKind[i] = 0; out.flag[i] = 0; out.remainderDisposition[i] = 0;
  out.particleId[i] = 0; out.sourceId[i] = 0; out.requestOrdinal[i] = 0; out.column[i] = 0; out.targetColumn[i] = -1;
  out.lineageStart[i] = out.lineageCount; out.lineageLength[i] = 0;
  out.requested[i] = 0; out.granted[i] = 0; out.remainder[i] = 0; out.massBefore[i] = 0; out.massAfter[i] = 0;
  out.x[i] = 0; out.y[i] = 0; out.vx[i] = 0; out.vy[i] = 0; out.deltaS[i] = 0;
  out.budgetLocal[i] = 0; out.budgetTotal[i] = 0; out.budgetBottom[i] = 0;
  out.budgetClipped[i] = 0; out.guardRejected[i] = 0; out.capacityClipped[i] = 0; out.poolClipped[i] = 0;
  out.toSuspH[i] = 0; out.toSuspL[i] = 0; out.guardCount[i] = 0; out.guardLimitingEdge[i] = -1;
  out.guardAllLegal[i] = 1; out.guardAccepted[i] = 0; out.guardRejectedAmount[i] = 0;
}
function copyAction(work: P011ParticleHotWorkspace, action: number, ordinal: number): void {
  const a = work.patchActions, e = work.events, i = e.count++; clearEventRow(e, i);
  e.kind[i] = a.kind[action]; e.phase[i] = a.phase[action]; e.sourceKind[i] = a.sourceKind[action];
  e.particleId[i] = a.particleId[action]; e.sourceId[i] = a.sourceId[action]; e.column[i] = a.column[action];
  e.requestOrdinal[i] = ordinal; e.requested[i] = a.requested[action]; e.granted[i] = a.granted[action];
  e.massBefore[i] = a.massBefore[action]; e.massAfter[i] = a.massAfter[action];
  e.x[i] = a.x[action]; e.y[i] = a.y[action]; e.vx[i] = a.vx[action]; e.vy[i] = a.vy[action];
  e.lineageLength[i] = a.lineageLength[action];
  for (let j = 0; j < a.lineageLength[action]; j++) e.lineage[e.lineageCount++] = a.lineage[a.lineageStart[action] + j];
}
export class P011ParticleM2CommitSink implements P011ParticleHotCommitSink {
  readonly work: P011ParticleHotWorkspace;
  readonly budget: P011ParticleM2BudgetAdapter;
  readonly host: P011ParticleM2Host;
  readonly ticket = new P011ParticleHotTicketReceipt(); readonly refund = new P011ParticleHotRefundReceipt();
  readonly guard = new P011ParticleHotGuardBuffer();
  private readonly beforeTicket = new P011ParticleHotTicketReceipt();
  private readonly beforeRefund = new P011ParticleHotRefundReceipt();
  private readonly beforeGuard = new P011ParticleHotGuardBuffer();
  private readonly ticketBinding = this.ticket;
  private readonly refundBinding = this.refund;
  private readonly guardBinding = this.guard;
  failAfterPublish = 0;
  // Chain-tail T2 already passed the real clip/guard and bed extraction.
  birthOnly = 0;
  constructor(work: P011ParticleHotWorkspace, host: P011ParticleM2Host) {
    if (work.active !== 0 || work.commitSink !== null) throw new Error('PARTICLE_HOT_ALREADY_BOUND');
    this.work = work;
    this.host = host;
    this.budget = new P011ParticleM2BudgetAdapter(work, host); work.budget = this.budget; work.commitSink = this;
    work.sinkCheckpoint = this;
    work.registerColdBuffers(this.budget.coldBuffers()); work.registerColdBuffers(this.refund.coldBuffers());
    work.registerColdBuffers(this.guard.coldBuffers()); work.registerColdBuffers(this.beforeRefund.coldBuffers());
    work.registerColdBuffers(this.beforeGuard.coldBuffers());
  }
  buffersValidHot(): boolean {
    return this.work.budget === this.budget && this.work.commitSink === this && this.ticket === this.ticketBinding &&
      this.refund === this.refundBinding && this.guard === this.guardBinding && this.budget.storageValidHot() &&
      this.refund.storageValidHot() && this.guard.storageValidHot() && this.beforeRefund.storageValidHot() && this.beforeGuard.storageValidHot();
  }
  saveOutsHot(): void {
    this.beforeTicket.copyFromHot(this.ticket); this.beforeRefund.copyFromHot(this.refund); this.beforeGuard.copyFromHot(this.guard);
  }
  restoreOutsHot(): void {
    this.ticket.copyFromHot(this.beforeTicket); this.refund.copyFromHot(this.beforeRefund); this.guard.copyFromHot(this.beforeGuard);
  }
  private failHot(code: number): number {
    const rolled = rollbackStepHot(this.work, this.work.stepEpoch);
    return rolled === 0 ? code : rolled;
  }
  applyInto(work: P011ParticleHotWorkspace, expectedStepEpoch: number, p: P011ParticleHotPlanBuffer,
    row: number, result: P011ParticleHotResultBuffer): number {
    if (work !== this.work || expectedStepEpoch !== work.stepEpoch || work.active !== 1) return 4;
    if (result !== work.results || !result.storageValidHot()) return 3;
    const valid = validateParticleHotRequest(work, p, row); if (valid !== 0) return valid;
    const f = work.field; if (f === null) return 1;
    const c = p.column[row], kind = p.kind[row], requested = p.requested[row];
    let granted = requested, budgetClipped = 0, guardRejected = 0, capacityClipped = 0, poolClipped = 0;
    let ticketToken = 0; let degraded = 0; let guarded = 0; let disposition = 1;
    const copied = work.pool.transferHotState(work.patchScratch, false); if (copied !== 0) return copied;
    const target = kind === 3 ? p.targetColumn[row] : c;
    if (target < 0 || target >= work.cols) return 2;
    const free = (f.mediumMode === 'DRY_AIR_V1' ? f.capacity : f.waterTop[target]) - sand(work, target) - f.suspH[target] - f.suspL[target] -
      work.patchScratch.colParticleMassH[target] - work.patchScratch.colParticleMassL[target];
    if ((kind === 1 || kind === 3) && free + 1e-9 < requested) { capacityClipped = requested; granted = 0; }
    else if (kind === 2 || kind === 3 || kind === 4 && this.birthOnly === 0) {
      const reserve = this.budget.reserveInto(expectedStepEpoch, p, row, this.ticket); if (reserve !== 0) return this.failHot(reserve);
      ticketToken = this.ticket.ticketToken; granted = this.ticket.reserved;
      if (kind === 3 && granted < requested) {
        const refunded = this.budget.refundInto(ticketToken, granted, this.refund); if (refunded !== 0) return this.failHot(refunded);
        granted = 0; budgetClipped = requested;
      } else {
        budgetClipped = requested - granted;
        if (kind !== 3) {
          const status = guardAdmitInto(work, expectedStepEpoch, c, kind === 2 ? 1 : -1, granted, this.guard);
          if (status !== 0) return this.failHot(status);
          const actual = this.host.internalParticleGuard(c, kind === 2 ? 1 : -1, granted);
          if (!Number.isFinite(actual) || actual < 0 || actual > granted) return this.failHot(13);
          this.guard.accepted = actual; this.guard.rejected = granted - actual; guarded = 1;
          const refunded = this.budget.refundInto(ticketToken, this.guard.rejected, this.refund);
          if (refunded !== 0) return this.failHot(refunded);
          guardRejected = this.guard.rejected; granted = this.guard.accepted;
        }
      }
    }
    let gh = p.phase[row] === 0 ? granted : 0, gl = p.phase[row] === 1 ? granted : 0;
    // Stage the field exactly as the cold reference (mixed -> light -> heavy).
    if (kind === 2) { disposition = granted > 0 ? 2 : 6; f.mh[c] += gh; f.ml[c] += gl; }
    else if (kind === 4 && this.birthOnly === 0) {
      const available = sand(work, c);
      if (granted > available) {
        const clipped = granted - available; capacityClipped += clipped;
        if (ticketToken !== 0) { const status = this.budget.refundInto(ticketToken, clipped, this.refund); if (status !== 0) return this.failHot(status); }
        granted = available;
      }
      const fromMix = Math.min(granted, f.mh[c] + f.ml[c]); const scale = fromMix / Math.max(f.mh[c] + f.ml[c], 1e-9);
      gh = f.mh[c] * scale; gl = f.ml[c] * scale;
      let remaining = granted - fromMix; const fromLight = Math.min(remaining, f.bl[c]); gl += fromLight; remaining -= fromLight;
      const fromHeavy = Math.min(remaining, f.bh[c]); gh += fromHeavy;
      f.mh[c] -= f.mh[c] * scale; f.ml[c] -= f.ml[c] * scale; f.bl[c] -= fromLight; f.bh[c] -= fromHeavy;
      disposition = granted > 0 ? 1 : 6;
    } else if (kind === 5) disposition = 3;
    else if (kind === 3) disposition = granted > 0 ? 4 : 6;
    else if (kind === 6) disposition = 5;
    else if (granted === 0) disposition = 6;
    f.surface[c] = sand(work, c);
    work.stagedYActive = kind === 2 || kind === 4 ? 1 : 0; work.stagedY = sand(work, c) + work.params.contactEps;
    let token = preparePatchHot(work, p, row, gh, gl, disposition);
    if (token === 0 && work.lastStatus === 5 && kind === 4 && granted > 0 && f.mediumMode !== 'DRY_AIR_V1') {
      disposition = 3; degraded = 1; token = preparePatchHot(work, p, row, gh, gl, disposition);
    } else if (token === 0 && work.lastStatus === 5 && kind === 1) {
      poolClipped = granted; granted = 0; gh = 0; gl = 0; disposition = 6;
      token = preparePatchHot(work, p, row, gh, gl, disposition);
    }
    work.stagedYActive = 0;
    if (token === 0) return this.failHot(work.lastStatus);
    if (kind === 5 || degraded === 1) { f.suspH[c] += gh; f.suspL[c] += gl; }
    let births = 0;
    for (let i = 0; i < work.patchActions.count; i++) if (work.patchActions.kind[i] === 1) births++;
    const main = kind === 6 || (kind === 1 && births > 0) ? 0 : 1;
    const eventCount = work.patchActions.count + main + degraded;
    if (work.events.count + eventCount > work.events.capacity) return this.failHot(9);
    if (work.events.lineageCount + work.patchActions.lineageCount > work.events.lineageCapacity) return this.failHot(10);
    const published = publishPatchHot(work, token); if (published !== 0) return this.failHot(published);
    if (kind === 1 || this.birthOnly === 1) { work.stepExternalH += gh; work.stepExternalL += gl; }
    if (f.mediumMode === 'DRY_AIR_V1') {
      const pool=work.pool;
      f.waterTop[c]=sand(work,c)+f.suspH[c]+f.suspL[c]+pool.particleMassAt(c);
      if(target!==c)f.waterTop[target]=sand(work,target)+f.suspH[target]+f.suspL[target]+pool.particleMassAt(target);
    }
    const audit = auditParticleHot(work); if (audit !== 0) return this.failHot(audit);
    if (this.failAfterPublish !== 0) return this.failHot(13);
    if (ticketToken !== 0) { const closed = this.budget.closeInto(ticketToken, granted, this.refund); if (closed !== 0) return this.failHot(closed); }
    const begin = work.events.count;
    for (let i = 0; i < work.patchActions.count; i++) copyAction(work, i, p.ordinal[row]);
    let guardEvent = -1;
    if (main !== 0) {
      const e = work.events, i = e.count++; clearEventRow(e, i);
      e.kind[i] = kind; e.requestOrdinal[i] = p.ordinal[row]; e.particleId[i] = p.particleId[row]; e.sourceId[i] = p.sourceId[row];
      e.phase[i] = p.phase[row]; e.sourceKind[i] = p.sourceKind[row]; e.column[i] = c; e.targetColumn[i] = p.targetColumn[row];
      e.requested[i] = requested; e.granted[i] = granted; e.remainder[i] = requested - granted;
      e.massBefore[i] = requested; e.massAfter[i] = requested - granted;
      e.deltaS[i] = kind === 2 ? granted : kind === 4 ? -granted : 0;
      e.budgetLocal[i] = granted; e.budgetTotal[i] = ticketToken === 0 && this.birthOnly === 0 ? 0 : granted;
      e.flag[i] = kind === 5 ? 1 : 0; e.remainderDisposition[i] = kind === 2 && requested > granted ? 1 : 0;
      e.budgetClipped[i] = budgetClipped; e.guardRejected[i] = guardRejected;
      e.capacityClipped[i] = capacityClipped; e.poolClipped[i] = poolClipped;
      if (kind === 5) { e.toSuspH[i] = gh; e.toSuspL[i] = gl; }
      if (guarded === 1) {
        guardEvent = i; e.guardCount[i] = this.guard.count; e.guardLimitingEdge[i] = this.guard.limitingEdge;
        e.guardAllLegal[i] = this.guard.allLegalBefore; e.guardAccepted[i] = this.guard.accepted;
        e.guardRejectedAmount[i] = this.guard.rejected;
        for (let j = 0; j < this.guard.count; j++) {
          e.guardEdgeId[i * 3 + j] = this.guard.edgeId[j]; e.guardResidual[i * 3 + j] = this.guard.residual[j];
          e.guardInfluence[i * 3 + j] = this.guard.influence[j];
        }
      }
      const start = p.mergeStart[row]; e.lineageLength[i] = p.mergeCount[row];
      for (let j = 0; j < p.mergeCount[row]; j++) e.lineage[e.lineageCount++] = p.lineage[start + j];
    }
    if (degraded === 1) {
      const e = work.events, i = e.count++; clearEventRow(e, i); e.kind[i] = 5; e.flag[i] = 1;
      e.requestOrdinal[i] = p.ordinal[row]; e.sourceId[i] = p.sourceId[row]; e.phase[i] = p.phase[row];
      e.sourceKind[i] = p.sourceKind[row]; e.column[i] = c; e.requested[i] = granted; e.granted[i] = granted;
      e.massBefore[i] = granted; e.massAfter[i] = granted;
      e.toSuspH[i] = gh; e.toSuspL[i] = gl;
    }
    const finished = finishPatchHot(work, token); if (finished !== 0) return this.failHot(finished);
    result.committed[row] = 1; result.disposition[row] = disposition; result.released[row] = requested === granted ? 1 : 0;
    result.rowStatus[row] = 0; result.granted[row] = granted; result.remainder[row] = requested - granted;
    result.budgetClipped[row] = budgetClipped; result.guardRejected[row] = guardRejected;
    result.capacityClipped[row] = capacityClipped; result.poolClipped[row] = poolClipped; result.massAfter[row] = requested - granted;
    result.guardIndex[row] = guardEvent; result.eventBegin[row] = begin; result.eventCount[row] = eventCount;
    return 0;
  }
}
