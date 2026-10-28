export const CW_MEDIUM: string = 'SINGLE_SAND_CLEAR_WATER_V1';
export const CW_ROUTE: string = 'single-sand-clear-water-v1';
export const CW_SOURCE: string = 'SAND_WATER_SOURCE_V1';
export const CW_ROW_REVISION: string = 'CW_SOURCE_ROW_LE_V1';
export const CW_PENDING_CALIBRATION: string = 'PENDING_CALIBRATION';
export const CW_SOLID_TOTAL_PER_SECOND: number = 300;
export const CW_SOLID_LOCAL_PER_SECOND: number = 28;
export const CW_HARD_MAX: number = Math.tan(30*Math.PI/180);
export const CW_INVENTORY_EPS: number = 1e-9;
export const CW_DRIFT_LIMIT: number = 0.001;
export const CW_TRANSFER_ABS_LIMIT: number = 1e-6;
export enum ClearWaterStatus {
  OK=0, INVALID_CONFIG=1, INVALID_IDENTITY=2, INVALID_VALUE=3, STALE_RUN=4, STALE_INPUT_GENERATION=5,
  STALE_GEOMETRY=6, IDENTITY_CONFLICT=7, QUEUE_FULL=8, DEDUP_FULL=9, EVENT_FULL=10, POOL_FULL=11,
  CAPACITY_FULL=12, PARTICLE_RESERVED_CAPACITY=13, POSITION_IN_BED=14, NUMERIC_UNREPRESENTABLE=15,
  AUDIT_FAILED=16, TOKEN_EXHAUSTED=17, REENTRANT=18, UNSUPPORTED_FEATURE=19, PUBLICATION_INVALID=20,
  INVALID_TIME_WINDOW=21, PARTIAL_CAPACITY=22
}
export enum ClearWaterSourceKind { SAND=0, WATER=1 }
export interface ClearWaterRunConfig {
  revision: string; configRev: number; seed: number; cols: number; capacity: number; particleCapacity: number;
  massPackage: number; kappaW: number; dragWet: number; settlingSpeed: number; rateSand: number; rateWater: number;
  maxSourceVolume: number; dtWindowPolicy: string; opticsRev: number; sigmaWater: number;
  tintLinear: number[]; surfaceGain: number; queueCapacity: number; dedupCapacity: number; eventCapacity: number;
  calibrationStatus: string;
}
export interface ClearWaterInitialState { W: Float64Array; bh: Float64Array; mh: Float64Array; }
export interface ClearWaterSourceRequest {
  revision: string; runId: string; runGeneration: number; configRev: number; geometryEpoch: number;
  inputGeneration: number; gestureId: number; sourceId: number; requestSeq: number; kind: ClearWaterSourceKind;
  x: number; y: number; requestedVolume: number; timestamp: number; timestampUnit: string; seed: number;
  clockWindowId: number;
}
// Output objects are caller-owned and reusable. Null is never a valid success ID.
export class ClearWaterSourceReceipt {
  revision: string = CW_SOURCE; runId: string = ''; runGeneration: number = 0; configRev: number = 0;
  geometryEpoch: number = 0; inputGeneration: number = 0; gestureId: number = 0; sourceId: number = 0;
  requestSeq: number = 0; kind: number = 0; x: number = 0; y: number = 0; timestamp: number = 0;
  timestampUnit: string = ''; seed: number = 0; clockWindowId: number = 0;
  status: ClearWaterStatus = ClearWaterStatus.OK; disposition: string = 'PENDING'; replay: boolean = false;
  requested: number = 0; admitted: number = 0; rejected: number = 0;
  transactionId: number | null = null; kindSuccessSeq: number | null = null;
  sourceCommit: number = 0; physicsCommitId: number = 0;
  globalWaterBefore: number = 0; globalWaterAfter: number = 0; globalSandBefore: number = 0; globalSandAfter: number = 0;
  columnWaterBefore: number = 0; columnWaterAfter: number = 0; columnSandBefore: number = 0; columnSandAfter: number = 0;
  controlAdmitted: number = 0; actualWaterDelta: number = 0; actualSandDelta: number = 0;
  waterSuccessCount: number = 0; sandSuccessCount: number = 0; I_water: number = 0; I_sand: number = 0;
  sourceLedgerRev: number = 1; errorStage: number = 0;
}
export class ClearWaterStepResult {
  stepRevision:string='CW_B_STEP_V1';stepReason:number=0;stage:number=0;ordinal:number=0;lastBStepSerial:number=0;sourceCommitAtEntry:number=0;acceptedAngleDeg:number=0;arenaSerial:number=0;eventCount:number=0;lineageCount:number=0;maxResidual:number=0;allEdgesWithinH:boolean=false;
  status: ClearWaterStatus = ClearWaterStatus.OK; stepped: boolean = false; lastAcceptedCommit: number = 0;
  failedAttempt: number | null = null; implementation: string = 'NOT_IMPLEMENTED_B_COUPLING';
}
export interface ClearWaterAllocation {
  name: string; type: string; length: number; byteLength: number; lifetime: string;
}
export function cwSafePositive(v: number): boolean { return Number.isSafeInteger(v)&&v>0; }
// Only used on finite operands; distinguishes the two zero encodings without
// Object.is (not available in the target ArkTS standard library).
export function cwSameFiniteBits(a: number,b: number): boolean {return a===b&&(a!==0||1/a===1/b);}
export function cwConfigValid(c: ClearWaterRunConfig): boolean {
  return c.revision==='CLEAR_WATER_RUN_CONFIG_V1'&&cwSafePositive(c.configRev)&&cwSafePositive(c.seed)&&c.seed<=0xffffffff&&
    c.cols===128&&c.capacity===256&&cwSafePositive(c.particleCapacity)&&c.particleCapacity<=512&&
    Number.isFinite(c.massPackage)&&c.massPackage>0&&Number.isFinite(c.maxSourceVolume)&&c.maxSourceVolume>0&&
    Number.isFinite(c.kappaW)&&c.kappaW>=0&&Number.isFinite(c.dragWet)&&c.dragWet>=0&&Number.isFinite(c.settlingSpeed)&&c.settlingSpeed>=0&&
    Number.isFinite(c.rateSand)&&c.rateSand>=0&&Number.isFinite(c.rateWater)&&c.rateWater>=0&&
    c.dtWindowPolicy==='CW_ACTIVE_TICK_DT_V1'&&cwSafePositive(c.opticsRev)&&Number.isFinite(c.sigmaWater)&&c.sigmaWater>=0&&
    Number.isFinite(c.surfaceGain)&&c.surfaceGain>=0&&c.tintLinear.length===3&&c.tintLinear.every((v: number):boolean=>Number.isFinite(v)&&v>=0&&v<=1)&&
    cwSafePositive(c.queueCapacity)&&c.queueCapacity<=64&&cwSafePositive(c.dedupCapacity)&&c.dedupCapacity<=4096&&
    cwSafePositive(c.eventCapacity)&&c.eventCapacity<=4096&&c.calibrationStatus===CW_PENDING_CALIBRATION;
}
// Fixed wire offsets. Internal zero at ID slots encodes exported null.
export const CW_QUEUE_OFFSETS: string[] = ['runGeneration','configRev','geometryEpoch','inputGeneration','gestureId','sourceId','requestSeq','kind',
  'x','y','requestedVolume','rawTimestamp','unit','seed','enqueueOrdinal','queueState','receiptIndex','normalizedTimeMs','clockWindowId','reserved0'];
export const CW_DEDUP_OFFSETS: string[] = CW_QUEUE_OFFSETS.concat(['status','disposition','admitted','rejected','transactionId','kindSuccessSeq',
  'sourceCommit','physicsCommit','globalWaterBefore','globalWaterAfter','globalSandBefore','globalSandAfter','columnWaterBefore','columnWaterAfter',
  'columnSandBefore','columnSandAfter','controlAdmitted','actualWaterDelta','actualSandDelta','waterSuccessCount','sandSuccessCount','I_water','I_sand',
  'sourceLedgerRev','errorStage','reserved45','reserved46','reserved47','reserved48','reserved49','reserved50','reserved51','reserved52','reserved53','reserved54','reserved55']);
export const CW_EVENT_OFFSETS: string[] = ['transactionId','kind','column','controlAdmitted','actualWaterDelta','actualSandDelta',
  'firstParticleId','particleCount','sourceId','physicsCommit','enqueueOrdinal','kindSuccessSeq','columnWaterBefore','columnWaterAfter',
  'columnParticleBefore','columnParticleAfter','x','y','seed','massPackage','tailMass','fullPackageCount','sourceLedgerRev','reserved0'];
