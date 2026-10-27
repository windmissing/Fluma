// Only imported by the test physics Worker. The UI never owns EGL or physics.
import gpu from 'libp011_gpu_pair_prototype.so';
import { P011ParticleSnapshot } from './P011ParticleField';
import { P011ParticleHotWorkspace } from './P011ParticleHot';
import { T159Binding, T159Pair } from './T159Selector';
import { t159Increment,t159Split,T159Words,t159PoolVersion,t159CandidateRole,t159CandidateOrdinal } from './T159Metadata';
export interface T159Config {
  nmax: number; shaderDirectory: string; buildManifestSha256: string; sourceManifestSha256: string;
  runId: string; configId: string; diagnosticOracle: boolean;
  buildShaderSha256: string; queryShaderSha256: string; reduceShaderSha256: string;
}
export interface T159NativeReply extends T159Pair {
  ownerGenerationLo: number; ownerGenerationHi: number; contextEpochLo: number; contextEpochHi: number;
  slot0Uses: number; slot1Uses: number; liveBuffers: number; peakBuffers: number;
}
export class T141RequestContextV2 {
  schemaRevision: string = 'T141_REQUEST_CONTEXT_V2';
  identityEncodingRevision: string = 'T138_CANONICAL_IDENTITY_LE_V2';
  runId: string = ''; stateId: string = ''; configId: string = ''; buildManifestSha256: string = '';
  ownerGenerationLo: number = 0; ownerGenerationHi: number = 0;
  contextEpochLo: number = 0; contextEpochHi: number = 0;
  payloadSerialLo: number = 0; payloadSerialHi: number = 0;
  poolMutationSerialLo: number = 0; poolMutationSerialHi: number = 0;
  attemptSerialLo: number = 0; attemptSerialHi: number = 0;
  substepSerialLo: number = 0; substepSerialHi: number = 0;
  requestSerialLo: number = 0; requestSerialHi: number = 0;
  candidateRole: number = 0; candidateOrdinal: number = 0; slotIndex: number = 0;
  paramsRevision: number = 0; inputRevision: number = 0; seed: number = 0;
}
export class T159NativeBinding implements T159Binding {
  readonly config: T159Config;
  readonly ready: T159NativeReply;
  private request: T159Words = new T159Words();
  private slot: number = 0;
  private active: boolean = false;
  readonly requestRows: string[] = [];
  constructor(config: T159Config) {
    this.config = config; this.ready = gpu.configure(config) as T159NativeReply;
    this.active = this.ready.status === 'READY';
  }
  selectPair(p: P011ParticleSnapshot,radius: number,w: P011ParticleHotWorkspace): T159Pair {
    if (!this.active) throw new Error('NATIVE_NOT_READY');
    t159Increment(this.request);
    const mutation = t159PoolVersion(p), attempt=t159Split(w.attemptToken), substep=t159Split(w.substepId);
    const c = new T141RequestContextV2();
    c.runId=this.config.runId+'|runToken:'+w.runToken.toString();
    c.stateId='owner:'+w.ownerId.toString()+'|step:'+w.stepEpoch.toString()+'|attempt:'+w.attemptToken.toString()+'|substep:'+w.substepId.toString();
    c.configId=this.config.configId; c.buildManifestSha256=this.config.buildManifestSha256;
    c.ownerGenerationLo=this.ready.ownerGenerationLo; c.ownerGenerationHi=this.ready.ownerGenerationHi;
    c.contextEpochLo=this.ready.contextEpochLo; c.contextEpochHi=this.ready.contextEpochHi;
    c.payloadSerialLo=this.request.lo; c.payloadSerialHi=this.request.hi;
    c.requestSerialLo=this.request.lo; c.requestSerialHi=this.request.hi;
    c.poolMutationSerialLo=mutation.lo; c.poolMutationSerialHi=mutation.hi;
    c.attemptSerialLo=attempt.lo; c.attemptSerialHi=attempt.hi;
    c.substepSerialLo=substep.lo; c.substepSerialHi=substep.hi;
    c.candidateRole=t159CandidateRole(); c.candidateOrdinal=t159CandidateOrdinal(); c.slotIndex=this.slot;
    c.paramsRevision=w.paramsRevision; c.inputRevision=w.inputRevision; c.seed=w.seed;
    this.slot=1-this.slot;
    const r = gpu.selectPair(p,radius,c,w) as T159NativeReply;
    // Bounded command size/steps also bound request rows; refuse rather than silently drop evidence.
    if (this.requestRows.length >= 16384) throw new Error('REQUEST_ARCHIVE_LIMIT');
    this.requestRows.push(JSON.stringify({context:c,result:r}));
    if (r.status !== 'PAIR' && r.status !== 'NO_PAIR') this.active=false;
    return r;
  }
  reset(): T159NativeReply { this.active=false; return gpu.reset() as T159NativeReply; }
}
