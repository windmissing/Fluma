// P-011 D1-R3 S0/S1: fixed-capacity HOT workspace and pure dual collect.
// This module deliberately has no renderer or WaterSandField dependency.
import { P011ParticleField, P011ParticleParams, P011ParticleSnapshot, particleSnapshotBufferAtHot } from './P011ParticleField';
import { P011ParticleHotBudgetAdapter } from './P011ParticleHotBudget';

export const P011_HOT_OK = 0;
export const P011_HOT_NOT_CONFIGURED = 1;
export const P011_HOT_INVALID_INPUT = 2;
export const P011_HOT_INVALID_BUFFER = 3;
export const P011_HOT_STALE_HANDLE = 4;
export const P011_HOT_INVALID_PATCH_STATE = 5;
export const P011_HOT_PATCH_BUSY = 6;
export const P011_HOT_PLAN_OVERFLOW = 7;
export const P011_HOT_MOTION_OVERFLOW = 8;
export const P011_HOT_JOURNAL_OVERFLOW = 9;
export const P011_HOT_LINEAGE_OVERFLOW = 10;
export const P011_HOT_TICKET_OVERFLOW = 11;
export const P011_HOT_PLAN_MISMATCH = 12;
export const P011_HOT_AUDIT_FAILED = 13;
export const P011_HOT_TOKEN_EXHAUSTED = 14;
export const P011_HOT_DUPLICATE_REQUEST = 15;
export const P011_HOT_IDENTITY_MISMATCH = 16;
export const P011_HOT_UNKNOWN_PARTICLE = 17;
export const P011_HOT_STEP_BUSY = 18;

let nextOwnerId = 1;
let nextLeaseToken = 1;

export interface P011ParticleHotConfig {
  cols: number; nmax: number; requestCapacity: number; motionCapacity: number;
  eventCapacity: number; lineageCapacity: number; params: P011ParticleParams;
}

export interface P011ParticleHotFieldBuffer {
  mediumMode?: string;
  readonly cols: number; readonly capacity: number;
  readonly waterTop: Float64Array; readonly bh: Float64Array; readonly bl: Float64Array;
  readonly mh: Float64Array; readonly ml: Float64Array;
  readonly suspH: Float64Array; readonly suspL: Float64Array; readonly surface: Float64Array;
  sourceInjectedH: number; sourceInjectedL: number; sourceEpoch: number;
}
export interface P011ParticleHotStepInput {
  runToken: number; substepId: number; attemptToken: number; dt: number; effectiveTiltRad: number;
  seed: number; paramsRevision: number; inputRevision: number; field: P011ParticleHotFieldBuffer;
}
export interface P011ParticleHotCommitSink {
  applyInto(work: P011ParticleHotWorkspace, expectedStepEpoch: number, plan: P011ParticleHotPlanBuffer,
    row: number, results: P011ParticleHotResultBuffer): number;
}
export interface P011ParticleHotSinkCheckpoint {
  saveOutsHot(): void;
  restoreOutsHot(): void;
  buffersValidHot(): boolean;
}

export class P011ParticleHotPlanBuffer {
  readonly requestCapacity: number; readonly motionCapacity: number; readonly lineageCapacity: number;
  ownerId = 0; runToken = 0; stepEpoch = 0; substepId = 0; attemptToken = 0;
  paramsRevision = 0; inputRevision = 0; seed = 0; dt = 0; effectiveTiltRad = 0;
  requestCount = 0; motionCount = 0; totalRequested = 0; valid = 0; consumed = 0;
  readonly kind: Uint8Array; readonly phase: Uint8Array; readonly sourceKind: Uint8Array;
  readonly column: Int32Array; readonly targetColumn: Int32Array;
  readonly particleId: Uint32Array; readonly sourceId: Uint32Array;
  readonly substep: Float64Array; readonly ordinal: Float64Array; readonly requested: Float64Array;
  readonly totalWeight: Float64Array; readonly predictedX: Float64Array; readonly predictedY: Float64Array;
  readonly predictedVx: Float64Array; readonly predictedVy: Float64Array;
  readonly mergeStart: Int32Array; readonly mergeCount: Int32Array;
  readonly motionParticleId: Uint32Array; readonly motionColumn: Int32Array;
  readonly motionX: Float64Array; readonly motionY: Float64Array;
  readonly motionVx: Float64Array; readonly motionVy: Float64Array;
  readonly lineage: Uint32Array;
  private readonly storage: ArrayBuffer[];
  constructor(requestCapacity: number, motionCapacity: number, lineageCapacity: number) {
    this.requestCapacity = requestCapacity; this.motionCapacity = motionCapacity;
    this.lineageCapacity = lineageCapacity;
    this.kind = new Uint8Array(requestCapacity); this.phase = new Uint8Array(requestCapacity);
    this.sourceKind = new Uint8Array(requestCapacity); this.column = new Int32Array(requestCapacity);
    this.targetColumn = new Int32Array(requestCapacity); this.particleId = new Uint32Array(requestCapacity);
    this.sourceId = new Uint32Array(requestCapacity); this.substep = new Float64Array(requestCapacity);
    this.ordinal = new Float64Array(requestCapacity); this.requested = new Float64Array(requestCapacity);
    this.totalWeight = new Float64Array(requestCapacity); this.predictedX = new Float64Array(requestCapacity);
    this.predictedY = new Float64Array(requestCapacity); this.predictedVx = new Float64Array(requestCapacity);
    this.predictedVy = new Float64Array(requestCapacity); this.mergeStart = new Int32Array(requestCapacity);
    this.mergeCount = new Int32Array(requestCapacity);
    this.motionParticleId = new Uint32Array(motionCapacity); this.motionColumn = new Int32Array(motionCapacity);
    this.motionX = new Float64Array(motionCapacity); this.motionY = new Float64Array(motionCapacity);
    this.motionVx = new Float64Array(motionCapacity); this.motionVy = new Float64Array(motionCapacity);
    this.lineage = new Uint32Array(lineageCapacity);
    this.storage = [this.kind.buffer, this.phase.buffer, this.sourceKind.buffer, this.column.buffer, this.targetColumn.buffer, this.particleId.buffer, this.sourceId.buffer, this.substep.buffer, this.ordinal.buffer, this.requested.buffer, this.totalWeight.buffer, this.predictedX.buffer, this.predictedY.buffer, this.predictedVx.buffer, this.predictedVy.buffer, this.mergeStart.buffer, this.mergeCount.buffer, this.motionParticleId.buffer, this.motionColumn.buffer, this.motionX.buffer, this.motionY.buffer, this.motionVx.buffer, this.motionVy.buffer, this.lineage.buffer];
  }
  storageValidHot(): boolean {
    return this.kind.buffer === this.storage[0] && this.kind.byteOffset === 0 && this.kind.length === this.requestCapacity &&
      this.phase.buffer === this.storage[1] && this.phase.byteOffset === 0 && this.phase.length === this.requestCapacity &&
      this.sourceKind.buffer === this.storage[2] && this.sourceKind.byteOffset === 0 && this.sourceKind.length === this.requestCapacity &&
      this.column.buffer === this.storage[3] && this.column.byteOffset === 0 && this.column.length === this.requestCapacity &&
      this.targetColumn.buffer === this.storage[4] && this.targetColumn.byteOffset === 0 && this.targetColumn.length === this.requestCapacity &&
      this.particleId.buffer === this.storage[5] && this.particleId.byteOffset === 0 && this.particleId.length === this.requestCapacity &&
      this.sourceId.buffer === this.storage[6] && this.sourceId.byteOffset === 0 && this.sourceId.length === this.requestCapacity &&
      this.substep.buffer === this.storage[7] && this.substep.byteOffset === 0 && this.substep.length === this.requestCapacity &&
      this.ordinal.buffer === this.storage[8] && this.ordinal.byteOffset === 0 && this.ordinal.length === this.requestCapacity &&
      this.requested.buffer === this.storage[9] && this.requested.byteOffset === 0 && this.requested.length === this.requestCapacity &&
      this.totalWeight.buffer === this.storage[10] && this.totalWeight.byteOffset === 0 && this.totalWeight.length === this.requestCapacity &&
      this.predictedX.buffer === this.storage[11] && this.predictedX.byteOffset === 0 && this.predictedX.length === this.requestCapacity &&
      this.predictedY.buffer === this.storage[12] && this.predictedY.byteOffset === 0 && this.predictedY.length === this.requestCapacity &&
      this.predictedVx.buffer === this.storage[13] && this.predictedVx.byteOffset === 0 && this.predictedVx.length === this.requestCapacity &&
      this.predictedVy.buffer === this.storage[14] && this.predictedVy.byteOffset === 0 && this.predictedVy.length === this.requestCapacity &&
      this.mergeStart.buffer === this.storage[15] && this.mergeStart.byteOffset === 0 && this.mergeStart.length === this.requestCapacity &&
      this.mergeCount.buffer === this.storage[16] && this.mergeCount.byteOffset === 0 && this.mergeCount.length === this.requestCapacity &&
      this.motionParticleId.buffer === this.storage[17] && this.motionParticleId.byteOffset === 0 && this.motionParticleId.length === this.motionCapacity &&
      this.motionColumn.buffer === this.storage[18] && this.motionColumn.byteOffset === 0 && this.motionColumn.length === this.motionCapacity &&
      this.motionX.buffer === this.storage[19] && this.motionX.byteOffset === 0 && this.motionX.length === this.motionCapacity &&
      this.motionY.buffer === this.storage[20] && this.motionY.byteOffset === 0 && this.motionY.length === this.motionCapacity &&
      this.motionVx.buffer === this.storage[21] && this.motionVx.byteOffset === 0 && this.motionVx.length === this.motionCapacity &&
      this.motionVy.buffer === this.storage[22] && this.motionVy.byteOffset === 0 && this.motionVy.length === this.motionCapacity &&
      this.lineage.buffer === this.storage[23] && this.lineage.byteOffset === 0 && this.lineage.length === this.lineageCapacity;
  }
  coldBuffers(): ArrayBuffer[] { return this.storage; }
}

export class P011ParticleHotResultBuffer {
  readonly capacity: number; private readonly storage: ArrayBuffer[];
  ownerId = 0; stepEpoch = 0; resultCount = 0; status = P011_HOT_OK;
  readonly committed: Uint8Array; readonly disposition: Uint8Array; readonly released: Uint8Array;
  readonly rowStatus: Int32Array; readonly guardIndex: Int32Array;
  readonly eventBegin: Int32Array; readonly eventCount: Int32Array;
  readonly granted: Float64Array; readonly remainder: Float64Array;
  readonly budgetClipped: Float64Array; readonly guardRejected: Float64Array;
  readonly capacityClipped: Float64Array; readonly poolClipped: Float64Array; readonly massAfter: Float64Array;
  constructor(capacity: number) {
    this.capacity = capacity;
    this.committed = new Uint8Array(capacity); this.disposition = new Uint8Array(capacity);
    this.released = new Uint8Array(capacity); this.rowStatus = new Int32Array(capacity);
    this.guardIndex = new Int32Array(capacity); this.eventBegin = new Int32Array(capacity);
    this.eventCount = new Int32Array(capacity); this.granted = new Float64Array(capacity);
    this.remainder = new Float64Array(capacity); this.budgetClipped = new Float64Array(capacity);
    this.guardRejected = new Float64Array(capacity); this.capacityClipped = new Float64Array(capacity);
    this.poolClipped = new Float64Array(capacity); this.massAfter = new Float64Array(capacity);
    this.storage = [this.committed.buffer, this.disposition.buffer, this.released.buffer, this.rowStatus.buffer, this.guardIndex.buffer, this.eventBegin.buffer, this.eventCount.buffer, this.granted.buffer, this.remainder.buffer, this.budgetClipped.buffer, this.guardRejected.buffer, this.capacityClipped.buffer, this.poolClipped.buffer, this.massAfter.buffer];
  }
  storageValidHot(): boolean {
    return this.committed.buffer === this.storage[0] && this.committed.length === this.capacity &&
      this.disposition.buffer === this.storage[1] && this.disposition.length === this.capacity &&
      this.released.buffer === this.storage[2] && this.released.length === this.capacity &&
      this.rowStatus.buffer === this.storage[3] && this.rowStatus.length === this.capacity &&
      this.guardIndex.buffer === this.storage[4] && this.guardIndex.length === this.capacity &&
      this.eventBegin.buffer === this.storage[5] && this.eventBegin.length === this.capacity &&
      this.eventCount.buffer === this.storage[6] && this.eventCount.length === this.capacity &&
      this.granted.buffer === this.storage[7] && this.granted.length === this.capacity &&
      this.remainder.buffer === this.storage[8] && this.remainder.length === this.capacity &&
      this.budgetClipped.buffer === this.storage[9] && this.budgetClipped.length === this.capacity &&
      this.guardRejected.buffer === this.storage[10] && this.guardRejected.length === this.capacity &&
      this.capacityClipped.buffer === this.storage[11] && this.capacityClipped.length === this.capacity &&
      this.poolClipped.buffer === this.storage[12] && this.poolClipped.length === this.capacity &&
      this.massAfter.buffer === this.storage[13] && this.massAfter.length === this.capacity;
  }
  coldBuffers(): ArrayBuffer[] { return this.storage; }
}

