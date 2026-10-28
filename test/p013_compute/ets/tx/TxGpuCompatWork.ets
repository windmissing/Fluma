// T212 / T205 §10. No native import, production switch or physical writer.
import { P011ParticleHotWorkspace } from '../P011ParticleHot';
export class TxGpuCompatWork {
  ownerId:number=0;runToken:number=0;stepEpoch:number=0;attemptToken:number=0;substepId:number=0;
  paramsRevision:number=0;inputRevision:number=0;seed:number=0;
  bind(w:P011ParticleHotWorkspace,branch:number):void {
    this.ownerId=w.ownerId;this.runToken=w.runToken;this.stepEpoch=branch;this.attemptToken=branch;this.substepId=w.substepId;
    this.paramsRevision=w.paramsRevision;this.inputRevision=w.inputRevision;this.seed=w.seed;
  }
}
export class TxGpuContextV2 {
  schemaRevision:string='T141_REQUEST_CONTEXT_V2';identityEncodingRevision:string='T138_CANONICAL_IDENTITY_LE_V2';
  runId:string='';stateId:string='';configId:string='';buildManifestSha256:string='';
  ownerGenerationLo:number=0;ownerGenerationHi:number=0;contextEpochLo:number=0;contextEpochHi:number=0;
  requestSerialLo:number=0;requestSerialHi:number=0;payloadSerialLo:number=0;payloadSerialHi:number=0;
  poolMutationSerialLo:number=0;poolMutationSerialHi:number=0;attemptSerialLo:number=0;attemptSerialHi:number=0;
  substepSerialLo:number=0;substepSerialHi:number=0;candidateRole:number=0;candidateOrdinal:number=0;slotIndex:number=0;
  paramsRevision:number=0;inputRevision:number=0;seed:number=0;
}
export function txSameNumber(a:number,b:number):boolean {return a===b&&(a!==0||1/a===1/b);}
export function txPositiveCounter(n:number):number {if(!Number.isSafeInteger(n+1))throw new Error('TX_GPU_CONTROL_EXHAUSTED');return n+1;}
