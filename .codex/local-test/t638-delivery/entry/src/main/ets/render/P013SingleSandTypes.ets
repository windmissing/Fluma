export enum P013SingleSandPipeline {
  SINGLE_SAND_DUALSTATE_SCENE_V1 = 'SINGLE_SAND_DUALSTATE_SCENE_V1'
}
export const SINGLE_VERSIONS:string =
  'P013_SINGLE_SAND_CONFIG_V1|SINGLE_SAND_DUALSTATE_SCENE_V1|SINGLE_H_SAND_V1|DRY_H_ONLY_V1|' +
  'SINGLE_SAND_BOUNDED_LINEAR_V1|SINGLE_SAND_SRGB8_V1|SINGLE_GRAIN_R8_SIGNED_V1|' +
  'SINGLE_DISC_INNER_RIM_V1|BINARY_NATIVE_CENTER_V1|CONTAINER_LOGICAL_PX_V1|' +
  'E2_GAUSS_COL6_R18_P1_V2|LINEAR_DISPLAY_PREMULT_OVER_V1|T356_YCODE_DUALSTATE_V1|T364_RAW_GRAIN_V1';
export class SingleSandConfigV1 {
  versions:string=SINGLE_VERSIONS;
  session:number=0; revision:number=1;
  digest:string=''; grainHash:string=''; generatorHash:string='';
}
export class SingleSandGrainV1 {
  asset:string='SINGLE_GRAIN_R8_SIGNED_V1';
  session:number=0; generation:number=0; digest:string='';
}
export class SingleSandPacketV1 {
  packet:string='P013_SINGLE_SAND_SCENE_PACKET_V1';
  pipeline:string=P013SingleSandPipeline.SINGLE_SAND_DUALSTATE_SCENE_V1;
  material:string='SINGLE_H_SAND_V1'; medium:string='DRY_AIR_V1'; route:string='dual-field-particle-v1';
  projection:string='COLUMN_PRESERVING_F32_RENDER_V1';
  session:number=0; surface:number=0; context:number=0; geometry:number=0; continuity:number=0;
  epoch:number=0; commit:number=0; frame:number=0; certificate:number=0; seal:number=0;
  revision:number=0; generation:number=0; width:number=0; height:number=0; slots:number=0; count:number=0;
  run:string=''; token:string=''; kind:string=''; ledger:string='';
  digest:string=''; grainHash:string=''; rawHash:string=''; displayHash:string=''; pbHash:string=''; particleHash:string='';
  shaderHash:string='';
  x:number=0; y:number=0; w:number=0; h:number=0; logicalW:number=0; logicalH:number=0; capacity:number=0;
  inverted:boolean=false; enabled:boolean=false;
}
export interface SingleSandReceipt {
  revision:string; reason:string; presentationState:string; firstFailureStage:string;
  requestedToken:string; actualToken:string; currentFrameSucceeded:boolean; swapSucceeded:boolean;
  shaderHash:string; rawGL:number; rawEGL:number; uploadedBytes:number;
}
export interface SingleSandSurface {
  code:number; sizeSource:string; surfaceEpoch:number; contextEpoch:number; widthPx:number; heightPx:number;
}
export interface SingleSandNative {
  init(id:string):SingleSandInit;
  getState():SingleSandSurface;
  prepareDualFieldSurface(session:number,epoch:number):SingleSandSurface;
  registerSingleSandConfiguration(config:SingleSandConfigV1,values:Float64Array):SingleSandReceipt;
  stageSingleSandGrain(desc:SingleSandGrainV1,tile:Uint8Array):SingleSandReceipt;
  drawSingleSandScene(packet:SingleSandPacketV1,profile:Float32Array,xy:Float32Array,meta:Uint32Array):SingleSandReceipt;
}
export interface SingleSandInit {code:number;sessionId:number;}