// Fixed action/event rows. Births and merges are staged here before publish.
export class P011ParticleHotEventBuffer {
  private readonly storage: ArrayBuffer[];
  readonly capacity: number; readonly lineageCapacity: number;
  count = 0; lineageCount = 0;
  ownerId = 0; runToken = 0; stepEpoch = 0; substepId = 0; attemptToken = 0;
  readonly kind: Uint8Array; readonly phase: Uint8Array; readonly sourceKind: Uint8Array;
  readonly flag: Uint8Array; readonly remainderDisposition: Uint8Array;
  readonly particleId: Uint32Array; readonly sourceId: Uint32Array;
  readonly requestOrdinal: Int32Array; readonly column: Int32Array; readonly targetColumn: Int32Array;
  readonly lineageStart: Int32Array; readonly lineageLength: Int32Array; readonly lineage: Uint32Array;
  readonly requested: Float64Array; readonly granted: Float64Array; readonly remainder: Float64Array;
  readonly massBefore: Float64Array; readonly massAfter: Float64Array;
  readonly x: Float64Array; readonly y: Float64Array; readonly vx: Float64Array; readonly vy: Float64Array;
  readonly deltaS: Float64Array; readonly budgetLocal: Float64Array; readonly budgetTotal: Float64Array;
  readonly budgetBottom: Float64Array;
  readonly budgetClipped: Float64Array; readonly guardRejected: Float64Array;
  readonly capacityClipped: Float64Array; readonly poolClipped: Float64Array;
  readonly toSuspH: Float64Array; readonly toSuspL: Float64Array;
  readonly guardCount: Int32Array; readonly guardLimitingEdge: Int32Array; readonly guardAllLegal: Uint8Array;
  readonly guardAccepted: Float64Array; readonly guardRejectedAmount: Float64Array;
  readonly guardEdgeId: Int32Array; readonly guardResidual: Float64Array; readonly guardInfluence: Float64Array;
  constructor(capacity: number, lineageCapacity: number) {
    this.capacity = capacity; this.lineageCapacity = lineageCapacity;
    this.kind = new Uint8Array(capacity); this.phase = new Uint8Array(capacity); this.sourceKind = new Uint8Array(capacity);
    this.flag = new Uint8Array(capacity); this.remainderDisposition = new Uint8Array(capacity);
    this.particleId = new Uint32Array(capacity); this.sourceId = new Uint32Array(capacity);
    this.requestOrdinal = new Int32Array(capacity); this.column = new Int32Array(capacity);
    this.targetColumn = new Int32Array(capacity); this.lineageStart = new Int32Array(capacity);
    this.lineageLength = new Int32Array(capacity); this.lineage = new Uint32Array(lineageCapacity);
    this.requested = new Float64Array(capacity); this.granted = new Float64Array(capacity);
    this.remainder = new Float64Array(capacity); this.massBefore = new Float64Array(capacity);
    this.massAfter = new Float64Array(capacity); this.x = new Float64Array(capacity); this.y = new Float64Array(capacity);
    this.vx = new Float64Array(capacity); this.vy = new Float64Array(capacity); this.deltaS = new Float64Array(capacity);
    this.budgetLocal = new Float64Array(capacity); this.budgetTotal = new Float64Array(capacity);
    this.budgetBottom = new Float64Array(capacity);
    this.budgetClipped = new Float64Array(capacity); this.guardRejected = new Float64Array(capacity);
    this.capacityClipped = new Float64Array(capacity); this.poolClipped = new Float64Array(capacity);
    this.toSuspH = new Float64Array(capacity); this.toSuspL = new Float64Array(capacity);
    this.guardCount = new Int32Array(capacity); this.guardLimitingEdge = new Int32Array(capacity);
    this.guardAllLegal = new Uint8Array(capacity); this.guardAccepted = new Float64Array(capacity);
    this.guardRejectedAmount = new Float64Array(capacity); this.guardEdgeId = new Int32Array(3 * capacity);
    this.guardResidual = new Float64Array(3 * capacity); this.guardInfluence = new Float64Array(3 * capacity);
    this.storage = [this.kind.buffer, this.phase.buffer, this.sourceKind.buffer, this.flag.buffer, this.remainderDisposition.buffer, this.particleId.buffer, this.sourceId.buffer, this.requestOrdinal.buffer, this.column.buffer, this.targetColumn.buffer, this.lineageStart.buffer, this.lineageLength.buffer, this.lineage.buffer, this.requested.buffer, this.granted.buffer, this.remainder.buffer, this.massBefore.buffer, this.massAfter.buffer, this.x.buffer, this.y.buffer, this.vx.buffer, this.vy.buffer, this.deltaS.buffer, this.budgetLocal.buffer, this.budgetTotal.buffer, this.budgetBottom.buffer, this.budgetClipped.buffer, this.guardRejected.buffer, this.capacityClipped.buffer, this.poolClipped.buffer, this.toSuspH.buffer, this.toSuspL.buffer, this.guardCount.buffer, this.guardLimitingEdge.buffer, this.guardAllLegal.buffer, this.guardAccepted.buffer, this.guardRejectedAmount.buffer, this.guardEdgeId.buffer, this.guardResidual.buffer, this.guardInfluence.buffer];
  }
  coldBuffers(): ArrayBuffer[] { return this.storage; }
  storageValidHot(): boolean {
    return this.kind.buffer === this.storage[0] && this.kind.length === this.capacity &&
      this.phase.buffer === this.storage[1] && this.phase.length === this.capacity &&
      this.sourceKind.buffer === this.storage[2] && this.sourceKind.length === this.capacity &&
      this.flag.buffer === this.storage[3] && this.flag.length === this.capacity &&
      this.remainderDisposition.buffer === this.storage[4] && this.remainderDisposition.length === this.capacity &&
      this.particleId.buffer === this.storage[5] && this.particleId.length === this.capacity &&
      this.sourceId.buffer === this.storage[6] && this.sourceId.length === this.capacity &&
      this.requestOrdinal.buffer === this.storage[7] && this.requestOrdinal.length === this.capacity &&
      this.column.buffer === this.storage[8] && this.column.length === this.capacity &&
      this.targetColumn.buffer === this.storage[9] && this.targetColumn.length === this.capacity &&
      this.lineageStart.buffer === this.storage[10] && this.lineageStart.length === this.capacity &&
      this.lineageLength.buffer === this.storage[11] && this.lineageLength.length === this.capacity &&
      this.lineage.buffer === this.storage[12] && this.lineage.length === this.lineageCapacity &&
      this.requested.buffer === this.storage[13] && this.requested.length === this.capacity &&
      this.granted.buffer === this.storage[14] && this.granted.length === this.capacity &&
      this.remainder.buffer === this.storage[15] && this.remainder.length === this.capacity &&
      this.massBefore.buffer === this.storage[16] && this.massBefore.length === this.capacity &&
      this.massAfter.buffer === this.storage[17] && this.massAfter.length === this.capacity &&
      this.x.buffer === this.storage[18] && this.x.length === this.capacity &&
      this.y.buffer === this.storage[19] && this.y.length === this.capacity &&
      this.vx.buffer === this.storage[20] && this.vx.length === this.capacity &&
      this.vy.buffer === this.storage[21] && this.vy.length === this.capacity &&
      this.deltaS.buffer === this.storage[22] && this.deltaS.length === this.capacity &&
      this.budgetLocal.buffer === this.storage[23] && this.budgetLocal.length === this.capacity &&
      this.budgetTotal.buffer === this.storage[24] && this.budgetTotal.length === this.capacity &&
      this.budgetBottom.buffer === this.storage[25] && this.budgetBottom.length === this.capacity &&
      this.budgetClipped.buffer === this.storage[26] && this.budgetClipped.length === this.capacity &&
      this.guardRejected.buffer === this.storage[27] && this.guardRejected.length === this.capacity &&
      this.capacityClipped.buffer === this.storage[28] && this.capacityClipped.length === this.capacity &&
      this.poolClipped.buffer === this.storage[29] && this.poolClipped.length === this.capacity &&
      this.toSuspH.buffer === this.storage[30] && this.toSuspH.length === this.capacity &&
      this.toSuspL.buffer === this.storage[31] && this.toSuspL.length === this.capacity &&
      this.guardCount.buffer === this.storage[32] && this.guardCount.length === this.capacity &&
      this.guardLimitingEdge.buffer === this.storage[33] && this.guardLimitingEdge.length === this.capacity &&
      this.guardAllLegal.buffer === this.storage[34] && this.guardAllLegal.length === this.capacity &&
      this.guardAccepted.buffer === this.storage[35] && this.guardAccepted.length === this.capacity &&
      this.guardRejectedAmount.buffer === this.storage[36] && this.guardRejectedAmount.length === this.capacity &&
      this.guardEdgeId.buffer === this.storage[37] && this.guardEdgeId.length === 3 * this.capacity &&
      this.guardResidual.buffer === this.storage[38] && this.guardResidual.length === 3 * this.capacity &&
      this.guardInfluence.buffer === this.storage[39] && this.guardInfluence.length === 3 * this.capacity;
  }
}

