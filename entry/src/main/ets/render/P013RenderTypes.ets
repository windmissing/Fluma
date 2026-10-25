// P-013 P0 v1. Explicit synchronous bridge types; versions are evidence identity.
export enum P013Mode {
  CANVAS_BASELINE = 0,
  CANVAS_TOPLIGHT_REFERENCE = 1,
  GLES_LMAT_DIAG_OFF = 2,
  GLES_LMAT_DIAG_ON = 3
}
export class P013FrameInput {
  sessionId: number = 0;
  frameId: number = 0;
  snapshotId: number = 0;
  surfaceEpoch: number = 0;
  fieldEpoch: number = 0;
  geometryEpoch: number = 0;
  continuityEpoch: number = 0;
  appearanceConfigRevision: number = 0;
  lightConfigRevision: number = 0;
  frameTimeNs: number = 0;
  surfaceWidthPx: number = 0;
  surfaceHeightPx: number = 0;
  boxXPx: number = 0;
  boxYPx: number = 0;
  boxWidthPx: number = 0;
  boxHeightPx: number = 0;
  capacity: number = 0;
  visualInverted: boolean = false;
  diagnosticEnabled: boolean = false;
  T_dataLayoutVersion: number = 1;
  slopeKernelVersion: string = 'E2_APPROX_P0';
  zOriginVersion: number = 1;
}
export class P013LightParams {
  // Registered P012 E2 candidates; lref=1 is the existing prototype reference.
  // Bridge normalizes this direction; no numeric defaults are frozen by this class.
  la: number = 0.85;
  ld: number = 0.15;
  lmin: number = 0.80;
  lmax: number = 1.12;
  lref: number = 1;
  ellLightMaterial: number = 24;
  lightX: number = -0.55;
  lightY: number = -0.55;
  lightZ: number = 0.63;
  sigmaCandidate: number = 6;
}
export interface P013ControlResult {
  code: number;
  sessionId: number;
  state: string;
  surfaceEpoch: number;
  contextEpoch: number;
}
export interface P013RendererState extends P013ControlResult {
  widthPx: number;
  heightPx: number;
  sizeSource: string;
  active: boolean;
  ownerBound: boolean;
  lastErrorCode: number;
  nativeOwnedBytes: number;
  textureAbandonCount: number;
  programAbandonCount: number;
  vaoAbandonCount: number;
  textureLiveCount: number;
  programLiveCount: number;
  vaoLiveCount: number;
  telemetryLiveCount: number;
}
export interface P013FrameResult extends P013ControlResult {
  frameId: number;
  snapshotId: number;
  uploadedFrameId: number;
  submittedFrameId: number;
  swapSucceeded: boolean;
  lastSwapFrameId: number;
  eglError: number;
  glError: number;
  recoveryRequired: boolean;
  errorStage: string;
}
export interface P013TelemetryRecord extends P013FrameResult {
  schemaVersion: number;
  bridgeValidateMs: number;
  bridgeCopyMs: number;
  bridgeCpuMs: number;
  uploadCpuMs: number;
  uploadPathMs: number;
  uploadBytes: number;
  uploadCount: number;
  textureAllocCount: number;
  textureAbandonCount: number;
  programAbandonCount: number;
  vaoAbandonCount: number;
  textureLiveCount: number;
  texturePayloadBytes: number;
  eglInitMs: number;
  makeCurrentMs: number;
  unbindMs: number;
  swapMs: number;
  lifecycleLockWaitMs: number;
  shaderBuildMs: number;
  uniformCpuMs: number;
  drawCpuMs: number;
  gpuPassMs: number | null;
  gpuTimerValid: boolean;
  gpuDisjoint: boolean;
  shaderHash: string;
  lightConfig: P013LightParams;
  normalizedLightDirection: P013LightDirection;
  T_dataHash: string;
  compileLogHash: string;
  gpuTimerStatus: string;
  ownerThreadId: number;
  frameGap: number;
  mode: string;
  T_dataLayoutVersion: number;
  slopeKernelVersion: string;
  zOriginVersion: number;
  fieldEpoch: number;
  geometryEpoch: number;
  continuityEpoch: number;
  appearanceConfigRevision: number;
  lightConfigRevision: number;
  frameTimeNs: number;
  diagEncoding: string;
  surfaceCreateCount: number;
  surfaceDestroyCount: number;
  contextCreateCount: number;
  contextDestroyCount: number;
  contextLossCount: number;
  recoveryAttemptCount: number;
  staleLifecycleCount: number;
  programLiveCount: number;
  vaoLiveCount: number;
  nativeOwnedBytes: number;
  gpuQueryPendingCount: number;
}
export interface P013TelemetryBatch {
  records: P013TelemetryRecord[];
  droppedTelemetry: number;
  telemetryLiveCount: number;
}
export interface P013NativeRenderer {
  init(componentId: string): P013ControlResult;
  getState(): P013RendererState;
  setActive(active: boolean): P013ControlResult;
  drawFrame(frame: P013FrameInput, tdata: Float32Array, params: P013LightParams): P013FrameResult;
  release(): P013ControlResult;
  drainTelemetry(): P013TelemetryBatch;
}
// Per-attempt controller evidence is bounded just like native telemetry.
export class P013ControllerRecord {
  sessionId: number = 0;
  frameId: number = 0;
  snapshotId: number = 0;
  code: number = 0;
  packMs: number = 0;
  profileBuildMs: number = 0;
  profileCopyMs: number = 0;
  packedBytes: number = 2048;
  drawCallMs: number = 0;
  totalAddedCpuMs: number = 0;
  frameIntervalMs: number = 0;
  callbackStartIntervalMs: number = 0;
  diagnosticVisible: boolean = false;
  mode: number = 0;
  copy(): P013ControllerRecord {
    const out = new P013ControllerRecord();
    out.sessionId = this.sessionId;
    out.frameId = this.frameId;
    out.snapshotId = this.snapshotId;
    out.code = this.code;
    out.packMs = this.packMs;
    out.profileBuildMs = this.profileBuildMs;
    out.profileCopyMs = this.profileCopyMs;
    out.packedBytes = this.packedBytes;
    out.drawCallMs = this.drawCallMs;
    out.totalAddedCpuMs = this.totalAddedCpuMs;
    out.frameIntervalMs = this.frameIntervalMs;
    out.callbackStartIntervalMs = this.callbackStartIntervalMs;
    out.diagnosticVisible = this.diagnosticVisible;
    out.mode = this.mode;
    return out;
  }
}
export interface P013EvidenceBatch {
  native: P013TelemetryBatch;
  controller: P013ControllerRecord[];
  droppedControllerTelemetry: number;
  telemetryDrainMs: number;
}

export interface P013LightDirection {
  lightX: number;
  lightY: number;
  lightZ: number;
}
export interface P013Stats {
  n: number;
  p50: number | null;
  p95: number | null;
  max: number | null;
  failCount: number;
}
