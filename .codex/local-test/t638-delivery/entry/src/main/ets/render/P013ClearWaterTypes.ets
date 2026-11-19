import { P013FrameInput,P013NativeRenderer } from './P013RenderTypes';
import { ClearWaterSceneSlot } from '../physics/clearwater/P011ClearWaterCTypes';
export class ClearWaterNativeFrame extends P013FrameInput {
 scenePacketRevision:string='CW_SCENE_PACKET_V3';renderContractRevision:string='CW_NEWPIPE_OPTICS16_V1';routeId:string='single-sand-clear-water-v1';mediumMode:string='SINGLE_SAND_CLEAR_WATER_V1';
 configDigest:string='';payloadDigest:string='';
 contextEpoch:number=0;particleCount:number=0;particleCapacity:number=192;dpi:number=1;frameToken:string='';runId:string='';
 header64:Float64Array=new Float64Array(64);ledger64:Float64Array=new Float64Array(64);originSourceCommit64:Float64Array=new Float64Array(192);waitReason8:Uint8Array=new Uint8Array(192);
 bind(slot:ClearWaterSceneSlot,run:string):void {const h=slot.header64;this.payloadDigest=slot.payloadDigest;this.header64=h;this.ledger64=slot.ledger64;this.originSourceCommit64=slot.originSourceCommit64;this.waitReason8=slot.waitReason8;this.runId=run;this.particleCount=h[18];this.particleCapacity=h[19];this.snapshotId=h[2];this.fieldEpoch=h[3];this.geometryEpoch=h[9];this.appearanceConfigRevision=h[4];this.lightConfigRevision=h[36];this.capacity=256;this.T_dataLayoutVersion=3;
  // Full explicit identity; never replace this with a hash-only equality.
  this.frameToken=['CW_FRAME_TOKEN_C1',run,h[0],h[1],h[5],h[6],h[2],h[10],h[11],h[12],h[13],h[14],this.sessionId,this.contextEpoch,this.surfaceEpoch,this.geometryEpoch,this.surfaceWidthPx,this.surfaceHeightPx,this.dpi,this.visualInverted?1:0,this.boxXPx,this.boxYPx,this.boxWidthPx,this.boxHeightPx,h[4],h[35],h[36],h[32],h[33],h[34],h[38],h[39],h[15],h[16],h[17],this.frameId,this.configDigest,this.payloadDigest].join('|');
 }
}
export interface ClearWaterNativeReceipt {code:number;presentationState:string;firstFailureStage:string;currentFrameSucceeded:boolean;requestedSwapSucceeded:boolean;displayPresentVerified:boolean;requestedFrameToken:string;renderedFrameToken:string;lastGoodFrameToken:string;frameTextureBytes:number;nativePacketBytes:number;shaderHash:string;calibrationStatus:string;}
export interface ClearWaterNative extends P013NativeRenderer {drawClearWaterSceneV3(frame:ClearWaterNativeFrame,profile64:Float64Array,particleProof64:Float64Array,xy32:Float32Array,meta32:Uint32Array,optics64:Float64Array):ClearWaterNativeReceipt;}