export class P011ParticleHotWorkspace {
  mediumMode: string = 'WET_COLUMN_V1';
  private readonly bufferBindings: ArrayBuffer[]; private readonly bufferBytes: number[];
  readonly fieldBinding: ArrayBuffer[];
  readonly ownerId: number; readonly pool: P011ParticleField; readonly params: P011ParticleParams;
  readonly planA: P011ParticleHotPlanBuffer; readonly planB: P011ParticleHotPlanBuffer;
  readonly results: P011ParticleHotResultBuffer;
  readonly frozenWaterTop: Float64Array; readonly frozenBh: Float64Array; readonly frozenBl: Float64Array;
  readonly frozenMh: Float64Array; readonly frozenMl: Float64Array;
  readonly frozenSuspH: Float64Array; readonly frozenSuspL: Float64Array; readonly frozenSurface: Float64Array;
  readonly order: Int32Array; readonly seenEpoch: Float64Array; readonly seenEpochBefore: Float64Array;
  readonly fieldBeforeWaterTop: Float64Array; readonly fieldBeforeBh: Float64Array;
  readonly fieldBeforeBl: Float64Array; readonly fieldBeforeMh: Float64Array; readonly fieldBeforeMl: Float64Array;
  readonly fieldBeforeSuspH: Float64Array; readonly fieldBeforeSuspL: Float64Array; readonly fieldBeforeSurface: Float64Array;
  readonly eventKind: Uint8Array; readonly eventParticleId: Uint32Array; readonly eventSourceId: Uint32Array;
  readonly eventColumn: Int32Array; readonly eventTargetColumn: Int32Array;
  readonly eventRequested: Float64Array; readonly eventGranted: Float64Array;
  readonly frozenPool: P011ParticleSnapshot;
  readonly patchBefore: P011ParticleSnapshot; readonly patchAfter: P011ParticleSnapshot;
  readonly patchScratch: P011ParticleSnapshot; readonly patchActions: P011ParticleHotEventBuffer;
  readonly seenRequests: P011ParticleHotPlanBuffer;
  readonly patchRequest: P011ParticleHotPlanBuffer;
  readonly patchOriginal: P011ParticleHotPlanBuffer;
  stagedYActive = 0; stagedY = 0;
  readonly events: P011ParticleHotEventBuffer;
  readonly sealedPlan: P011ParticleHotPlanBuffer; readonly sealedResults: P011ParticleHotResultBuffer;
  committedPageToken = 0;
  coldRunToken = 0; coldRunId = '';
  pageSourceH = 0; pageSourceL = 0; pageSourceEpoch = 0;
  readonly validationRows: Int32Array; readonly auditMassH: Float64Array; readonly auditMassL: Float64Array;
  readonly auditSlots: Uint8Array;
  budget: P011ParticleHotBudgetAdapter | null = null;
  commitSink: P011ParticleHotCommitSink | null = null;
  sinkCheckpoint: P011ParticleHotSinkCheckpoint | null = null;
  stepExternalH = 0; stepExternalL = 0;
  readonly hotContractVersion = 1;
  failureStatus = 0; failureOrdinal = 0; failureAttemptToken = 0; applying = 0;
  field: P011ParticleHotFieldBuffer | null = null;
  patchPlan: P011ParticleHotPlanBuffer | null = null; patchRow = -1;
  requestCapacity: number; motionCapacity: number; eventCapacity: number; lineageCapacity: number;
  cols: number; nmax: number; stepEpoch = 0; active = 0; configured = 1; lastStatus = P011_HOT_OK;
  capacity = 0;
  attemptToken = 0; runToken = 0; substepId = 0; paramsRevision = 0; inputRevision = 0;
  dt = 0; effectiveTiltRad = 0; seed = 0; sourceInjectedH = 0; sourceInjectedL = 0; sourceEpoch = 0;
  activePatchToken = 0; activePatchStepEpoch = 0; activePatchOrdinal = 0; patchState = 0;
  errorStageCode = 0; rollbackCount = 0; staleHandleRejectCount = 0; patchBusyCount = 0;
  overflowCount = 0; tokenExhausted = 0; bufferIdentityChanged = 0; legacyColdCallCount = 0; pageBacklog = 0;
  diagnosticEnabled = false; mergeCandidateChecks = 0; mergeCalls = 0; mergeSuccess = 0;
  birthRequested = 0; birthCompleted = 0; collectCalls = 0;
  diagnosticActiveCount = 0; diagnosticJournalUsed = 0; diagnosticLineageUsed = 0;
  diagnosticFreeBefore = 0; diagnosticLiveBefore = 0; diagnosticNextIdBefore = 0;
  diagnosticFreeAfter = 0; diagnosticLiveAfter = 0; diagnosticNextIdAfter = 0;
  diagnosticFirstError = 0; diagnosticFirstErrorStage = 0;
  constructor(config: P011ParticleHotConfig, pool: P011ParticleField) {
    this.ownerId = nextOwnerId++; this.pool = pool;
    // Cold owned copy: caller edits cannot change a frozen step's predictor.
    this.params = { massPackage: config.params.massPackage, nmax: config.params.nmax,
      terminalSpeedHeavy: config.params.terminalSpeedHeavy, terminalSpeedLight: config.params.terminalSpeedLight,
      dragHeavy: config.params.dragHeavy, dragLight: config.params.dragLight,
      liftSpeedHeavy: config.params.liftSpeedHeavy, liftSpeedLight: config.params.liftSpeedLight,
      liftSpread: config.params.liftSpread, mergeColRadius: config.params.mergeColRadius,
      mergeHeightRadius: config.params.mergeHeightRadius, contactEps: config.params.contactEps,
      channelRateCap: config.params.channelRateCap };
    this.cols = config.cols; this.nmax = config.nmax; this.requestCapacity = config.requestCapacity;
    this.motionCapacity = config.motionCapacity; this.eventCapacity = config.eventCapacity;
    this.lineageCapacity = config.lineageCapacity;
    this.planA = new P011ParticleHotPlanBuffer(config.requestCapacity, config.motionCapacity, config.lineageCapacity);
    this.planB = new P011ParticleHotPlanBuffer(config.requestCapacity, config.motionCapacity, config.lineageCapacity);
    this.results = new P011ParticleHotResultBuffer(config.requestCapacity);
    this.frozenWaterTop = new Float64Array(config.cols); this.frozenBh = new Float64Array(config.cols);
    this.frozenBl = new Float64Array(config.cols); this.frozenMh = new Float64Array(config.cols);
    this.frozenMl = new Float64Array(config.cols); this.frozenSuspH = new Float64Array(config.cols);
    this.frozenSuspL = new Float64Array(config.cols); this.frozenSurface = new Float64Array(config.cols);
    this.fieldBeforeWaterTop = new Float64Array(config.cols); this.fieldBeforeBh = new Float64Array(config.cols);
    this.fieldBeforeBl = new Float64Array(config.cols); this.fieldBeforeMh = new Float64Array(config.cols);
    this.fieldBeforeMl = new Float64Array(config.cols); this.fieldBeforeSuspH = new Float64Array(config.cols);
    this.fieldBeforeSuspL = new Float64Array(config.cols); this.fieldBeforeSurface = new Float64Array(config.cols);
    this.order = new Int32Array(config.nmax); this.seenEpoch = new Float64Array(config.requestCapacity + 1);
    this.seenEpochBefore = new Float64Array(config.requestCapacity + 1);
    this.eventKind = new Uint8Array(config.eventCapacity); this.eventParticleId = new Uint32Array(config.eventCapacity);
    this.eventSourceId = new Uint32Array(config.eventCapacity); this.eventColumn = new Int32Array(config.eventCapacity);
    this.eventTargetColumn = new Int32Array(config.eventCapacity);
    this.eventRequested = new Float64Array(config.eventCapacity); this.eventGranted = new Float64Array(config.eventCapacity);
    this.frozenPool = { nmax: config.nmax, cols: config.cols, liveCount: 0, freeCount: config.nmax, nextId: 1,
      pMass: new Float64Array(config.nmax), pX: new Float64Array(config.nmax), pY: new Float64Array(config.nmax),
      pVx: new Float64Array(config.nmax), pVy: new Float64Array(config.nmax), pPhase: new Uint8Array(config.nmax),
      pSource: new Uint8Array(config.nmax), pState: new Uint8Array(config.nmax), pId: new Uint32Array(config.nmax),
      pSourceId: new Uint32Array(config.nmax), freeSlots: new Int32Array(config.nmax),
      colParticleMassH: new Float64Array(config.cols), colParticleMassL: new Float64Array(config.cols) };
    const storage = pool.bindHotTransactionStorage();
    this.patchBefore = storage.before; this.patchAfter = storage.after; this.patchScratch = storage.scratch;
    this.patchActions = new P011ParticleHotEventBuffer(config.eventCapacity, config.lineageCapacity);
    this.seenRequests = new P011ParticleHotPlanBuffer(config.requestCapacity, 1, 2 * config.requestCapacity);
    this.patchRequest = new P011ParticleHotPlanBuffer(1, 1, 2);
    this.patchOriginal = new P011ParticleHotPlanBuffer(1, 1, 2);
    this.events = new P011ParticleHotEventBuffer(config.eventCapacity, config.lineageCapacity);
    this.sealedPlan = new P011ParticleHotPlanBuffer(config.requestCapacity, config.motionCapacity, config.lineageCapacity);
    this.sealedResults = new P011ParticleHotResultBuffer(config.requestCapacity);
    this.validationRows = new Int32Array(config.requestCapacity + 1);
    this.auditMassH = new Float64Array(config.cols); this.auditMassL = new Float64Array(config.cols);
    this.auditSlots = new Uint8Array(config.nmax);
    this.bufferBindings = []; this.bufferBytes = [];
    this.fieldBinding = [this.frozenWaterTop.buffer, this.frozenBh.buffer, this.frozenBl.buffer, this.frozenMh.buffer,
      this.frozenMl.buffer, this.frozenSuspH.buffer, this.frozenSuspL.buffer, this.frozenSurface.buffer];
    this.registerColdBuffers([this.frozenWaterTop.buffer, this.frozenBh.buffer, this.frozenBl.buffer, this.frozenMh.buffer, this.frozenMl.buffer, this.frozenSuspH.buffer, this.frozenSuspL.buffer, this.frozenSurface.buffer, this.order.buffer, this.seenEpoch.buffer, this.seenEpochBefore.buffer, this.fieldBeforeWaterTop.buffer, this.fieldBeforeBh.buffer, this.fieldBeforeBl.buffer, this.fieldBeforeMh.buffer, this.fieldBeforeMl.buffer, this.fieldBeforeSuspH.buffer, this.fieldBeforeSuspL.buffer, this.fieldBeforeSurface.buffer, this.eventKind.buffer, this.eventParticleId.buffer, this.eventSourceId.buffer, this.eventColumn.buffer, this.eventTargetColumn.buffer, this.eventRequested.buffer, this.eventGranted.buffer, this.validationRows.buffer, this.auditMassH.buffer, this.auditMassL.buffer, this.auditSlots.buffer]);
    for (let i = 0; i < 13; i++) this.registerColdBuffers([particleSnapshotBufferAtHot(this.frozenPool, i)]);
    for (let i = 0; i < 13; i++) this.registerColdBuffers([particleSnapshotBufferAtHot(this.patchBefore, i)]);
    for (let i = 0; i < 13; i++) this.registerColdBuffers([particleSnapshotBufferAtHot(this.patchAfter, i)]);
    for (let i = 0; i < 13; i++) this.registerColdBuffers([particleSnapshotBufferAtHot(this.patchScratch, i)]);
    this.registerColdBuffers(this.pool.hotCanonicalBuffers());
    this.registerColdBuffers(this.planA.coldBuffers()); this.registerColdBuffers(this.planB.coldBuffers());
    this.registerColdBuffers(this.seenRequests.coldBuffers()); this.registerColdBuffers(this.patchRequest.coldBuffers());
    this.registerColdBuffers(this.patchOriginal.coldBuffers()); this.registerColdBuffers(this.results.coldBuffers());
    this.registerColdBuffers(this.events.coldBuffers()); this.registerColdBuffers(this.patchActions.coldBuffers());
    this.registerColdBuffers(this.sealedPlan.coldBuffers()); this.registerColdBuffers(this.sealedResults.coldBuffers());
  }
  // COLD registration only. Dynamic shape and capacity never change in HOT.
  registerColdBuffers(buffers: ArrayBuffer[]): void {
    if (this.active !== 0) throw new Error('PARTICLE_HOT_REGISTRATION_BUSY');
    for (let i = 0; i < buffers.length; i++) {
      if (this.ownsBufferHot(buffers[i])) throw new Error('PARTICLE_HOT_BUFFER_ALIAS');
      this.bufferBindings.push(buffers[i]); this.bufferBytes.push(buffers[i].byteLength);
    }
  }
  ownsBufferHot(buffer: ArrayBuffer): boolean {
    for (let i = 0; i < this.bufferBindings.length; i++) if (buffer === this.bufferBindings[i]) return true;
    return false;
  }
  storageValidHot(): boolean {
    for (let i = 0; i < this.bufferBindings.length; i++) if (this.bufferBindings[i].byteLength !== this.bufferBytes[i]) return false;
    for (let i = 0; i < 13; i++) {
      if (particleSnapshotBufferAtHot(this.frozenPool, i) !== this.bufferBindings[30 + i] ||
        particleSnapshotBufferAtHot(this.patchBefore, i) !== this.bufferBindings[43 + i] ||
        particleSnapshotBufferAtHot(this.patchAfter, i) !== this.bufferBindings[56 + i] ||
        particleSnapshotBufferAtHot(this.patchScratch, i) !== this.bufferBindings[69 + i]) return false;
    }
    return this.frozenWaterTop.buffer === this.bufferBindings[0] &&
      this.frozenBh.buffer === this.bufferBindings[1] &&
      this.frozenBl.buffer === this.bufferBindings[2] &&
      this.frozenMh.buffer === this.bufferBindings[3] &&
      this.frozenMl.buffer === this.bufferBindings[4] &&
      this.frozenSuspH.buffer === this.bufferBindings[5] &&
      this.frozenSuspL.buffer === this.bufferBindings[6] &&
      this.frozenSurface.buffer === this.bufferBindings[7] &&
      this.order.buffer === this.bufferBindings[8] &&
      this.seenEpoch.buffer === this.bufferBindings[9] &&
      this.seenEpochBefore.buffer === this.bufferBindings[10] &&
      this.fieldBeforeWaterTop.buffer === this.bufferBindings[11] &&
      this.fieldBeforeBh.buffer === this.bufferBindings[12] &&
      this.fieldBeforeBl.buffer === this.bufferBindings[13] &&
      this.fieldBeforeMh.buffer === this.bufferBindings[14] &&
      this.fieldBeforeMl.buffer === this.bufferBindings[15] &&
      this.fieldBeforeSuspH.buffer === this.bufferBindings[16] &&
      this.fieldBeforeSuspL.buffer === this.bufferBindings[17] &&
      this.fieldBeforeSurface.buffer === this.bufferBindings[18] &&
      this.eventKind.buffer === this.bufferBindings[19] &&
      this.eventParticleId.buffer === this.bufferBindings[20] &&
      this.eventSourceId.buffer === this.bufferBindings[21] &&
      this.eventColumn.buffer === this.bufferBindings[22] &&
      this.eventTargetColumn.buffer === this.bufferBindings[23] &&
      this.eventRequested.buffer === this.bufferBindings[24] &&
      this.eventGranted.buffer === this.bufferBindings[25] &&
      this.validationRows.buffer === this.bufferBindings[26] &&
      this.auditMassH.buffer === this.bufferBindings[27] &&
      this.auditMassL.buffer === this.bufferBindings[28] &&
      this.auditSlots.buffer === this.bufferBindings[29] &&
      this.pool.storageValidForHot() && this.planA.storageValidHot() && this.planB.storageValidHot() &&
      this.seenRequests.storageValidHot() && this.patchRequest.storageValidHot() && this.patchOriginal.storageValidHot() &&
      this.results.storageValidHot() && this.events.storageValidHot() && this.patchActions.storageValidHot() &&
      this.sealedPlan.storageValidHot() && this.sealedResults.storageValidHot() &&
      (this.sinkCheckpoint === null || this.sinkCheckpoint.buffersValidHot());
  }
  get activePatchState(): number { return this.patchState; }
  get requestCount(): number { return this.planA.requestCount; }
  get motionCount(): number { return this.planA.motionCount; }
  get resultCount(): number { return this.results.resultCount; }
  get eventCount(): number { return this.events.count; }
  get lineageCount(): number { return this.events.lineageCount; }
  get hotStatus(): number { return this.lastStatus; }
  copyStateToHot(out: P011ParticleSnapshot): number {
    for (let i = 0; i < 13; i++) {
      const buffer = particleSnapshotBufferAtHot(out, i);
      if (this.ownsBufferHot(buffer)) return this.lastStatus = 3;
      if (this.field !== null) for (let j = 0; j < 8; j++) if (buffer === this.fieldBinding[j]) return this.lastStatus = 3;
    }
    if (out.nmax !== this.nmax || out.cols !== this.cols || out.pMass.length !== this.nmax ||
      out.pX.length !== this.nmax || out.pY.length !== this.nmax || out.pVx.length !== this.nmax ||
      out.pVy.length !== this.nmax || out.pPhase.length !== this.nmax || out.pSource.length !== this.nmax ||
      out.pState.length !== this.nmax || out.pId.length !== this.nmax || out.pSourceId.length !== this.nmax ||
      out.freeSlots.length !== this.nmax || out.colParticleMassH.length !== this.cols ||
      out.colParticleMassL.length !== this.cols) return this.lastStatus = P011_HOT_INVALID_BUFFER;
    return this.lastStatus = this.pool.transferHotState(out, false);
  }
}

