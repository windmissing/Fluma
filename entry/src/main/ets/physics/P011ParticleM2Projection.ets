import { crossSameF64 } from './derived/P011DryCrossVerticalV1';
import { DerivedProjectionV2 } from './derived/P011DerivedProjectionV2';
// D2 independent particle preview/live projection; fixed storage, exact numeric comparison.
import { P011ParticleHotWorkspace } from './P011ParticleHot';
class P011ParticleProjectionSlot {
  theta = 0; valid = false; length = 0; readonly values: Float64Array;
  plans:object|null=null;run='';attempt='';candidate=0;
  constructor(size: number) { this.values = new Float64Array(size); }
}
export class P011ParticleM2Projection {
  private readonly slots: P011ParticleProjectionSlot[] = [];
  private readonly scratch: Float64Array;
  private cursor = 0;
  private candidate = 0;
  private derived:DerivedProjectionV2|null=null;
  enableDerivedV2():void{if(this.derived===null)this.derived=new DerivedProjectionV2();}
  constructor(work: P011ParticleHotWorkspace) {
    const size = 5 + work.requestCapacity * 16 + work.nmax * 11 +
      (work.crossRows===null?0:13+work.crossBudgetProof.length+work.requestCapacity+work.crossRows.capacity*27+work.nmax*11+work.cols*10);
    this.scratch = new Float64Array(size);
    for (let i = 0; i < 8; i++) this.slots.push(new P011ParticleProjectionSlot(size));
  }
  clear(): void { for (let i = 0; i < this.slots.length; i++) {this.slots[i].valid = false;this.slots[i].plans=null;} this.cursor = 0;this.candidate=0; }
  private write(work: P011ParticleHotWorkspace, out: Float64Array): number {
    const p = work.planA, r = work.results, pool = work.patchAfter; let n = 0;
    out[n++] = work.mediumMode==='DRY_AIR_V1'?1:0;
    out[n++] = p.requestCount; out[n++] = p.totalRequested; out[n++] = pool.liveCount; out[n++] = pool.nextId;
    const rows=work.crossRows;
    if(rows!==null){
      out[n++]=work.crossProof;out[n++]=work.ownerId;out[n++]=work.runToken;
      out[n++]=p.substepId;out[n++]=p.dt;out[n++]=p.effectiveTiltRad;
      out[n++]=p.paramsRevision;out[n++]=p.inputRevision;out[n++]=rows.count;
      out[n++]=work.crossEpoch;out[n++]=work.frozenPool.liveCount;
      out[n++]=work.frozenPool.freeCount;out[n++]=work.frozenPool.nextId;
      for(let c=0;c<work.cols;c++){
        out[n++]=work.fieldBeforeBh[c];out[n++]=work.fieldBeforeBl[c];
        out[n++]=work.fieldBeforeMh[c];out[n++]=work.fieldBeforeMl[c];
        out[n++]=work.fieldBeforeWaterTop[c];out[n++]=work.fieldBeforeSurface[c];
        out[n++]=work.fieldBeforeSuspH[c];out[n++]=work.fieldBeforeSuspL[c];
        out[n++]=work.frozenPool.colParticleMassH[c];out[n++]=work.frozenPool.colParticleMassL[c];
      }
      for(let i=0;i<work.crossBudgetProof.length;i++)out[n++]=work.crossBudgetProof[i];
      for(let i=0;i<rows.count*8;i++)out[n++]=rows.keys[i];
      for(let i=0;i<rows.count*20;i++)if(i%20!==14)out[n++]=rows.values[i];
    }
    for (let i = 0; i < p.requestCount; i++) {
      if(rows!==null)out[n++]=p.blockedYFree[i];
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
      if(rows!==null){
        const before=work.frozenPool;
        out[n++]=before.pState[i];out[n++]=before.pId[i];out[n++]=before.pSourceId[i];
        out[n++]=before.pPhase[i];out[n++]=before.pSource[i];out[n++]=before.pMass[i];
        out[n++]=before.pX[i];out[n++]=before.pY[i];out[n++]=before.pVx[i];out[n++]=before.pVy[i];
        out[n++]=before.freeSlots[i];
      }
    }
    return n;
  }
  capture(theta: number, work: P011ParticleHotWorkspace, plans:object|null=null): void {
    let slot = this.slots[this.cursor];
    if(work.crossRows===null)for (let i = 0; i < this.slots.length; i++) if (this.slots[i].valid && this.slots[i].theta === theta) slot = this.slots[i];
    if(work.crossRows!==null&&plans===null)throw new Error('CROSS_PREVIEW_IDENTITY');
    slot.plans=plans;slot.run=work.crossRun;slot.attempt=work.crossAttempt;slot.candidate=++this.candidate;
    if(work.derivedV2!==null){if(this.derived===null)throw new Error('DERIVED_PROJECTION_UNBOUND');this.derived.capture(this.slots.indexOf(slot),work);}
    slot.theta = theta; slot.length = this.write(work, slot.values); slot.valid = true;
    this.cursor = (this.cursor + 1) % this.slots.length;
  }
  consistent(theta: number, work: P011ParticleHotWorkspace, plans:object|null=null): boolean {
    const length = this.write(work, this.scratch);
    for (let i = 0; i < this.slots.length; i++) {
      const slot = this.slots[i];
      if (!slot.valid || slot.theta !== theta || slot.length !== length) continue;
      if(work.crossRows!==null&&(plans===null||plans!==slot.plans||slot.run!==work.crossRun||
        slot.attempt!==work.crossAttempt||slot.candidate<=0))continue;
      if(work.derivedV2!==null&&(this.derived===null||!this.derived.consistent(i,work)))return false;
      for (let j = 0; j < length; j++) if (!Number.isFinite(this.scratch[j]) || (work.crossRows===null?this.scratch[j]!==slot.values[j]:!crossSameF64(this.scratch[j],slot.values[j]))) return false;
      return true;
    }
    return false;
  }
}
