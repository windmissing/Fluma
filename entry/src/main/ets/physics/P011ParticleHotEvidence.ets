// P-011 D1-R3 §7 COLD export only. All returned arrays/objects are owned copies.
import { P011ParticleHotWorkspace, P011ParticleHotFieldBuffer } from './P011ParticleHot';
import { P011ParticleSnapshot, P011ParticlePlan, P011ParticleRequest, P011ParticleOp,
  P011ParticleMotion, P011ParticleApplyResult, P011ParticleGuardDecision } from './P011ParticleField';
import { P011ParticlePrototypeEvent } from './P011ParticlePrototype';

export interface P011ParticleHotEvidence {
  ownerId: number; pageLease: number; stepEpoch: number; attemptToken: number;
  plan: P011ParticlePlan; results: P011ParticleApplyResult[]; events: P011ParticlePrototypeEvent[];
  pool: P011ParticleSnapshot; field: P011ParticleHotFieldBuffer;
  eventLevelValidationOwnership: 'ENDPOINT_CHECKER';
  transactionLayerEventValidationRole: 'EMIT_AND_ATOMIC_COMMIT_ONLY';
}
export function registerParticleHotRun(work: P011ParticleHotWorkspace, runToken: number, runId: string): void {
  if (work.active !== 0 || work.pageBacklog !== 0 || !Number.isSafeInteger(runToken) || runToken <= 0 || !runId) {
    throw new Error('PARTICLE_HOT_RUN_REGISTRATION_INVALID');
  }
  work.coldRunToken = runToken; work.coldRunId = runId;
}
function operation(kind: number): P011ParticleOp {
  if (kind === 1) return 'spawn'; if (kind === 2) return 'deposit'; if (kind === 3) return 'crossing';
  if (kind === 4) return 'lift'; if (kind === 5) return 'degrade'; return 'merge';
}
function snapshot(s: P011ParticleSnapshot): P011ParticleSnapshot {
  return { nmax:s.nmax, cols:s.cols, liveCount:s.liveCount, freeCount:s.freeCount, nextId:s.nextId,
    pMass:s.pMass.slice(), pX:s.pX.slice(), pY:s.pY.slice(), pVx:s.pVx.slice(), pVy:s.pVy.slice(),
    pPhase:s.pPhase.slice(), pSource:s.pSource.slice(), pState:s.pState.slice(), pId:s.pId.slice(),
    pSourceId:s.pSourceId.slice(), freeSlots:s.freeSlots.slice(),
    colParticleMassH:s.colParticleMassH.slice(), colParticleMassL:s.colParticleMassL.slice() };
}
export function exportParticleEvidence(work: P011ParticleHotWorkspace, committedPageToken: number): P011ParticleHotEvidence {
  if (work.active !== 0 || work.pageBacklog !== 1 || committedPageToken <= 0 ||
    !Number.isSafeInteger(committedPageToken) || committedPageToken !== work.committedPageToken || !work.storageValidHot()) {
    throw new Error('PARTICLE_EVIDENCE_STALE_PAGE');
  }
  const p = work.sealedPlan, r = work.sealedResults, e = work.events;
  if (!work.coldRunId || work.coldRunToken !== p.runToken) throw new Error('PARTICLE_EVIDENCE_RUN_ID_MISSING');
  const keys: string[] = new Array<string>(work.requestCapacity + 1); keys.fill('');
  const requests: P011ParticleRequest[] = []; const motions: P011ParticleMotion[] = [];
  for (let i = 0; i < p.requestCount; i++) {
    const kind = operation(p.kind[i]); const lineage: number[] = [];
    for (let j = 0; j < p.mergeCount[i]; j++) lineage.push(p.lineage[p.mergeStart[i] + j]);
    const key = JSON.stringify([work.coldRunId, p.substepId, kind, p.particleId[i] || p.sourceId[i],
      p.column[i], p.targetColumn[i], p.ordinal[i]]);
    keys[p.ordinal[i]] = key;
    requests.push({requestKey:key, substepId:p.substepId, ordinal:p.ordinal[i], kind:kind,
      particleId:p.particleId[i], sourceId:p.sourceId[i], phase:p.phase[i], sourceKind:p.sourceKind[i],
      column:p.column[i], targetColumn:p.targetColumn[i], requested:p.requested[i], totalWeight:p.totalWeight[i],
      predictedX:p.predictedX[i], predictedY:p.predictedY[i], predictedVx:p.predictedVx[i], predictedVy:p.predictedVy[i], mergedIds:lineage});
  }
  for (let i = 0; i < p.motionCount; i++) motions.push({particleId:p.motionParticleId[i], column:p.motionColumn[i],
    x:p.motionX[i], y:p.motionY[i], vx:p.motionVx[i], vy:p.motionVy[i]});
  const events: P011ParticlePrototypeEvent[] = [];
  const guards: P011ParticleGuardDecision[] = [];
  for (let i = 0; i < e.count; i++) {
    let guard: P011ParticleGuardDecision | null = null;
    if (e.guardCount[i] > 0) {
      const edges: number[] = []; for (let j = 0; j < e.guardCount[i]; j++) edges.push(e.guardEdgeId[i * 3 + j]);
      guard = {accepted:e.guardAccepted[i], rejected:e.guardRejectedAmount[i], allLegalBefore:e.guardAllLegal[i] === 1,
        limitingEdge:e.guardLimitingEdge[i] < 0 ? null : e.guardLimitingEdge[i], affectedEdgeIds:edges};
    }
    // Result rows index the immutable event carrying their own guard evidence.
    if (guard !== null) guards[i] = guard;
    const ids: number[] = []; for (let j = 0; j < e.lineageLength[i]; j++) ids.push(e.lineage[e.lineageStart[i] + j]);
    const kind = e.kind[i] === 1 ? 'SPAWN' : e.kind[i] === 2 ? 'LAND' : e.kind[i] === 3 ? 'CROSS_COLUMN' :
      e.kind[i] === 4 ? 'LIFT' : e.kind[i] === 5 ? 'DEGRADE' : 'MERGE';
    const event: P011ParticlePrototypeEvent = {kind:kind, requestKey:keys[e.requestOrdinal[i]], particleId:e.particleId[i],
      sourceId:e.sourceId[i], phase:e.phase[i], sourceKind:e.sourceKind[i], column:e.column[i], targetColumn:e.targetColumn[i],
      mass:e.massBefore[i], requested:e.requested[i], granted:e.granted[i], remainder:e.remainder[i],
      budgetClipped:e.budgetClipped[i], guardRejected:e.guardRejected[i], capacityClipped:e.capacityClipped[i],
      poolClipped:e.poolClipped[i], deltaS:e.deltaS[i], mergedIds:ids, flag:e.flag[i] === 1 ? 'PARTICLE_DEGRADED_TO_COLUMN' : '',
      substepId:p.substepId, eventLevelValidationOwnership:'ENDPOINT_CHECKER',
      transactionLayerEventValidationRole:'EMIT_AND_ATOMIC_COMMIT_ONLY'};
    if (kind === 'SPAWN') {event.admitted = e.granted[i]; event.x = e.x[i]; event.y = e.y[i]; event.vx = e.vx[i]; event.vy = e.vy[i];}
    else if (kind === 'MERGE') {event.massBefore = e.massBefore[i]; event.massAfter = e.massAfter[i]; event.positionMomentPreserved = true;}
    else {
      event.massAfter = e.massAfter[i]; event.released = e.remainder[i] === 0;
      event.remainderDisposition = e.remainderDisposition[i] === 1 ? 'RETAIN_PARTICLE' : 'NONE';
      event.guardPreview = guard; event.budgetLocal = e.budgetLocal[i]; event.budgetTotal = e.budgetTotal[i]; event.bottom = 0;
    }
    events.push(event);
  }
  const results: P011ParticleApplyResult[] = [];
  for (let i = 0; i < r.resultCount; i++) results.push({requestKey:keys[p.ordinal[i]], committed:r.committed[i] === 1,
    errorCode:null, requested:p.requested[i], granted:r.granted[i], remainder:r.remainder[i], budgetClipped:r.budgetClipped[i],
    guardRejected:r.guardRejected[i], capacityClipped:r.capacityClipped[i], poolClipped:r.poolClipped[i],
    guard:r.guardIndex[i] < 0 ? null : guards[r.guardIndex[i]], eventBegin:r.eventBegin[i], eventCount:r.eventCount[i]});
  const plan: P011ParticlePlan = {identity:{runId:work.coldRunId, substepId:p.substepId, dt:p.dt, effectiveTiltRad:p.effectiveTiltRad,
    seed:p.seed, paramsRevision:p.paramsRevision, inputRevision:p.inputRevision}, requestCount:p.requestCount,
    requests:requests, totalRequested:p.totalRequested, requestHash:JSON.stringify(requests), motions:motions, motionHash:JSON.stringify(motions)};
  const field: P011ParticleHotFieldBuffer = {cols:work.cols, capacity:work.capacity, waterTop:work.fieldBeforeWaterTop.slice(),
    bh:work.fieldBeforeBh.slice(), bl:work.fieldBeforeBl.slice(), mh:work.fieldBeforeMh.slice(), ml:work.fieldBeforeMl.slice(),
    suspH:work.fieldBeforeSuspH.slice(), suspL:work.fieldBeforeSuspL.slice(), surface:work.fieldBeforeSurface.slice(),
    sourceInjectedH:work.pageSourceH, sourceInjectedL:work.pageSourceL, sourceEpoch:work.pageSourceEpoch};
  const evidence: P011ParticleHotEvidence = {ownerId:work.ownerId, pageLease:committedPageToken, stepEpoch:p.stepEpoch,
    attemptToken:p.attemptToken, plan:plan, results:results, events:events, pool:snapshot(work.patchAfter), field:field,
    eventLevelValidationOwnership:'ENDPOINT_CHECKER', transactionLayerEventValidationRole:'EMIT_AND_ATOMIC_COMMIT_ONLY'};
  if (committedPageToken !== work.committedPageToken) throw new Error('PARTICLE_EVIDENCE_STALE_PAGE');
  work.pageBacklog = 0; work.committedPageToken = 0; return evidence;
}