function validCapacity(n: number): boolean { return Number.isSafeInteger(n) && n > 0; }
function fieldBufferAtHot(f: P011ParticleHotFieldBuffer, i: number): ArrayBuffer {
  switch (i) {
    case 0: return f.waterTop.buffer;
    case 1: return f.bh.buffer;
    case 2: return f.bl.buffer;
    case 3: return f.mh.buffer;
    case 4: return f.ml.buffer;
    case 5: return f.suspH.buffer;
    case 6: return f.suspL.buffer;
    default: return f.surface.buffer;
  }
}
function fieldStorageValidHot(work: P011ParticleHotWorkspace, f: P011ParticleHotFieldBuffer, bound: boolean): boolean {
  if(bound && (f.mediumMode ?? 'WET_COLUMN_V1')!==work.mediumMode)return false;
  for (let i = 0; i < 8; i++) {
    const buffer = fieldBufferAtHot(f, i);
    if (buffer.byteLength !== work.cols * 8 || work.ownsBufferHot(buffer) || (bound && buffer !== work.fieldBinding[i])) return false;
    for (let j = 0; j < i; j++) if (buffer === fieldBufferAtHot(f, j)) return false;
  }
  return true;
}
function validParams(p: P011ParticleParams): boolean {
  return Number.isSafeInteger(p.nmax) && p.nmax > 0 && Number.isFinite(p.massPackage) && p.massPackage > 0 &&
    Number.isFinite(p.terminalSpeedHeavy) && Number.isFinite(p.terminalSpeedLight) &&
    Number.isFinite(p.dragHeavy) && Number.isFinite(p.dragLight) && Number.isFinite(p.liftSpeedHeavy) &&
    Number.isFinite(p.liftSpeedLight) && Number.isFinite(p.liftSpread) && Number.isFinite(p.mergeColRadius) &&
    Number.isFinite(p.mergeHeightRadius) && Number.isFinite(p.contactEps) && p.contactEps > 0 && p.channelRateCap === 0 &&
    p.terminalSpeedHeavy >= 0 && p.terminalSpeedLight >= 0 && p.dragHeavy >= 0 && p.dragLight >= 0 &&
    p.liftSpeedHeavy >= 0 && p.liftSpeedLight >= 0 && p.liftSpread >= 0 && p.mergeColRadius >= 0 && p.mergeHeightRadius >= 0;
}
export function configureHot(config: P011ParticleHotConfig): P011ParticleHotWorkspace {
  if (config.cols !== 128 || config.nmax !== config.params.nmax || !validParams(config.params) ||
    !validCapacity(config.requestCapacity) || config.motionCapacity !== config.nmax ||
    !validCapacity(config.eventCapacity) || !validCapacity(config.lineageCapacity)) throw new Error('PARTICLE_HOT_INVALID_CONFIG');
  return new P011ParticleHotWorkspace(config, new P011ParticleField(config.cols, config.nmax));
}
export function warmUpHot(work: P011ParticleHotWorkspace): number {
  if (work.configured !== 1) return P011_HOT_NOT_CONFIGURED;
  work.pool.warmUpTransactions();
  work.lastStatus = P011_HOT_OK;
  return P011_HOT_OK;
}

export function beginStepHot(work: P011ParticleHotWorkspace, input: P011ParticleHotStepInput): number {
  if (work.configured !== 1) return work.lastStatus = P011_HOT_NOT_CONFIGURED;
  if (work.active !== 0) return work.lastStatus = P011_HOT_STEP_BUSY;
  if (work.pageBacklog !== 0) return work.lastStatus = P011_HOT_JOURNAL_OVERFLOW;
  const f = input.field; const c = work.cols;
  if(f.mediumMode!==undefined && f.mediumMode!=='WET_COLUMN_V1' && f.mediumMode!=='DRY_AIR_V1')return work.lastStatus=P011_HOT_INVALID_INPUT;
  if (!work.storageValidHot() || !fieldStorageValidHot(work, f, false)) return work.lastStatus = 3;
  if (!Number.isSafeInteger(input.runToken) || input.runToken <= 0 || !Number.isSafeInteger(input.substepId) || input.substepId < 0 ||
    !Number.isSafeInteger(input.attemptToken) || input.attemptToken <= 0 || !Number.isFinite(input.dt) || input.dt <= 0 ||
    !Number.isFinite(input.effectiveTiltRad) || !Number.isSafeInteger(input.seed) ||
    !Number.isSafeInteger(input.paramsRevision) || !Number.isSafeInteger(input.inputRevision) || f.cols !== c ||
    !Number.isFinite(f.capacity) || f.capacity < 1 || f.waterTop.length !== c || f.bh.length !== c || f.bl.length !== c || f.mh.length !== c ||
    f.ml.length !== c || f.suspH.length !== c || f.suspL.length !== c || f.surface.length !== c ||
    !Number.isFinite(f.sourceInjectedH) || f.sourceInjectedH < 0 || !Number.isFinite(f.sourceInjectedL) ||
    f.sourceInjectedL < 0 || !Number.isSafeInteger(f.sourceEpoch) || f.sourceEpoch < 0) {
    return work.lastStatus = P011_HOT_INVALID_INPUT;
  }
  for (let col = 0; col < c; col++) {
    if (!Number.isFinite(f.waterTop[col]) || !Number.isFinite(f.bh[col]) || !Number.isFinite(f.bl[col]) ||
      !Number.isFinite(f.mh[col]) || !Number.isFinite(f.ml[col]) || !Number.isFinite(f.suspH[col]) ||
      !Number.isFinite(f.suspL[col]) || !Number.isFinite(f.surface[col]) || f.waterTop[col] < 0 ||
      f.bh[col] < 0 || f.bl[col] < 0 || f.mh[col] < 0 || f.ml[col] < 0 || f.suspH[col] < 0 ||
      f.suspL[col] < 0 || !Number.isFinite(f.bh[col] + f.bl[col] + f.mh[col] + f.ml[col])) {
      return work.lastStatus = P011_HOT_INVALID_INPUT;
    }
  }
  if (work.stepEpoch >= Number.MAX_SAFE_INTEGER) return work.lastStatus = P011_HOT_TOKEN_EXHAUSTED;
  const poolStatus = work.pool.transferHotState(work.frozenPool, false);
  if (poolStatus !== P011_HOT_OK) return work.lastStatus = poolStatus;
  if(work.diagnosticEnabled){work.diagnosticFreeBefore=work.frozenPool.freeCount;
    work.diagnosticLiveBefore=work.frozenPool.liveCount;work.diagnosticNextIdBefore=work.frozenPool.nextId;
    work.diagnosticFirstError=0;work.diagnosticFirstErrorStage=0;}
  if (work.budget !== null) {
    const budgetStatus = work.budget.beginHot(work.stepEpoch + 1, input.dt);
    if (budgetStatus !== 0) return work.lastStatus = budgetStatus;
  }
  work.stepEpoch++;
  work.runToken = input.runToken; work.substepId = input.substepId; work.attemptToken = input.attemptToken;
  work.dt = input.dt; work.effectiveTiltRad = input.effectiveTiltRad; work.seed = input.seed;
  work.capacity = f.capacity; work.mediumMode = f.mediumMode ?? 'WET_COLUMN_V1';
  work.paramsRevision = input.paramsRevision; work.inputRevision = input.inputRevision;
  work.sourceInjectedH = f.sourceInjectedH; work.sourceInjectedL = f.sourceInjectedL; work.sourceEpoch = f.sourceEpoch;
  work.frozenWaterTop.set(f.waterTop); work.frozenBh.set(f.bh); work.frozenBl.set(f.bl);
  work.frozenMh.set(f.mh); work.frozenMl.set(f.ml); work.frozenSuspH.set(f.suspH); work.frozenSuspL.set(f.suspL);
  work.frozenSurface.set(f.surface);
  work.field = f;
  for (let i = 0; i < 8; i++) work.fieldBinding[i] = fieldBufferAtHot(f, i);
  work.fieldBeforeWaterTop.set(f.waterTop); work.fieldBeforeBh.set(f.bh); work.fieldBeforeBl.set(f.bl);
  work.fieldBeforeMh.set(f.mh); work.fieldBeforeMl.set(f.ml); work.fieldBeforeSuspH.set(f.suspH);
  work.fieldBeforeSuspL.set(f.suspL); work.fieldBeforeSurface.set(f.surface);
  work.active = 1; work.lastStatus = P011_HOT_OK; work.errorStageCode = 0;
  work.events.count = 0; work.events.lineageCount = 0; work.stepExternalH = 0; work.stepExternalL = 0;
  work.events.ownerId = work.ownerId; work.events.runToken = work.runToken; work.events.stepEpoch = work.stepEpoch;
  work.events.substepId = work.substepId; work.events.attemptToken = work.attemptToken;
  if (work.sinkCheckpoint !== null) work.sinkCheckpoint.saveOutsHot();
  work.stagedYActive = 0;
  work.seenEpochBefore.set(work.seenEpoch);
  work.planA.requestCount = 0; work.planA.motionCount = 0; work.planA.totalRequested = 0;
  work.planB.requestCount = 0; work.planB.motionCount = 0; work.planB.totalRequested = 0;
  work.planA.valid = 0; work.planB.valid = 0; work.planA.consumed = 0; work.planB.consumed = 0;
  work.results.resultCount = 0; work.results.status = P011_HOT_OK;
  return P011_HOT_OK;
}

export function checkpointHot(work: P011ParticleHotWorkspace, expectedStepEpoch: number): number {
  if (work.active !== 1 || expectedStepEpoch !== work.stepEpoch) return work.lastStatus = P011_HOT_STALE_HANDLE;
  return work.lastStatus = P011_HOT_OK;
}
export function rollbackStepHot(work: P011ParticleHotWorkspace, expectedStepEpoch: number): number {
  if (work.active !== 1 || expectedStepEpoch !== work.stepEpoch) return work.lastStatus = P011_HOT_STALE_HANDLE;
  const restored = work.pool.transferHotState(work.frozenPool, true);
  if (restored !== P011_HOT_OK) return work.lastStatus = restored;
  if (work.budget !== null) {
    const status = work.budget.rollbackHot(expectedStepEpoch); if (status !== 0) return work.lastStatus = status;
  }
  const field = work.field;
  if (field !== null) {
    field.waterTop.set(work.fieldBeforeWaterTop); field.bh.set(work.fieldBeforeBh); field.bl.set(work.fieldBeforeBl);
    field.mh.set(work.fieldBeforeMh); field.ml.set(work.fieldBeforeMl); field.suspH.set(work.fieldBeforeSuspH);
    field.suspL.set(work.fieldBeforeSuspL); field.surface.set(work.fieldBeforeSurface);
    field.sourceInjectedH = work.sourceInjectedH; field.sourceInjectedL = work.sourceInjectedL; field.sourceEpoch = work.sourceEpoch;
  }
  work.activePatchToken = 0; work.activePatchStepEpoch = 0; work.activePatchOrdinal = 0; work.patchState = 0;
  work.applying = 0;
  work.patchPlan = null; work.patchRow = -1; work.patchActions.count = 0; work.patchActions.lineageCount = 0;
  work.seenEpoch.set(work.seenEpochBefore);
  work.events.count = 0; work.events.lineageCount = 0; work.stepExternalH = 0; work.stepExternalL = 0;
  work.results.committed.fill(0); work.results.eventCount.fill(0);
  if (work.sinkCheckpoint !== null) work.sinkCheckpoint.restoreOutsHot();
  work.planA.consumed = 0; work.planB.consumed = 0; work.results.resultCount = 0;
  work.active = 0; work.rollbackCount++; work.lastStatus = P011_HOT_OK;
  return P011_HOT_OK;
}
export function finishStepHot(work: P011ParticleHotWorkspace, expectedStepEpoch: number): number {
  if (work.active !== 1 || expectedStepEpoch !== work.stepEpoch) return work.lastStatus = P011_HOT_STALE_HANDLE;
  if (work.activePatchToken !== 0) return work.lastStatus = P011_HOT_INVALID_PATCH_STATE;
  if (!work.storageValidHot() || work.field === null || !fieldStorageValidHot(work, work.field, true)) return work.lastStatus = 3;
  if (work.budget !== null) {
    if (work.planA.consumed !== 1 || work.results.resultCount !== work.planA.requestCount || !work.budget.allClosedHot()) {
      return work.lastStatus = P011_HOT_INVALID_PATCH_STATE;
    }
    const audit = auditParticleHot(work); if (audit !== 0) return work.lastStatus = audit;
    if (nextLeaseToken > Number.MAX_SAFE_INTEGER) { work.tokenExhausted = 1; return work.lastStatus = 14; }
    const status = work.budget.finishHot(expectedStepEpoch); if (status !== 0) return work.lastStatus = status;
    sealEvidenceHot(work);
  }
  work.active = 0; work.lastStatus = P011_HOT_OK; return P011_HOT_OK;
}

