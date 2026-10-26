import { P013FrameInput,P013NativeRenderer,P013ControlResult } from './P013RenderTypes';
import { P013MaterialNative,P013MaterialConfigReceipt } from './P013MaterialTypes';
export class P013SceneFrame extends P013FrameInput {
  scenePacketRevision: string='P013_SCENE_PACKET_V1';renderContractRevision: string='P013_NEWPIPE_ONLY_V1';
  contextEpoch: number=0;stateEpoch: number=0;physicsCommitId: number=0;requestedGeneration: number=0;
  frameToken: string='';runId: string='';ledgerCommitKey: string='';routeId: string='';mediumMode: string='WET_COLUMN_V1';sourceFailure: string='';
  particleCount: number=0;particleCapacity: number=1;particleLayerEnabled: boolean=true;
  constructor(){super();this.T_dataLayoutVersion=2;this.slopeKernelVersion='E2_GAUSS_COL6_R18_P1_V2';}
}
export interface P013SceneSource {
  frameToken: string;runId: string;ledgerCommitKey: string;stateEpoch: number;physicsCommitId: number;
  surfaceEpoch: number;contextEpoch: number;geometryEpoch: number;continuityEpoch: number;generation: number;configRevision: number;
}
export interface P013SceneReceipt {
  code: number;requestedPipelineId: string;renderPipelineId: string;renderContractRevision: string;compositionRevision: string;compositingDomain: string;
  presentationState: string;faultReason: string;firstFailureStage: string;currentFrameSucceeded: boolean;
  requestedSwapSucceeded: boolean;staleSwapSucceeded: boolean;displayPresentVerified: boolean;
  requestedFrameToken: string;requestedGeneration: number;requestedConfigRevision: number;
  renderedFrameToken: string|null;renderedGeneration: number;renderedConfigRevision: number;
  lastGoodFrameToken: string|null;lastGoodGeneration: number;lastGoodConfigRevision: number;
  lastGoodSourceEpochs: P013SceneSource|null;renderedSource: P013SceneSource|null;
  targetSurfaceEpoch: number;targetContextEpoch: number;actualPresentationGeneration: number;
  activeGeneration: number;pendingGeneration: number;frameTextureLiveCount: number;frameFboLiveCount: number;frameTextureBytes: number;
  materialTextureLiveCount: number;assetCpuBytes: number;particleCount: number;particleBufferBytes: number;
  particlePresentedSource: string;tracerDrawCount: number;materialShaderHash: string;
}
export interface P013SceneNative extends P013NativeRenderer,P013MaterialNative {
  registerSceneConfiguration(binding: P013MaterialConfigReceipt,values: Float64Array): P013ControlResult;
  drawNewPipelineFrame(frame: P013SceneFrame,data: Float32Array,xy: Float32Array,meta: Uint32Array,motion: Float64Array): P013SceneReceipt;
}
