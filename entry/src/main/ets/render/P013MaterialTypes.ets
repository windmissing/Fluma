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