function sealEvidenceHot(work: P011ParticleHotWorkspace): void {
  const a = work.planA, b = work.sealedPlan;
  b.ownerId = a.ownerId; b.runToken = a.runToken; b.stepEpoch = a.stepEpoch; b.substepId = a.substepId;
  b.attemptToken = a.attemptToken; b.paramsRevision = a.paramsRevision; b.inputRevision = a.inputRevision;
  b.seed = a.seed; b.dt = a.dt; b.effectiveTiltRad = a.effectiveTiltRad;
  b.requestCount = a.requestCount; b.motionCount = a.motionCount; b.totalRequested = a.totalRequested;
  b.valid = a.valid; b.consumed = a.consumed;
  for (let i = 0; i < a.requestCount; i++) {
    b.kind[i] = a.kind[i]; b.phase[i] = a.phase[i]; b.sourceKind[i] = a.sourceKind[i]; b.column[i] = a.column[i]; b.targetColumn[i] = a.targetColumn[i]; b.particleId[i] = a.particleId[i]; b.sourceId[i] = a.sourceId[i]; b.substep[i] = a.substep[i]; b.ordinal[i] = a.ordinal[i]; b.requested[i] = a.requested[i]; b.totalWeight[i] = a.totalWeight[i]; b.predictedX[i] = a.predictedX[i]; b.predictedY[i] = a.predictedY[i]; b.predictedVx[i] = a.predictedVx[i]; b.predictedVy[i] = a.predictedVy[i]; b.mergeStart[i] = a.mergeStart[i]; b.mergeCount[i] = a.mergeCount[i];
    for (let j = 0; j < a.mergeCount[i]; j++) b.lineage[a.mergeStart[i] + j] = a.lineage[a.mergeStart[i] + j];
  }
  for (let i = 0; i < a.motionCount; i++) {
    b.motionParticleId[i] = a.motionParticleId[i]; b.motionColumn[i] = a.motionColumn[i]; b.motionX[i] = a.motionX[i]; b.motionY[i] = a.motionY[i]; b.motionVx[i] = a.motionVx[i]; b.motionVy[i] = a.motionVy[i];
  }
  const r = work.results, s = work.sealedResults;
  s.ownerId = r.ownerId; s.stepEpoch = r.stepEpoch; s.resultCount = r.resultCount; s.status = r.status;
  for (let i = 0; i < r.resultCount; i++) {
    s.committed[i] = r.committed[i]; s.disposition[i] = r.disposition[i]; s.released[i] = r.released[i]; s.rowStatus[i] = r.rowStatus[i]; s.guardIndex[i] = r.guardIndex[i]; s.eventBegin[i] = r.eventBegin[i]; s.eventCount[i] = r.eventCount[i]; s.granted[i] = r.granted[i]; s.remainder[i] = r.remainder[i]; s.budgetClipped[i] = r.budgetClipped[i]; s.guardRejected[i] = r.guardRejected[i]; s.capacityClipped[i] = r.capacityClipped[i]; s.poolClipped[i] = r.poolClipped[i]; s.massAfter[i] = r.massAfter[i];
  }
  work.pool.transferHotState(work.patchAfter, false);
  const f = work.field;
  if (f !== null) {
    work.fieldBeforeWaterTop.set(f.waterTop); work.fieldBeforeBh.set(f.bh); work.fieldBeforeBl.set(f.bl);
    work.fieldBeforeMh.set(f.mh); work.fieldBeforeMl.set(f.ml); work.fieldBeforeSuspH.set(f.suspH); work.fieldBeforeSuspL.set(f.suspL);
    work.fieldBeforeSurface.set(f.surface); work.pageSourceH = f.sourceInjectedH;
    work.pageSourceL = f.sourceInjectedL; work.pageSourceEpoch = f.sourceEpoch;
  }
  work.committedPageToken = allocateParticleHotLease(); work.pageBacklog = 1;
}

// Numeric operation codes are fixed by this module and converted to legacy strings only by cold export.
export const P011_HOT_KIND_SPAWN = 1;
export const P011_HOT_KIND_DEPOSIT = 2;
export const P011_HOT_KIND_CROSSING = 3;
export const P011_HOT_KIND_LIFT = 4;
export const P011_HOT_KIND_DEGRADE = 5;
export const P011_HOT_KIND_MERGE = 6;
export const P011_HOT_DISPOSITION_PARTICLE = 1;
export const P011_HOT_DISPOSITION_BED = 2;
export const P011_HOT_DISPOSITION_SUSP = 3;
export const P011_HOT_DISPOSITION_MOVE = 4;
export const P011_HOT_DISPOSITION_MERGE = 5;
export const P011_HOT_DISPOSITION_WAIT = 6;
export const P011_HOT_PHASE_HEAVY = 0;
export const P011_HOT_PHASE_LIGHT = 1;

function beforeInOrder(pool: P011ParticleSnapshot, a: number, b: number): boolean {
  const ia = pool.pId[a]; const ib = pool.pId[b];
  return ia < ib || (ia === ib && a < b);
}
function siftDown(order: Int32Array, root: number, end: number, pool: P011ParticleSnapshot): void {
  let parent = root;
  while (parent * 2 + 1 <= end) {
    let child = parent * 2 + 1;
    if (child + 1 <= end && beforeInOrder(pool, order[child], order[child + 1])) child++;
    if (!beforeInOrder(pool, order[parent], order[child])) return;
    const t = order[parent]; order[parent] = order[child]; order[child] = t; parent = child;
  }
}
function sortActive(work: P011ParticleHotWorkspace): number {
  const pool = work.frozenPool; const order = work.order; let count = 0;
  for (let i = 0; i < pool.nmax; i++) {
    if (pool.pState[i] !== 0) {
      if (!Number.isFinite(pool.pMass[i]) || !Number.isFinite(pool.pX[i]) || !Number.isFinite(pool.pY[i]) ||
        !Number.isFinite(pool.pVx[i]) || !Number.isFinite(pool.pVy[i]) || pool.pId[i] === 0 ||
        pool.pMass[i] <= 0 || pool.pPhase[i] > 1 || pool.pSource[i] > 1 || pool.pSourceId[i] === 0 ||
        pool.pState[i] > 2 || pool.pX[i] < 0 || pool.pX[i] >= work.cols || pool.pY[i] < 0 ||
        pool.pY[i] > work.capacity) return -1;
      order[count++] = i;
    }
  }
  sortOrderHot(order, count, pool);
  for (let i = 1; i < count; i++) if (pool.pId[order[i - 1]] === pool.pId[order[i]]) return -1;
  return count;
}
function sortOrderHot(order: Int32Array, count: number, pool: P011ParticleSnapshot): void {
  for (let i = Math.floor(count / 2) - 1; i >= 0; i--) siftDown(order, i, count - 1, pool);
  for (let end = count - 1; end > 0; end--) {
    const t = order[0]; order[0] = order[end]; order[end] = t; siftDown(order, 0, end - 1, pool);
  }
}

export function collectParticlePlanInto(work: P011ParticleHotWorkspace, expectedStepEpoch: number,
  destination: P011ParticleHotPlanBuffer): number {
  if (work.diagnosticEnabled) work.collectCalls++;
  if (work.active !== 1 || expectedStepEpoch !== work.stepEpoch) return work.lastStatus = P011_HOT_STALE_HANDLE;
  if (destination !== work.planA && destination !== work.planB) return work.lastStatus = P011_HOT_INVALID_BUFFER;
  if (work.planA.consumed !== 0 || work.applying !== 0) return work.lastStatus = P011_HOT_INVALID_PATCH_STATE;
  if (!destination.storageValidHot()) return work.lastStatus = P011_HOT_INVALID_BUFFER;
  const count = sortActive(work);
  if(work.diagnosticEnabled && count>=0)work.diagnosticActiveCount=count;
  if (count < 0) { destination.valid = 0; return work.lastStatus = P011_HOT_INVALID_INPUT; }
  const p = work.frozenPool; let deposits = 0; let crossings = 0; let motions = 0; let total = 0;
  // First pass counts output rows, so overflow is rejected before writing any plan row.
  for (let j = 0; j < count; j++) {
    const i = work.order[j]; const c = Math.floor(p.pX[i]);
    if (c < 0 || c >= work.cols) return work.lastStatus = P011_HOT_INVALID_INPUT;
    const heavy = p.pPhase[i] === P011_HOT_PHASE_HEAVY;
    const speed = heavy ? work.params.terminalSpeedHeavy : work.params.terminalSpeedLight;
    const drag = heavy ? work.params.dragHeavy : work.params.dragLight;
    const dry = work.mediumMode === 'DRY_AIR_V1';
    const wet = !dry && p.pY[i] <= work.frozenWaterTop[c];
    const blend = 1 - Math.exp(-drag * work.dt);
    const vx = wet ? p.pVx[i] + (speed * Math.sin(work.effectiveTiltRad) - p.pVx[i]) * blend :
      dry ? p.pVx[i] * Math.exp(-0.01 * work.dt) + 9.8 * Math.sin(work.effectiveTiltRad) * work.dt : p.pVx[i] * Math.exp(-0.01 * work.dt);
    const vy = wet ? p.pVy[i] + (-speed * Math.cos(work.effectiveTiltRad) - p.pVy[i]) * blend :
      p.pVy[i] - 9.8 * (dry ? Math.cos(work.effectiveTiltRad) : 1) * work.dt;
    const x = Math.max(0, Math.min(work.cols - 1e-9,
      Math.max(c - 1 + 1e-9, Math.min(c + 2 - 1e-9, p.pX[i] + vx * work.dt))));
    const y = Math.min(work.capacity, p.pY[i] + vy * work.dt);
    if (!Number.isFinite(vx) || !Number.isFinite(vy) || !Number.isFinite(y) || !Number.isFinite(x)) {
      return work.lastStatus = P011_HOT_INVALID_INPUT;
    }
    const contact = p.pState[i] === 2 || y <= sandHot(work, c);
    const target = Math.floor(x);
    if (!contact && target === c) motions++;
    else if (contact) deposits++;
    else crossings++;
    if (contact || target !== c) total += p.pMass[i];
  }
  if (!Number.isFinite(total)) return work.lastStatus = P011_HOT_INVALID_INPUT;
  if (deposits + crossings > work.requestCapacity) return work.lastStatus = P011_HOT_PLAN_OVERFLOW;
  if (motions > work.motionCapacity) return work.lastStatus = P011_HOT_MOTION_OVERFLOW;
  destination.requestCount = 0; destination.motionCount = 0; destination.totalRequested = 0;
  total = 0;
  destination.ownerId = work.ownerId; destination.runToken = work.runToken; destination.stepEpoch = work.stepEpoch;
  destination.substepId = work.substepId; destination.attemptToken = work.attemptToken;
  destination.paramsRevision = work.paramsRevision; destination.inputRevision = work.inputRevision;
  destination.seed = work.seed; destination.dt = work.dt; destination.effectiveTiltRad = work.effectiveTiltRad;
  // Preserve the legacy global phase order: all deposits, followed by all crossings.
  for (let phasePass = 0; phasePass < 2; phasePass++) {
    for (let j = 0; j < count; j++) {
      const i = work.order[j]; const c = Math.floor(p.pX[i]); const heavy = p.pPhase[i] === P011_HOT_PHASE_HEAVY;
      const speed = heavy ? work.params.terminalSpeedHeavy : work.params.terminalSpeedLight;
      const drag = heavy ? work.params.dragHeavy : work.params.dragLight;
      const dry = work.mediumMode === 'DRY_AIR_V1';
    const wet = !dry && p.pY[i] <= work.frozenWaterTop[c]; const blend = 1 - Math.exp(-drag * work.dt);
      const vx = wet ? p.pVx[i] + (speed * Math.sin(work.effectiveTiltRad) - p.pVx[i]) * blend :
        dry ? p.pVx[i] * Math.exp(-0.01 * work.dt) + 9.8 * Math.sin(work.effectiveTiltRad) * work.dt : p.pVx[i] * Math.exp(-0.01 * work.dt);
      const vy = wet ? p.pVy[i] + (-speed * Math.cos(work.effectiveTiltRad) - p.pVy[i]) * blend :
        p.pVy[i] - 9.8 * (dry ? Math.cos(work.effectiveTiltRad) : 1) * work.dt;
      const x = Math.max(0, Math.min(work.cols - 1e-9,
        Math.max(c - 1 + 1e-9, Math.min(c + 2 - 1e-9, p.pX[i] + vx * work.dt))));
      const y = Math.min(work.capacity, p.pY[i] + vy * work.dt); const contact = p.pState[i] === 2 || y <= sandHot(work, c);
      const target = Math.floor(x); const kind = contact ? P011_HOT_KIND_DEPOSIT : P011_HOT_KIND_CROSSING;
      if (!contact && target === c) {
        if (phasePass === 0) {
          const m = destination.motionCount++; destination.motionParticleId[m] = p.pId[i]; destination.motionColumn[m] = c;
          destination.motionX[m] = x; destination.motionY[m] = Math.max(y, sandHot(work, c) + work.params.contactEps);
          destination.motionVx[m] = vx; destination.motionVy[m] = vy;
        }
        continue;
      }
      if ((phasePass === 0 && kind !== P011_HOT_KIND_DEPOSIT) ||
        (phasePass === 1 && kind !== P011_HOT_KIND_CROSSING)) continue;
      const r = destination.requestCount++; destination.kind[r] = kind; destination.phase[r] = p.pPhase[i];
      destination.sourceKind[r] = p.pSource[i]; destination.column[r] = c;
      destination.targetColumn[r] = contact ? -1 : target; destination.particleId[r] = p.pId[i];
      destination.sourceId[r] = p.pSourceId[i]; destination.substep[r] = work.substepId;
      destination.ordinal[r] = r + 1; destination.requested[r] = p.pMass[i]; destination.totalWeight[r] = p.pMass[i];
      destination.predictedX[r] = contact ? p.pX[i] : x;
      destination.predictedY[r] = contact ? sandHot(work, c) + work.params.contactEps :
        Math.max(y, sandHot(work, target) + work.params.contactEps);
      destination.predictedVx[r] = contact ? 0 : vx; destination.predictedVy[r] = contact ? 0 : vy;
      destination.mergeStart[r] = 0; destination.mergeCount[r] = 0; total += p.pMass[i];
    }
  }
  destination.totalRequested = total; destination.valid = 1; destination.consumed = 0;
  return work.lastStatus = P011_HOT_OK;
}

