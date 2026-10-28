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
export function copyDerivedPayloadV2(from: WaterSandSnapshot, to: WaterSandSnapshot): number {
    if (from.cols !== 128 || to.cols !== 128 || from.capacity !== 256 || to.capacity !== 256)
        throw new Error('DERIVED_PAYLOAD_SHAPE');
    to.schemaVersion = from.schemaVersion;
    to.mediumMode = from.mediumMode;
    to.airDynamicsRevision = from.airDynamicsRevision;
    to.birthPlacementRevision = from.birthPlacementRevision;
    to.drySourceId = from.drySourceId;
    to.dryLastSequence = from.dryLastSequence;
    to.dryLastInputTime = from.dryLastInputTime;
    to.physicsCommitId = from.physicsCommitId;
    to.waterTop.set(from.waterTop);
    to.bh.set(from.bh);
    to.bl.set(from.bl);
    to.mh.set(from.mh);
    to.ml.set(from.ml);
    to.suspH.set(from.suspH);
    to.suspL.set(from.suspL);
    to.hs.set(from.hs);
    to.hl.set(from.hl);
    to.sandTop.set(from.sandTop);
    if (to.shakeSourceH !== undefined && from.shakeSourceH !== undefined)
        to.shakeSourceH.set(from.shakeSourceH);
    if (to.shakeSourceL !== undefined && from.shakeSourceL !== undefined)
        to.shakeSourceL.set(from.shakeSourceL);
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
    b.pMass.set(a.pMass);
    b.pX.set(a.pX);
    b.pY.set(a.pY);
    b.pVx.set(a.pVx);
    b.pVy.set(a.pVy);
    b.pPhase.set(a.pPhase);
    b.pSource.set(a.pSource);
    b.pState.set(a.pState);
    b.pId.set(a.pId);
    b.pSourceId.set(a.pSourceId);
    b.freeSlots.set(a.freeSlots);
    b.colParticleMassH.set(a.colParticleMassH);
    b.colParticleMassL.set(a.colParticleMassL);
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
    return 13 * 128 * 8 + 55 * a.nmax + 16 * 128;
}
