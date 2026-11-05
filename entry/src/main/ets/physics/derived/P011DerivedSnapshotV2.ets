import { FrameBudgetV2 } from './P011FrameBudgetV2';
import { WaterSandSnapshot } from '../WaterSandField';
import { P011ParticleSnapshot } from '../P011ParticleField';
import { P011ExternalSourceSnapshot, P011ExternalAdmissionRecord } from '../P011ExternalAdmissions';
export function createDerivedPayloadV2(n: number): WaterSandSnapshot {
    const p: P011ParticleSnapshot = { nmax: n, cols: 128, liveCount: 0, freeCount: n, nextId: 1, pMass: new Float64Array(n), pX: new Float64Array(n), pY: new Float64Array(n), pVx: new Float64Array(n), pVy: new Float64Array(n), pPhase: new Uint8Array(n), pSource: new Uint8Array(n), pState: new Uint8Array(n), pId: new Uint32Array(n), pSourceId: new Uint32Array(n), freeSlots: new Int32Array(n), colParticleMassH: new Float64Array(128), colParticleMassL: new Float64Array(128) };
    for (let i = 0; i < n; i++)
        p.freeSlots[i] = n - 1 - i;
    const out: WaterSandSnapshot = { schemaVersion: 4, cols: 128, capacity: 256, waterTop: new Float64Array(128), bh: new Float64Array(128), bl: new Float64Array(128), mh: new Float64Array(128), ml: new Float64Array(128), suspH: new Float64Array(128), suspL: new Float64Array(128), hs: new Float64Array(128), hl: new Float64Array(128), sandTop: new Float64Array(128), shakeSourceH: new Float64Array(128), shakeSourceL: new Float64Array(128), particleState: p, externalSourceState: new P011ExternalSourceSnapshot() };
    return out;
}
export function copyDerivedPayloadV2(from: WaterSandSnapshot, to: WaterSandSnapshot,budget:FrameBudgetV2|null=null): number {
    if (from.cols !== 128 || to.cols !== 128 || from.capacity !== 256 || to.capacity !== 256)
        throw new Error('DERIVED_PAYLOAD_SHAPE');
    let copiedBytes=0;
    to.schemaVersion = from.schemaVersion;
    to.mediumMode = from.mediumMode;
    to.airDynamicsRevision = from.airDynamicsRevision;
    to.birthPlacementRevision = from.birthPlacementRevision;
    to.drySourceId = from.drySourceId;
    to.dryLastSequence = from.dryLastSequence;
    to.dryLastInputTime = from.dryLastInputTime;
    to.physicsCommitId = from.physicsCommitId;
    to.waterTop.set(from.waterTop);copiedBytes+=from.waterTop.byteLength;budget?.add(11,from.waterTop.byteLength);
    to.bh.set(from.bh);copiedBytes+=from.bh.byteLength;budget?.add(11,from.bh.byteLength);
    to.bl.set(from.bl);copiedBytes+=from.bl.byteLength;budget?.add(11,from.bl.byteLength);
    to.mh.set(from.mh);copiedBytes+=from.mh.byteLength;budget?.add(11,from.mh.byteLength);
    to.ml.set(from.ml);copiedBytes+=from.ml.byteLength;budget?.add(11,from.ml.byteLength);
    to.suspH.set(from.suspH);copiedBytes+=from.suspH.byteLength;budget?.add(11,from.suspH.byteLength);
    to.suspL.set(from.suspL);copiedBytes+=from.suspL.byteLength;budget?.add(11,from.suspL.byteLength);
    to.hs.set(from.hs);copiedBytes+=from.hs.byteLength;budget?.add(11,from.hs.byteLength);
    to.hl.set(from.hl);copiedBytes+=from.hl.byteLength;budget?.add(11,from.hl.byteLength);
    to.sandTop.set(from.sandTop);copiedBytes+=from.sandTop.byteLength;budget?.add(11,from.sandTop.byteLength);
    if (to.shakeSourceH !== undefined && from.shakeSourceH !== undefined){
        to.shakeSourceH.set(from.shakeSourceH);copiedBytes+=from.shakeSourceH.byteLength;budget?.add(11,from.shakeSourceH.byteLength);}
    if (to.shakeSourceL !== undefined && from.shakeSourceL !== undefined){
        to.shakeSourceL.set(from.shakeSourceL);copiedBytes+=from.shakeSourceL.byteLength;budget?.add(11,from.shakeSourceL.byteLength);}
    to.shakeSourceLiftedH = from.shakeSourceLiftedH;
    to.shakeSourceLiftedL = from.shakeSourceLiftedL;
    to.shakeSourceLandedH = from.shakeSourceLandedH;
    to.shakeSourceLandedL = from.shakeSourceLandedL;
    to.shakeSourceId = from.shakeSourceId;
    to.shakeSourceSequence = from.shakeSourceSequence;
    const a = from.particleState, b = to.particleState;
    if (a === undefined || a === null || b === undefined || b === null || a.nmax !== b.nmax)
        throw new Error('DERIVED_PAYLOAD_POOL');
    b.liveCount = a.liveCount;
    b.freeCount = a.freeCount;
    b.nextId = a.nextId;
    b.pMass.set(a.pMass);copiedBytes+=a.pMass.byteLength;budget?.add(11,a.pMass.byteLength);
    b.pX.set(a.pX);copiedBytes+=a.pX.byteLength;budget?.add(11,a.pX.byteLength);
    b.pY.set(a.pY);copiedBytes+=a.pY.byteLength;budget?.add(11,a.pY.byteLength);
    b.pVx.set(a.pVx);copiedBytes+=a.pVx.byteLength;budget?.add(11,a.pVx.byteLength);
    b.pVy.set(a.pVy);copiedBytes+=a.pVy.byteLength;budget?.add(11,a.pVy.byteLength);
    b.pPhase.set(a.pPhase);copiedBytes+=a.pPhase.byteLength;budget?.add(11,a.pPhase.byteLength);
    b.pSource.set(a.pSource);copiedBytes+=a.pSource.byteLength;budget?.add(11,a.pSource.byteLength);
    b.pState.set(a.pState);copiedBytes+=a.pState.byteLength;budget?.add(11,a.pState.byteLength);
    b.pId.set(a.pId);copiedBytes+=a.pId.byteLength;budget?.add(11,a.pId.byteLength);
    b.pSourceId.set(a.pSourceId);copiedBytes+=a.pSourceId.byteLength;budget?.add(11,a.pSourceId.byteLength);
    b.freeSlots.set(a.freeSlots);copiedBytes+=a.freeSlots.byteLength;budget?.add(11,a.freeSlots.byteLength);
    b.colParticleMassH.set(a.colParticleMassH);copiedBytes+=a.colParticleMassH.byteLength;budget?.add(11,a.colParticleMassH.byteLength);
    b.colParticleMassL.set(a.colParticleMassL);copiedBytes+=a.colParticleMassL.byteLength;budget?.add(11,a.colParticleMassL.byteLength);
    const x = from.externalSourceState, y = to.externalSourceState;
    if (x === undefined || y === undefined)
        throw new Error('DERIVED_PAYLOAD_SOURCE');
    y.revision = x.revision;
    y.initialWater = x.initialWater;
    y.initialHeavy = x.initialHeavy;
    y.initialLight = x.initialLight;
    y.sourceInjectedH = x.sourceInjectedH;
    y.sourceInjectedL = x.sourceInjectedL;
    y.sourceEpoch = x.sourceEpoch;
    const e = x.admissions, o = y.admissions;
    o.revision = e.revision;
    o.count = e.count;
    o.admittedH = e.admittedH;
    o.admittedL = e.admittedL;
    o.observedTransactions = e.observedTransactions;
    o.rawHistoryStatus = e.rawHistoryStatus;
    o.exportFailures = e.exportFailures;
    if (e.lastRecord === null)
        o.lastRecord = null;
    else {
        if (o.lastRecord === null)
            o.lastRecord = new P011ExternalAdmissionRecord();
        const d = o.lastRecord, s = e.lastRecord;
        d.runId = s.runId;
        d.sourceId = s.sourceId;
        d.sequence = s.sequence;
        d.inputTime = s.inputTime;
        d.admittedH = s.admittedH;
        d.admittedL = s.admittedL;
        d.cumulativeCount = s.cumulativeCount;
        d.admissionTransactionId = s.admissionTransactionId;
    }
    return copiedBytes;
}