export function compareParticlePlansHot(a: P011ParticleHotPlanBuffer, b: P011ParticleHotPlanBuffer): number {
  if (a === b || !planValuesValidHot(a) || !planValuesValidHot(b) || a.ownerId !== b.ownerId || a.runToken !== b.runToken ||
    a.stepEpoch !== b.stepEpoch || a.substepId !== b.substepId || a.attemptToken !== b.attemptToken ||
    a.paramsRevision !== b.paramsRevision || a.inputRevision !== b.inputRevision || a.seed !== b.seed ||
    a.dt !== b.dt || a.effectiveTiltRad !== b.effectiveTiltRad || a.requestCount !== b.requestCount ||
    a.motionCount !== b.motionCount || a.totalRequested !== b.totalRequested) return P011_HOT_PLAN_MISMATCH;
  for (let i = 0; i < a.requestCount; i++) {
    if (a.kind[i] !== b.kind[i] || a.phase[i] !== b.phase[i] || a.sourceKind[i] !== b.sourceKind[i] ||
      a.column[i] !== b.column[i] || a.targetColumn[i] !== b.targetColumn[i] || a.particleId[i] !== b.particleId[i] ||
      a.sourceId[i] !== b.sourceId[i] || a.substep[i] !== b.substep[i] || a.ordinal[i] !== b.ordinal[i] ||
      a.requested[i] !== b.requested[i] || a.totalWeight[i] !== b.totalWeight[i] ||
      a.predictedX[i] !== b.predictedX[i] || a.predictedY[i] !== b.predictedY[i] ||
      a.predictedVx[i] !== b.predictedVx[i] || a.predictedVy[i] !== b.predictedVy[i] ||
      a.mergeStart[i] !== b.mergeStart[i] || a.mergeCount[i] !== b.mergeCount[i]) return P011_HOT_PLAN_MISMATCH;
    for (let j = 0; j < a.mergeCount[i]; j++) {
      if (a.lineage[a.mergeStart[i] + j] !== b.lineage[b.mergeStart[i] + j]) return P011_HOT_PLAN_MISMATCH;
    }
  }
  for (let i = 0; i < a.motionCount; i++) {
    if (a.motionParticleId[i] !== b.motionParticleId[i] || a.motionColumn[i] !== b.motionColumn[i] ||
      a.motionX[i] !== b.motionX[i] || a.motionY[i] !== b.motionY[i] || a.motionVx[i] !== b.motionVx[i] ||
      a.motionVy[i] !== b.motionVy[i]) return P011_HOT_PLAN_MISMATCH;
  }
  return P011_HOT_OK;
}

function sandHot(work: P011ParticleHotWorkspace, c: number): number {
  return work.frozenBh[c] + work.frozenBl[c] + work.frozenMh[c] + work.frozenMl[c];
}

// S1 validates every effective row, including equal NaNs in both buffers.
function planValuesValidHot(p: P011ParticleHotPlanBuffer): boolean {
  if (!p.storageValidHot()) return false;
  if (p.valid !== 1 || !Number.isSafeInteger(p.ownerId) || p.ownerId <= 0 ||
    !Number.isSafeInteger(p.runToken) || p.runToken <= 0 || !Number.isSafeInteger(p.stepEpoch) || p.stepEpoch <= 0 ||
    !Number.isSafeInteger(p.substepId) || p.substepId < 0 || !Number.isSafeInteger(p.attemptToken) || p.attemptToken <= 0 ||
    !Number.isSafeInteger(p.paramsRevision) || !Number.isSafeInteger(p.inputRevision) || !Number.isSafeInteger(p.seed) ||
    !Number.isFinite(p.dt) || p.dt <= 0 || !Number.isFinite(p.effectiveTiltRad) ||
    !Number.isSafeInteger(p.requestCount) || p.requestCount < 0 || p.requestCount > p.requestCapacity ||
    !Number.isSafeInteger(p.motionCount) || p.motionCount < 0 || p.motionCount > p.motionCapacity ||
    !Number.isFinite(p.totalRequested) || p.totalRequested < 0) return false;
  let total = 0;
  for (let i = 0; i < p.requestCount; i++) {
    if (p.kind[i] < 1 || p.kind[i] > 6 || p.phase[i] > 1 || p.sourceKind[i] > 1 ||
      !Number.isSafeInteger(p.ordinal[i]) || p.ordinal[i] < 1 || p.ordinal[i] > p.requestCapacity ||
      p.substep[i] !== p.substepId || p.sourceId[i] === 0 || p.column[i] < 0 || p.column[i] >= 128 ||
      !Number.isFinite(p.requested[i]) || p.requested[i] < 0 ||
      !Number.isFinite(p.totalWeight[i]) || p.totalWeight[i] !== ((p.kind[i] === 2 || p.kind[i] === 3) ? p.requested[i] : 0) ||
      !Number.isFinite(p.predictedX[i]) || !Number.isFinite(p.predictedY[i]) ||
      !Number.isFinite(p.predictedVx[i]) || !Number.isFinite(p.predictedVy[i]) ||
      p.mergeStart[i] < 0 || p.mergeCount[i] < 0 || p.mergeStart[i] + p.mergeCount[i] > p.lineageCapacity) return false;
    total += p.totalWeight[i];
  }
  if (total !== p.totalRequested) return false;
  for (let i = 0; i < p.motionCount; i++) if (p.motionParticleId[i] === 0 || p.motionColumn[i] < 0 || p.motionColumn[i] >= 128 ||
    !Number.isFinite(p.motionX[i]) || !Number.isFinite(p.motionY[i]) || !Number.isFinite(p.motionVx[i]) ||
    !Number.isFinite(p.motionVy[i])) return false;
  return true;
}

