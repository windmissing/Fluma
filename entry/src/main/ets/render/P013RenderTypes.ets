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
  nativeTypedByteLength?: number;
  nativeTypedByteOffset?: number;
  nativeBufferByteLength?: number;
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
  drawDualFieldFrame(frame: P013DualFieldFrame, tdata: Float32Array, particleXY: Float32Array,
    particleMeta: Uint32Array, visualParams: P013DualFieldVisualParams): P013DualFieldReceipt;
  requestComposition(profile: P013CompositionProfile): P013CompositionReceipt;
  queryCompositionProfile(): P013CompositionReceipt;
  captureProbe(frameToken: string, points: Float32Array): P013CompositionReceipt;
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

/** G0 profile, separate from the seven material algorithm identities. */
export class P013CompositionProfile {
  compositionRevision: string = 'P013_P1_TEXTURE_STACK_V1';
  componentType: string = 'TEXTURE'; colorPipelineVersion: string = 'LIN_SRGB_REINHARD_V1';
  alphaMode: string = 'BINARY_MASK'; hitMode: string = 'NONE';
  probeOnly: boolean = false;
}
export interface P013CompositionReceipt extends P013ControlResult {
  compositionRevision: string; frameToken: string; physicalSnapshotHash: string;
  requestedColorSpace: string; actualColorSpace: string; attachmentEncoding: string;
  eglAlphaSize: number; srgbWriteControlSupported: boolean; srgbWriteEnabled: boolean;
  profileStatus: string; errorStage: string; readbackStage: string; readbackStatus: string;
  shaderHash: string; shaderHashAlgorithm: string;
  probePoints: number[]; rgba: number[]; linearRgba8: number[]; srgbRgba8: number[];
  eglVersion: string; eglExtensions: string; glVersion: string; glExtensions: string;
  eglError: number; glError: number; packAlignment: number; rowDirection: string;
  viewportWidth: number; viewportHeight: number; drawFramebuffer: number; readFramebuffer: number;
}
/** Analytic expectations only; measured tolerances belong to tester, not the developer. */
export function p013ProbeSrgb(linear: number): number {
  return linear <= 0.0031308 ? linear * 12.92 : 1.055 * Math.pow(linear, 1 / 2.4) - 0.055;
}

export class P013DualFieldFrame extends P013FrameInput {
  dualFieldPacketRevision: string = 'P013_DUALFIELD_PACKET_V1';
  particleLayoutVersion: string = 'PARTICLE_XY_META_SOA_V1';
  renderPipelineId: string = 'DUALFIELD_BASIC_V1';
  colorPipelineVersion: string = 'BASIC_SRGB_DISPLAY_V1';
  runId: string = ''; stateEpoch: number = 0; physicsCommitId: number = 0;
  contextEpoch: number = 0;
  particleCount: number = 0; particleCapacity: number = 0;
  particleLayerEnabled: boolean = true;
}
export class P013DualFieldVisualParams {
  visualRevision: string = 'P012_EXISTING_PHASE_CANDIDATES_V1';
  radiusPx: number = 1;
  heavyR: number = 145 / 255; heavyG: number = 168 / 255; heavyB: number = 111 / 255;
  lightR: number = 216 / 255; lightG: number = 196 / 255; lightB: number = 150 / 255;
  particleHeavyR: number = 196 / 255; particleHeavyG: number = 168 / 255; particleHeavyB: number = 120 / 255;
  particleLightR: number = 226 / 255; particleLightG: number = 208 / 255; particleLightB: number = 168 / 255;
}
export interface P013DualFieldReceipt extends P013FrameResult {
  runId: string; stateEpoch: number; physicsCommitId: number;
  renderPipelineId: string; particleCount: number; particleUploadBytes: number;
}
