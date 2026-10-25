// D2 independent particle preview/live projection; fixed storage, exact numeric comparison.
import { P011ParticleHotWorkspace } from './P011ParticleHot';
class P011ParticleProjectionSlot {
  theta = 0; valid = false; length = 0; readonly values: Float64Array;
  constructor(size: number) { this.values = new Float64Array(size); }
}
export class P011ParticleM2Projection {
  private readonly slots: P011ParticleProjectionSlot[] = [];
  private readonly scratch: Float64Array;
  private cursor = 0;
  constructor(work: P011ParticleHotWorkspace) {
    const size = 4 + work.requestCapacity * 16 + work.nmax * 11;
    this.scratch = new Float64Array(size);
    for (let i = 0; i < 8; i++) this.slots.push(new P011ParticleProjectionSlot(size));
  }
  clear(): void { for (let i = 0; i < this.slots.length; i++) this.slots[i].valid = false; this.cursor = 0; }
  private write(work: P011ParticleHotWorkspace, out: Float64Array): number {
    const p = work.planA, r = work.results, pool = work.patchAfter; let n = 0;
    out[n++] = p.requestCount; out[n++] = p.totalRequested; out[n++] = pool.liveCount; out[n++] = pool.nextId;
    for (let i = 0; i < p.requestCount; i++) {
      out[n++] = p.kind[i]; out[n++] = p.particleId[i]; out[n++] = p.sourceId[i]; out[n++] = p.column[i];
      out[n++] = p.targetColumn[i]; out[n++] = p.requested[i]; out[n++] = r.granted[i];
      out[n++] = r.budgetClipped[i]; out[n++] = r.guardRejected[i]; out[n++] = r.capacityClipped[i];
      out[n++] = r.poolClipped[i]; out[n++] = r.remainder[i]; out[n++] = p.predictedX[i];
      out[n++] = p.predictedY[i]; out[n++] = p.predictedVx[i]; out[n++] = p.predictedVy[i];
    }
    for (let i = 0; i < pool.nmax; i++) {
      out[n++] = pool.pState[i]; out[n++] = pool.pId[i]; out[n++] = pool.pSourceId[i];
      out[n++] = pool.pPhase[i]; out[n++] = pool.pSource[i]; out[n++] = pool.pMass[i];
      out[n++] = pool.pX[i]; out[n++] = pool.pY[i]; out[n++] = pool.pVx[i]; out[n++] = pool.pVy[i];
      out[n++] = pool.freeSlots[i];
    }
    return n;
  }
  capture(theta: number, work: P011ParticleHotWorkspace): void {
    let slot = this.slots[this.cursor];
    for (let i = 0; i < this.slots.length; i++) if (this.slots[i].valid && this.slots[i].theta === theta) slot = this.slots[i];
    slot.theta = theta; slot.length = this.write(work, slot.values); slot.valid = true;
    this.cursor = (this.cursor + 1) % this.slots.length;
  }
  consistent(theta: number, work: P011ParticleHotWorkspace): boolean {
    const length = this.write(work, this.scratch);
    for (let i = 0; i < this.slots.length; i++) {
      const slot = this.slots[i];
      if (!slot.valid || slot.theta !== theta || slot.length !== length) continue;
      for (let j = 0; j < length; j++) if (!Number.isFinite(this.scratch[j]) || this.scratch[j] !== slot.values[j]) return false;
      return true;
    }
    return false;
  }
}