// S2: no token is ever restored with a physical checkpoint.
export function allocateParticleHotLease(): number {
  if (nextLeaseToken > Number.MAX_SAFE_INTEGER) return 0;
  return nextLeaseToken++;
}
function copyPoolHot(from: P011ParticleSnapshot, to: P011ParticleSnapshot): void {
  to.liveCount = from.liveCount; to.freeCount = from.freeCount; to.nextId = from.nextId;
  to.pMass.set(from.pMass); to.pX.set(from.pX); to.pY.set(from.pY); to.pVx.set(from.pVx); to.pVy.set(from.pVy);
  to.pPhase.set(from.pPhase); to.pSource.set(from.pSource); to.pState.set(from.pState); to.pId.set(from.pId);
  to.pSourceId.set(from.pSourceId); to.freeSlots.set(from.freeSlots);
  to.colParticleMassH.set(from.colParticleMassH); to.colParticleMassL.set(from.colParticleMassL);
}
function particleSlotHot(pool: P011ParticleSnapshot, id: number): number {
  for (let i = 0; i < pool.nmax; i++) if (pool.pState[i] !== 0 && pool.pId[i] === id) return i;
  return -1;
}
export function sameRequestHot(a: P011ParticleHotPlanBuffer, i: number, b: P011ParticleHotPlanBuffer, j: number): boolean {
  if (a.kind[i] !== b.kind[j] || a.phase[i] !== b.phase[j] || a.sourceKind[i] !== b.sourceKind[j] ||
    a.column[i] !== b.column[j] || a.targetColumn[i] !== b.targetColumn[j] || a.particleId[i] !== b.particleId[j] ||
    a.sourceId[i] !== b.sourceId[j] || a.substep[i] !== b.substep[j] || a.ordinal[i] !== b.ordinal[j] ||
    a.requested[i] !== b.requested[j] || a.totalWeight[i] !== b.totalWeight[j] ||
    a.predictedX[i] !== b.predictedX[j] || a.predictedY[i] !== b.predictedY[j] ||
    a.predictedVx[i] !== b.predictedVx[j] || a.predictedVy[i] !== b.predictedVy[j] ||
    a.mergeCount[i] !== b.mergeCount[j]) return false;
  for (let k = 0; k < a.mergeCount[i]; k++) if (a.lineage[a.mergeStart[i] + k] !== b.lineage[b.mergeStart[j] + k]) return false;
  return true;
}
function rememberRequestHot(work: P011ParticleHotWorkspace, p: P011ParticleHotPlanBuffer, i: number): void {
  const o = p.ordinal[i]; const j = o - 1; const b = work.seenRequests;
  b.kind[j] = p.kind[i]; b.phase[j] = p.phase[i]; b.sourceKind[j] = p.sourceKind[i];
  b.column[j] = p.column[i]; b.targetColumn[j] = p.targetColumn[i]; b.particleId[j] = p.particleId[i];
  b.sourceId[j] = p.sourceId[i]; b.substep[j] = p.substep[i]; b.ordinal[j] = o;
  b.requested[j] = p.requested[i]; b.totalWeight[j] = p.totalWeight[i];
  b.predictedX[j] = p.predictedX[i]; b.predictedY[j] = p.predictedY[i];
  b.predictedVx[j] = p.predictedVx[i]; b.predictedVy[j] = p.predictedVy[i];
  b.mergeStart[j] = j * 2; b.mergeCount[j] = p.mergeCount[i];
  for (let k = 0; k < p.mergeCount[i]; k++) b.lineage[j * 2 + k] = p.lineage[p.mergeStart[i] + k];
  work.seenEpoch[o] = work.stepEpoch;
}
function requestIdentityHot(work: P011ParticleHotWorkspace, p: P011ParticleHotPlanBuffer, row: number): number {
  if (work.active !== 1 || p.ownerId !== work.ownerId || p.stepEpoch !== work.stepEpoch ||
    p.runToken !== work.runToken || p.substepId !== work.substepId || p.attemptToken !== work.attemptToken ||
    p.paramsRevision !== work.paramsRevision || p.inputRevision !== work.inputRevision || p.seed !== work.seed ||
    p.dt !== work.dt || p.effectiveTiltRad !== work.effectiveTiltRad || p.consumed !== 0) return P011_HOT_STALE_HANDLE;
  if ((p !== work.planA && p !== work.planB) || (work.applying !== 1 && !planValuesValidHot(p)) ||
    !Number.isInteger(row) || row < 0 || row >= p.requestCount ||
    (p.kind[row] === P011_HOT_KIND_MERGE ? p.mergeCount[row] !== 2 : p.mergeCount[row] !== 0)) return P011_HOT_INVALID_INPUT;
  if (work.applying === 1 && !sameRequestHot(p, row, work.planB, row)) return P011_HOT_IDENTITY_MISMATCH;
  const ordinal = p.ordinal[row];
  if (work.seenEpoch[ordinal] === work.stepEpoch) {
    return sameRequestHot(p, row, work.seenRequests, ordinal - 1) ? P011_HOT_DUPLICATE_REQUEST : P011_HOT_IDENTITY_MISMATCH;
  }
  return P011_HOT_OK;
}
export function validateParticleHotRequest(work: P011ParticleHotWorkspace, p: P011ParticleHotPlanBuffer, row: number): number {
  return requestIdentityHot(work, p, row);
}
function actionCommonHot(work: P011ParticleHotWorkspace, p: P011ParticleSnapshot, slot: number, kind: number): number {
  const a = work.patchActions;
  if (a.count >= a.capacity) return -1;
  const i = a.count++; a.kind[i] = kind; a.particleId[i] = p.pId[slot]; a.sourceId[i] = p.pSourceId[slot];
  a.phase[i] = p.pPhase[slot]; a.sourceKind[i] = p.pSource[slot]; a.column[i] = Math.floor(p.pX[slot]);
  a.targetColumn[i] = -1; a.x[i] = p.pX[slot]; a.y[i] = p.pY[slot]; a.vx[i] = p.pVx[slot]; a.vy[i] = p.pVy[slot];
  a.requested[i] = p.pMass[slot]; a.granted[i] = p.pMass[slot]; a.remainder[i] = 0;
  a.massBefore[i] = p.pMass[slot]; a.massAfter[i] = p.pMass[slot];
  a.lineageStart[i] = a.lineageCount; a.lineageLength[i] = 0;
  return i;
}
function mergePairHot(work: P011ParticleHotWorkspace, p: P011ParticleSnapshot, a: number, b: number): number {
  if (a === b || a < 0 || b < 0 || p.pPhase[a] !== p.pPhase[b] || p.pSource[a] !== p.pSource[b] ||
    p.pSourceId[a] !== p.pSourceId[b] || Math.floor(p.pX[a]) !== Math.floor(p.pX[b]) ||
    Math.abs(p.pY[a] - p.pY[b]) > work.params.mergeHeightRadius || p.pState[a] !== p.pState[b]) return P011_HOT_INVALID_PATCH_STATE;
  const actions = work.patchActions;
  if (actions.count >= actions.capacity) return P011_HOT_JOURNAL_OVERFLOW;
  if (actions.lineageCount + 2 > actions.lineageCapacity) return P011_HOT_LINEAGE_OVERFLOW;
  const ma = p.pMass[a]; const mb = p.pMass[b]; const mass = ma + mb;
  if (!Number.isFinite(mass)) return P011_HOT_INVALID_INPUT;
  const event = actionCommonHot(work, p, a, P011_HOT_KIND_MERGE);
  actions.lineage[actions.lineageCount++] = p.pId[a]; actions.lineage[actions.lineageCount++] = p.pId[b];
  actions.lineageLength[event] = 2; actions.requested[event] = mass; actions.granted[event] = mass;
  actions.massBefore[event] = mass; actions.massAfter[event] = mass;
  p.pX[a] = (p.pX[a] * ma + p.pX[b] * mb) / mass; p.pY[a] = (p.pY[a] * ma + p.pY[b] * mb) / mass;
  p.pVx[a] = (p.pVx[a] * ma + p.pVx[b] * mb) / mass; p.pVy[a] = (p.pVy[a] * ma + p.pVy[b] * mb) / mass;
  p.pMass[a] = mass; p.pMass[b] = 0; p.pState[b] = 0; p.liveCount--; p.freeSlots[p.freeCount++] = b;
  return P011_HOT_OK;
}
function mergeOldestHot(work: P011ParticleHotWorkspace, p: P011ParticleSnapshot): number {
  if (work.diagnosticEnabled) work.mergeCalls++;
  // Rebuild ID order from the current scratch pool after every merge. The old
  // scan selected the lexicographically smallest legal (pId[a], pId[b]).
  let afterId = 0;
  for (let pass = 0; pass < p.nmax; pass++) {
    let a = -1;
    for (let i = 0; i < p.nmax; i++) if (p.pState[i] !== 0 && p.pId[i] > afterId &&
      (a < 0 || p.pId[i] < p.pId[a])) a = i;
    if (a < 0) break;
    let b = -1;
    for (let j = 0; j < p.nmax; j++) {
      if (work.diagnosticEnabled) work.mergeCandidateChecks++;
      if (p.pState[j] !== 0 && p.pId[a] < p.pId[j] &&
        p.pPhase[a] === p.pPhase[j] && p.pSource[a] === p.pSource[j] && p.pSourceId[a] === p.pSourceId[j] &&
        p.pState[a] === p.pState[j] && Math.floor(p.pX[a]) === Math.floor(p.pX[j]) &&
        Math.abs(p.pY[a] - p.pY[j]) <= work.params.mergeHeightRadius &&
        (b < 0 || p.pId[j] < p.pId[b])) b = j;
    }
    if (b >= 0) { const status = mergePairHot(work, p, a, b); if (status === 0 && work.diagnosticEnabled) work.mergeSuccess++; return status; }
    afterId = p.pId[a];
  }
  return P011_HOT_INVALID_PATCH_STATE;
}
function editPoolHot(work: P011ParticleHotWorkspace, plan: P011ParticleHotPlanBuffer, row: number,
  gh: number, gl: number, disposition: number): number {
  const p = work.patchScratch; const kind = plan.kind[row]; const c = plan.column[row]; const amount = gh + gl;
  if (kind === P011_HOT_KIND_SPAWN || kind === P011_HOT_KIND_LIFT) {
    if (disposition === P011_HOT_DISPOSITION_SUSP) return kind === P011_HOT_KIND_LIFT ? 0 : P011_HOT_INVALID_PATCH_STATE;
    if (disposition === P011_HOT_DISPOSITION_WAIT) return amount === 0 ? 0 : P011_HOT_INVALID_PATCH_STATE;
    if (disposition !== P011_HOT_DISPOSITION_PARTICLE ||
      (kind === 1 && plan.sourceKind[row] !== 0) || (kind === 4 && plan.sourceKind[row] !== 1)) return P011_HOT_INVALID_INPUT;
    for (let phase = 0; phase < 2; phase++) {
      const mass = phase === 0 ? gh : gl; const count = Math.floor(mass / work.params.massPackage);
      const tail = mass - count * work.params.massPackage;
      const births = count + (tail > 0 ? 1 : 0);
      if (work.diagnosticEnabled) work.birthRequested += births;
      if (!Number.isSafeInteger(births) || births > work.patchActions.capacity - work.patchActions.count) return P011_HOT_JOURNAL_OVERFLOW;
      for (let k = 0; k < births; k++) {
        if (p.freeCount === 0) {
          if (work.mediumMode === 'DRY_AIR_V1') return 5;
          const merged = mergeOldestHot(work, p); if (merged !== 0) return merged;
        }
        if (p.nextId > 4294967295) return P011_HOT_INVALID_INPUT;
        const salt = (Math.imul(plan.sourceId[row], 1664525) ^ Math.imul(plan.ordinal[row] + k + phase * 8191, 1013904223) ^ work.seed) >>> 0;
        const jitter = (salt / 4294967296 - 0.5) * 0.999;
        const f = work.field; if (f === null) return P011_HOT_NOT_CONFIGURED;
        const sand = f.bh[c] + f.bl[c] + f.mh[c] + f.ml[c];
        const low = kind === 4 ? plan.predictedY[row] : sand + work.params.contactEps;
        const dry = work.mediumMode === 'DRY_AIR_V1';
        const high = dry ? f.capacity : f.waterTop[c] - work.params.contactEps;
        if (high < low) return P011_HOT_INVALID_PATCH_STATE;
        const speed = phase === 0 ? work.params.terminalSpeedHeavy : work.params.terminalSpeedLight;
        const lift = phase === 0 ? work.params.liftSpeedHeavy : work.params.liftSpeedLight;
        const slot = p.freeSlots[--p.freeCount];
        p.pMass[slot] = k < count ? work.params.massPackage : tail; p.pX[slot] = dry && kind === 1 ? plan.predictedX[row] : c + 0.5 + jitter;
        p.pY[slot] = dry && kind === 1 ? plan.predictedY[row] : kind === 4 ? low : Math.max(low, Math.min(high, plan.predictedY[row]));
        p.pVx[slot] = dry && kind === 1 ? 0 : kind === 4 ? jitter * 2 * work.params.liftSpread : speed * Math.sin(work.effectiveTiltRad);
        p.pVy[slot] = dry && kind === 1 ? 0 : kind === 4 ? lift : -speed * Math.cos(work.effectiveTiltRad);
        p.pPhase[slot] = phase; p.pSource[slot] = plan.sourceKind[row]; p.pSourceId[slot] = plan.sourceId[row];
        p.pId[slot] = p.nextId++; p.pState[slot] = 1; p.liveCount++;
        if (work.diagnosticEnabled) work.birthCompleted++;
        if (phase === 0) p.colParticleMassH[c] += p.pMass[slot]; else p.colParticleMassL[c] += p.pMass[slot];
        if (actionCommonHot(work, p, slot, P011_HOT_KIND_SPAWN) < 0) return P011_HOT_JOURNAL_OVERFLOW;
      }
    }
    return 0;
  }
  const slot = particleSlotHot(p, plan.particleId[row]);
  if (slot < 0) return P011_HOT_UNKNOWN_PARTICLE;
  if (p.pPhase[slot] !== plan.phase[row] || p.pSource[slot] !== plan.sourceKind[row] || p.pSourceId[slot] !== plan.sourceId[row] ||
    Math.floor(p.pX[slot]) !== c || p.pMass[slot] !== plan.requested[row] || (plan.phase[row] === 0 ? gl : gh) !== 0) return P011_HOT_IDENTITY_MISMATCH;
  const cache = plan.phase[row] === 0 ? p.colParticleMassH : p.colParticleMassL;
  if (kind === P011_HOT_KIND_DEPOSIT) {
    if ((disposition !== 2 && disposition !== 6) || amount > p.pMass[slot]) return P011_HOT_INVALID_PATCH_STATE;
    const remainder = amount === p.pMass[slot] ? 0 : p.pMass[slot] - amount;
    cache[c] -= amount; p.pMass[slot] = remainder;
    if (remainder === 0) { p.pState[slot] = 0; p.liveCount--; p.freeSlots[p.freeCount++] = slot; }
    else { p.pState[slot] = 2; p.pY[slot] = (plan.predictedY[row] - work.params.contactEps) + work.params.contactEps; p.pVy[slot] = 0; }
  } else if (kind === P011_HOT_KIND_DEGRADE) {
    if (disposition !== 3 || amount !== p.pMass[slot]) return P011_HOT_INVALID_PATCH_STATE;
    cache[c] -= p.pMass[slot]; p.pMass[slot] = 0; p.pState[slot] = 0; p.freeSlots[p.freeCount++] = slot; p.liveCount--;
  } else if (kind === P011_HOT_KIND_CROSSING) {
    const target = plan.targetColumn[row];
    if (target < 0 || target >= p.cols || Math.abs(target - c) !== 1) return P011_HOT_INVALID_INPUT;
    if (disposition === 6 && amount === 0) p.pX[slot] = target > c ? c + 1 - 1e-9 : c + 1e-9;
    else {
      if (disposition !== 4 || amount !== p.pMass[slot] || Math.floor(plan.predictedX[row]) !== target) return P011_HOT_INVALID_PATCH_STATE;
      cache[c] -= amount; cache[target] += amount;
      p.pX[slot] = plan.predictedX[row]; p.pY[slot] = plan.predictedY[row];
      p.pVx[slot] = plan.predictedVx[row]; p.pVy[slot] = plan.predictedVy[row];
    }
  } else if (kind === P011_HOT_KIND_MERGE) {
    if (disposition !== 5 || plan.lineage[plan.mergeStart[row]] !== plan.particleId[row]) return P011_HOT_INVALID_PATCH_STATE;
    return mergePairHot(work, p, slot, particleSlotHot(p, plan.lineage[plan.mergeStart[row] + 1]));
  } else return P011_HOT_INVALID_INPUT;
  return 0;
}

