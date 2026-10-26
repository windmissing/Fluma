import { P013FrameInput, P013LightParams, P013FrameResult } from './P013RenderTypes';
// T54 isolated G2 API. No product page enables this controller.
import { P013PreparedMetadata } from './preparation/P013MaterialPreparation';
export enum P013MaterialMode { GLES_SAND_COMPOSITE_OFF = 4, GLES_SAND_COMPOSITE_ON = 5 }
export class P013MaterialBundleDesc extends P013PreparedMetadata {
  apiVersion: number = 2; sessionId: number = 0; surfaceEpoch: number = 0; contextEpoch: number = 0;
}
export interface P013MaterialStageReceipt {
  code: number; assetGeneration: number; state: string; receivedBytes: number; uploadedBytes: number;
  totalBytes: number; activeGeneration: number; pendingGeneration: number; appearanceConfigRevision: number; contextEpoch: number;
  errorStage: string; materialUploadBytes: number; materialPendingBytes: number; materialPumpCount: number;
  materialTextureLiveCount: number; assetCpuBytes: number; discardedGeneration: number; frameToken: string;
}
export interface P013MaterialNative {
  drawMaterialFrame(frame: P013MaterialFrameInput, tdataV2: Float32Array, light: P013LightParams,
    material: P013MaterialParams): P013MaterialFrameResult;
  stageMaterialBundle(desc: P013MaterialBundleDesc, grainRgba: Uint8Array, strataRgba: Uint8Array): P013MaterialStageReceipt;
  pumpMaterialUploads(byteBudget: number): P013MaterialStageReceipt;
  canActivate(generation: number, contextEpoch: number): boolean;
  activateMaterial(generation: number, frameToken: string): P013MaterialStageReceipt;
  discardMaterialStage(generation: number): P013MaterialStageReceipt;
}
export interface P013MaterialFallback {
  drawLegacyFull(frameToken: string): void;
  clearForeground(): void;
  hideEntireGles(): void;
}

// T55 G3: explicit layout2 entry; P0 diagnosticEnabled is not a sand draw switch.
export class P013MaterialFrameInput extends P013FrameInput {
  apiVersion: number = 2; assetGeneration: number = 0; contextEpoch: number = 0;
  frameToken: string = ''; lightEnabled: boolean = false;
  materialRecipeVersion: string = 'P1_ALBEDO_DELTA_V1';
  colorPipelineVersion: string = 'LIN_SRGB_REINHARD_V1';
  textureSamplingVersion: string = 'P1_TEXEL_CANVAS_PX_V1';
  strataSamplingVersion: string = 'CONTINUOUS_COLUMN_WARP_V1';
  constructor() { super(); this.T_dataLayoutVersion=2;this.slopeKernelVersion='E2_GAUSS_COL6_R18_P1_V2'; }
}
export class P013MaterialParams {
  paletteBaseSrgb: number[] = [145/255,168/255,111/255,216/255,196/255,150/255];
  grainBaseSrgb: number[] = [145/255,168/255,111/255,216/255,196/255,150/255,181/255,182/255,131/255];
  grainOffsetCanvasPx: number[] = [0,0,0]; bandMinYCanvasPx: number[] = [0,0,0]; bandMaxYCanvasPx: number[] = [0,0,0];
  canvasBox: number[] = [0,0,0,0]; canvasToSurfaceScaleX: number = 0; canvasToSurfaceScaleY: number = 0;
  aB: number = .75; aC: number = .5; aC3: number = .10;
  warpRangeCycles: number = 0; effectiveLambdaMaterial: number = 0; effectiveLambdaCanvasPx: number = 0;
  grainEnabled: boolean = true; strataEnabled: boolean = true; sparkEnabled: boolean = true;
}
export interface P013MaterialFrameResult extends P013FrameResult {
  apiVersion: number; requestedGeneration: number; activeGeneration: number; assetGeneration: number;
  frameToken: string; materialRecipeVersion: string; colorPipelineVersion: string;
  T_dataLayoutVersion: number; slopeKernelVersion: string; zOriginVersion: number;
  shaderHash: string; materialTdataBytes: number; stale: boolean;
  renderContractRevision: string; sceneTransactionImplemented: boolean; faultReason: string;
}
