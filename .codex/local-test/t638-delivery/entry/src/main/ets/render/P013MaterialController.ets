import { P013LightParams } from './P013RenderTypes';
import { P013MaterialNative, P013MaterialConfigBinding, P013MaterialConfigReceipt, P013MaterialBundleDesc, P013MaterialStageReceipt, P013MaterialFallback, P013MaterialFrameInput, P013MaterialParams, P013MaterialFrameResult } from './P013MaterialTypes';
// T54: injected native boundary permits offline testing; no ticker or page ownership.
export class P013MaterialController {
  activeGeneration: number = 0; pendingGeneration: number = 0; fallbackReason: string = '';
  lastReceipt: P013MaterialStageReceipt | null = null;
  lastFrameReceipt: P013MaterialFrameResult | null = null;
  lastSuccessfulFrameToken: string = '';
  drawMaterialFrame(frame: P013MaterialFrameInput, tdataV2: Float32Array, light: P013LightParams,
    material: P013MaterialParams): P013MaterialFrameResult {
    const receipt=this.native.drawMaterialFrame(frame,tdataV2,light,material);
    if(receipt.requestedGeneration!==frame.assetGeneration || receipt.frameToken!==frame.frameToken ||
      (receipt.code===0 && receipt.swapSucceeded && receipt.assetGeneration!==frame.assetGeneration))
      throw new Error('MATERIAL_RECEIPT_IDENTITY');
    this.lastFrameReceipt=receipt;
    if(receipt.code===0 && receipt.swapSucceeded)this.lastSuccessfulFrameToken=frame.frameToken;
    return receipt;
  }
  private readonly native: P013MaterialNative;
  lastBinding: P013MaterialConfigReceipt | null = null;
  constructor(native: P013MaterialNative) { this.native=native; }
  bindMaterialConfiguration(binding: P013MaterialConfigBinding): P013MaterialConfigReceipt {
    const result=this.native.bindMaterialConfiguration(binding);if(result.code===0)this.lastBinding=result;return result;
  }
  private record(r: P013MaterialStageReceipt): P013MaterialStageReceipt {
    this.lastReceipt=r;this.activeGeneration=r.activeGeneration;
    this.pendingGeneration=r.pendingGeneration;
    return r;
  }
  stageMaterialBundle(desc: P013MaterialBundleDesc, grain: Uint8Array, strata: Uint8Array): P013MaterialStageReceipt {
    return this.record(this.native.stageMaterialBundle(desc,grain,strata));
  }
  pumpMaterialUploads(byteBudget: number): P013MaterialStageReceipt {
    return this.record(this.native.pumpMaterialUploads(byteBudget));
  }
  canActivate(generation: number, contextEpoch: number): boolean { return this.native.canActivate(generation,contextEpoch); }
  activateMaterial(generation: number, frameToken: string): P013MaterialStageReceipt {
    return this.record(this.native.activateMaterial(generation,frameToken));
  }
  discardMaterialStage(generation: number): P013MaterialStageReceipt { return this.record(this.native.discardMaterialStage(generation)); }
  // G4 must supply its real callbacks. Here only the isolated same-tick failure contract is implemented.
  fallback(frameToken: string, reason: string, target: P013MaterialFallback): void {
    this.fallbackReason=reason;
    throw new Error('NEWPIPE_FAULT_NO_CANVAS:'+frameToken+':'+reason);
  }
}