export function preparePatchHot(work: P011ParticleHotWorkspace, plan: P011ParticleHotPlanBuffer,
  row: number, grantedHeavy: number, grantedLight: number, dispositionCode: number): number {
  if (work.activePatchToken !== 0) { work.lastStatus = P011_HOT_PATCH_BUSY; work.patchBusyCount++; return 0; }
  const valid = requestIdentityHot(work, plan, row);
  if (valid !== 0) { work.lastStatus = valid; return 0; }
  if (!work.storageValidHot() || work.field === null || !fieldStorageValidHot(work, work.field, true)) { work.lastStatus = 3; return 0; }
  if (!Number.isFinite(grantedHeavy) || !Number.isFinite(grantedLight) || grantedHeavy < 0 || grantedLight < 0 ||
    grantedHeavy + grantedLight > plan.requested[row] + 1e-9 || !Number.isInteger(dispositionCode) || dispositionCode < 1 || dispositionCode > 6) {
    work.lastStatus = P011_HOT_INVALID_INPUT; return 0;
  }
  if (nextLeaseToken > Number.MAX_SAFE_INTEGER) { work.lastStatus = P011_HOT_TOKEN_EXHAUSTED; work.tokenExhausted = 1; return 0; }
  const copied = work.pool.transferHotState(work.patchBefore, false);
  if (copied !== 0) { work.lastStatus = copied; return 0; }
  copyPoolHot(work.patchBefore, work.patchScratch);
  work.patchActions.count = 0; work.patchActions.lineageCount = 0;
  copyRequestRowHot(plan, row, work.patchOriginal, 0); copyRequestRowHot(plan, row, work.patchRequest, 0);
  if (work.stagedYActive === 1) work.patchRequest.predictedY[0] = work.stagedY;
  const status = editPoolHot(work, work.patchRequest, 0, grantedHeavy, grantedLight, dispositionCode);
  if (status !== 0) {
    work.patchActions.count = 0; work.patchActions.lineageCount = 0; work.lastStatus = status;
    if (status === 9 || status === 10) work.overflowCount++;
    return 0;
  }
  copyPoolHot(work.patchScratch, work.patchAfter);
  const token = allocateParticleHotLease();
  work.activePatchToken = token; work.activePatchStepEpoch = work.stepEpoch; work.activePatchOrdinal = plan.ordinal[row];
  work.patchState = 1; work.patchPlan = plan; work.patchRow = row; work.lastStatus = 0;
  return token;
}
function patchLeaseHot(work: P011ParticleHotWorkspace, token: number, required: number): number {
  if (work.active !== 1 || !Number.isSafeInteger(token) || token <= 0 || token !== work.activePatchToken ||
    work.activePatchStepEpoch !== work.stepEpoch) { work.staleHandleRejectCount++; return P011_HOT_STALE_HANDLE; }
  if (work.patchState !== required) return P011_HOT_INVALID_PATCH_STATE;
  if (!work.storageValidHot() || work.field === null || !fieldStorageValidHot(work, work.field, true)) return P011_HOT_INVALID_BUFFER;
  return P011_HOT_OK;
}
function clearPatchHot(work: P011ParticleHotWorkspace): void {
  work.activePatchToken = 0; work.activePatchStepEpoch = 0; work.activePatchOrdinal = 0;
  work.patchState = 0; work.patchPlan = null; work.patchRow = -1;
}
export function publishPatchHot(work: P011ParticleHotWorkspace, token: number): number {
  const valid = patchLeaseHot(work, token, 1); if (valid !== 0) return work.lastStatus = valid;
  const status = work.pool.transferHotState(work.patchAfter, true); if (status !== 0) return work.lastStatus = status;
  work.patchState = 2; return work.lastStatus = 0;
}
export function restorePatchHot(work: P011ParticleHotWorkspace, token: number): number {
  const valid = patchLeaseHot(work, token, 2); if (valid !== 0) return work.lastStatus = valid;
  const status = work.pool.transferHotState(work.patchBefore, true); if (status !== 0) return work.lastStatus = status;
  clearPatchHot(work); return work.lastStatus = 0;
}
export function discardPatchHot(work: P011ParticleHotWorkspace, token: number): number {
  const valid = patchLeaseHot(work, token, 1); if (valid !== 0) return work.lastStatus = valid;
  clearPatchHot(work); return work.lastStatus = 0;
}
export function finishPatchHot(work: P011ParticleHotWorkspace, token: number): number {
  const valid = patchLeaseHot(work, token, 2); if (valid !== 0) return work.lastStatus = valid;
  const p = work.patchPlan; if (p === null) return work.lastStatus = P011_HOT_INVALID_PATCH_STATE;
  rememberRequestHot(work, work.patchOriginal, 0); clearPatchHot(work); return work.lastStatus = 0;
}

export function copyRequestRowHot(a: P011ParticleHotPlanBuffer, i: number, b: P011ParticleHotPlanBuffer, j: number): void {
  b.kind[j] = a.kind[i]; b.phase[j] = a.phase[i]; b.sourceKind[j] = a.sourceKind[i];
  b.column[j] = a.column[i]; b.targetColumn[j] = a.targetColumn[i]; b.particleId[j] = a.particleId[i];
  b.sourceId[j] = a.sourceId[i]; b.substep[j] = a.substep[i]; b.ordinal[j] = a.ordinal[i];
  b.requested[j] = a.requested[i]; b.totalWeight[j] = a.totalWeight[i];
  b.predictedX[j] = a.predictedX[i]; b.predictedY[j] = a.predictedY[i];
  b.predictedVx[j] = a.predictedVx[i]; b.predictedVy[j] = a.predictedVy[i];
  b.mergeStart[j] = j * 2; b.mergeCount[j] = a.mergeCount[i];
  for (let k = 0; k < a.mergeCount[i]; k++) b.lineage[j * 2 + k] = a.lineage[a.mergeStart[i] + k];
}

export function auditParticleHot(work: P011ParticleHotWorkspace): number {
  const f = work.field; if (f === null) return 1;
  const p = work.patchScratch; const status = work.pool.transferHotState(p, false); if (status !== 0) return status;
  work.auditMassH.fill(0); work.auditMassL.fill(0); work.auditSlots.fill(0);
  if (p.freeCount < 0 || p.freeCount > p.nmax || p.liveCount + p.freeCount !== p.nmax) return 13;
  for (let i = 0; i < p.freeCount; i++) {
    const slot = p.freeSlots[i]; if (slot < 0 || slot >= p.nmax || work.auditSlots[slot] !== 0 || p.pState[slot] !== 0) return 13;
    work.auditSlots[slot] = 1;
  }
  let live = 0;
  for (let i = 0; i < p.nmax; i++) if (p.pState[i] !== 0) {
    if (work.auditSlots[i] !== 0 || p.pState[i] > 2 || p.pPhase[i] > 1 || p.pSource[i] > 1 || p.pId[i] === 0 ||
      p.pSourceId[i] === 0 || !Number.isFinite(p.pMass[i]) || p.pMass[i] <= 0 || !Number.isFinite(p.pX[i]) ||
      p.pX[i] < 0 || p.pX[i] >= work.cols || !Number.isFinite(p.pY[i]) || p.pY[i] < 0 || p.pY[i] > work.capacity ||
      !Number.isFinite(p.pVx[i]) || !Number.isFinite(p.pVy[i])) return 13;
    const c = Math.floor(p.pX[i]); work.order[live++] = i;
    if (p.pPhase[i] === 0) work.auditMassH[c] += p.pMass[i]; else work.auditMassL[c] += p.pMass[i];
  }
  if (live !== p.liveCount) return 13;
  sortOrderHot(work.order, live, p);
  for (let i = 1; i < live; i++) if (p.pId[work.order[i - 1]] === p.pId[work.order[i]]) return 13;
  let beforeH = 0; let beforeL = 0; let afterH = 0; let afterL = 0;
  for (let c = 0; c < work.cols; c++) {
    const sand = f.bh[c] + f.bl[c] + f.mh[c] + f.ml[c];
    if (!Number.isFinite(sand) || !Number.isFinite(f.bh[c]) || !Number.isFinite(f.bl[c]) ||
      !Number.isFinite(f.mh[c]) || !Number.isFinite(f.ml[c]) || !Number.isFinite(f.suspH[c]) ||
      !Number.isFinite(f.suspL[c]) || !Number.isFinite(f.waterTop[c]) || f.bh[c] < -1e-9 || f.bl[c] < -1e-9 ||
      f.mh[c] < -1e-9 || f.ml[c] < -1e-9 || f.suspH[c] < -1e-9 || f.suspL[c] < -1e-9 ||
      !Number.isFinite(p.colParticleMassH[c]) || !Number.isFinite(p.colParticleMassL[c]) ||
      p.colParticleMassH[c] < -1e-9 || p.colParticleMassL[c] < -1e-9 ||
      Math.abs(work.auditMassH[c] - p.colParticleMassH[c]) > 1e-9 || Math.abs(work.auditMassL[c] - p.colParticleMassL[c]) > 1e-9 ||
      sand + f.suspH[c] + f.suspL[c] + p.colParticleMassH[c] + p.colParticleMassL[c] > f.waterTop[c] + 1e-9 ||
      f.waterTop[c] > work.capacity) return 13;
    if (work.mediumMode === 'DRY_AIR_V1' && (f.suspH[c] !== 0 || f.suspL[c] !== 0 ||
      Math.abs(f.waterTop[c] - sand - p.colParticleMassH[c] - p.colParticleMassL[c]) > 1e-9)) return 13;
    beforeH += work.fieldBeforeBh[c] + work.fieldBeforeMh[c] + work.fieldBeforeSuspH[c] + work.frozenPool.colParticleMassH[c];
    beforeL += work.fieldBeforeBl[c] + work.fieldBeforeMl[c] + work.fieldBeforeSuspL[c] + work.frozenPool.colParticleMassL[c];
    afterH += f.bh[c] + f.mh[c] + f.suspH[c] + p.colParticleMassH[c];
    afterL += f.bl[c] + f.ml[c] + f.suspL[c] + p.colParticleMassL[c];
  }
  if (work.budget !== null && work.budget.auditHot() !== 0) return 13;
  return Math.abs(afterH - beforeH - work.stepExternalH) > 1e-9 || Math.abs(afterL - beforeL - work.stepExternalL) > 1e-9 ? 13 : 0;
}
function failApplyHot(work: P011ParticleHotWorkspace, epoch: number, code: number, ordinal: number, stage: number): number {
  if(work.diagnosticEnabled&&work.diagnosticFirstError===0){work.diagnosticFirstError=code;work.diagnosticFirstErrorStage=stage;}
  work.failureStatus = code; work.failureOrdinal = ordinal; work.failureAttemptToken = work.attemptToken;
  const rolled = work.active === 1 ? rollbackStepHot(work, epoch) : 0;
  work.errorStageCode = stage; work.results.status = code;
  if (code === 7 || code === 8 || code === 9 || code === 10 || code === 11) work.overflowCount++;
  return work.lastStatus = rolled === 0 ? code : rolled;
}
export function applyParticlePlanInto(work: P011ParticleHotWorkspace, expectedStepEpoch: number,
  plan: P011ParticleHotPlanBuffer, sink: P011ParticleHotCommitSink, results: P011ParticleHotResultBuffer): number {
  if (work.active !== 1 || expectedStepEpoch !== work.stepEpoch) return work.lastStatus = 4;
  if (!work.storageValidHot() || work.field === null || !fieldStorageValidHot(work, work.field, true)) return work.lastStatus = 3;
  if (plan !== work.planA || sink !== work.commitSink || results !== work.results || !results.storageValidHot()) return work.lastStatus = 3;
  if (plan.consumed !== 0) return work.lastStatus = 15;
  if (compareParticlePlansHot(plan, work.planB) !== 0) return failApplyHot(work, expectedStepEpoch, 12, 0, 1);
  work.applying = 1;
  work.validationRows.fill(0);
  for (let i = 0; i < plan.requestCount; i++) {
    const valid = requestIdentityHot(work, plan, i); if (valid !== 0) return failApplyHot(work, expectedStepEpoch, valid, plan.ordinal[i], 1);
    const ordinal = plan.ordinal[i]; const previous = work.validationRows[ordinal];
    if (previous !== 0) return failApplyHot(work, expectedStepEpoch,
      sameRequestHot(plan, i, plan, previous - 1) ? 15 : 16, ordinal, 1);
    work.validationRows[ordinal] = i + 1;
  }
  if (work.budget !== null) work.budget.denominator = plan.totalRequested;
  results.ownerId = work.ownerId; results.stepEpoch = expectedStepEpoch; results.resultCount = 0;
  for (let i = 0; i < plan.requestCount; i++) {
    const status = sink.applyInto(work, expectedStepEpoch, plan, i, results);
    if (status !== 0) return failApplyHot(work, expectedStepEpoch, status, plan.ordinal[i], 2);
    if (results.committed[i] !== 1 || !Number.isFinite(results.granted[i]) || results.granted[i] < 0 ||
      !Number.isFinite(results.remainder[i]) || results.remainder[i] < 0 ||
      !Number.isFinite(results.budgetClipped[i]) || results.budgetClipped[i] < 0 ||
      !Number.isFinite(results.guardRejected[i]) || results.guardRejected[i] < 0 ||
      !Number.isFinite(results.capacityClipped[i]) || results.capacityClipped[i] < 0 ||
      !Number.isFinite(results.poolClipped[i]) || results.poolClipped[i] < 0 ||
      Math.abs(plan.requested[i] - results.granted[i] - results.budgetClipped[i] - results.guardRejected[i] -
        results.capacityClipped[i] - results.poolClipped[i]) > 1e-9) return failApplyHot(work, expectedStepEpoch, 13, plan.ordinal[i], 3);
    results.resultCount++;
  }
  const p = work.patchScratch;
  const copied = work.pool.transferHotState(p, false); if (copied !== 0) return failApplyHot(work, expectedStepEpoch, copied, 0, 4);
  for (let i = 0; i < plan.motionCount; i++) {
    const slot = particleSlotHot(p, plan.motionParticleId[i]);
    if (slot < 0 || Math.floor(p.pX[slot]) !== plan.motionColumn[i] || Math.floor(plan.motionX[i]) !== plan.motionColumn[i]) {
      return failApplyHot(work, expectedStepEpoch, 17, 0, 4);
    }
    p.pX[slot] = plan.motionX[i]; p.pY[slot] = plan.motionY[i]; p.pVx[slot] = plan.motionVx[i]; p.pVy[slot] = plan.motionVy[i];
  }
  const moved = work.pool.transferHotState(p, true); if (moved !== 0) return failApplyHot(work, expectedStepEpoch, moved, 0, 4);
  if(work.diagnosticEnabled){work.diagnosticJournalUsed=work.patchActions.count;
    work.diagnosticLineageUsed=work.patchActions.lineageCount;
    work.diagnosticFreeAfter=p.freeCount;work.diagnosticLiveAfter=p.liveCount;work.diagnosticNextIdAfter=p.nextId;}
  const audit = auditParticleHot(work); if (audit !== 0) return failApplyHot(work, expectedStepEpoch, audit, 0, 5);
  work.applying = 0; plan.consumed = 1; results.status = 0; return work.lastStatus = 0;
}
