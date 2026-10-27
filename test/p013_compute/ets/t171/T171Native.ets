// Only imported by the test physics Worker. The UI never owns EGL or physics.
import gpu from 'libp011_gpu_pair_prototype.so';
import { P011ParticleSnapshot } from './P011ParticleField';
import { P011ParticleHotWorkspace } from './P011ParticleHot';
import { T159Binding, T159Pair } from './T159Selector';
import { t159Increment,t159Split,T159Words,t159PoolVersion,t159CandidateRole,t159CandidateOrdinal } from './T159Metadata';
import { T159Config,T159NativeReply,T141RequestContextV2 } from './T159Native';
export interface T171FaultReply extends T159NativeReply {
 nativeValidationCpuMs:number;prepareValidateCpuMs:number;prepareFreezeCpuMs:number;prepareSortCpuMs:number;prepareRangeCpuMs:number;prepareHashPackCpuMs:number;prepareCpuMs:number;uploadCpuMs:number;dispatchCpuMs:number;fenceWaitCpuMs:number;mapCpuMs:number;requestCpuMs:number;ownerConsumeCpuMs:number;nativeReplyObjectCpuMs:number;nativeReturnEncodeCpuMs:number;
 resourcesRetired:boolean;execStatus:string;consumeStatus:string;rawGlError:number;quarantined:boolean;
 testConsumed:boolean;testOriginalIdentityHi:number;testModifiedIdentityHi:number;
}
export class T171NativeBinding implements T159Binding {
  readonly config: T159Config;
  readonly ready: T159NativeReply;
  private request: T159Words = new T159Words();
  private slot: number = 0;
  private active: boolean = false;
  readonly requestRows: string[] = [];
  archiveRequests:boolean=true;
  readonly bridgeMs:Float64Array=new Float64Array(16384);private bridgeCount:number=0;
  clock:()=>number=():number=>NaN;
  private requestStatus:Uint8Array=new Uint8Array(16384);private requestErrors:string[]=new Array<string>(16384).fill('');
  private timingRows:Float64Array=new Float64Array(16384*32);
  lastResult:T171FaultReply|null=null;
  private lastPool:P011ParticleSnapshot|null=null;private lastWork:P011ParticleHotWorkspace|null=null;
  private lastContext:T141RequestContextV2|null=null;private lastRadius:number=0;
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
    this.lastPool=p;this.lastWork=w;this.lastContext=c;this.lastRadius=radius;
    const bridgeStart=this.clock();const r = gpu.selectPair(p,radius,c,w) as T171FaultReply;this.lastResult=r;
    if(this.bridgeCount>=16384)throw new Error('BRIDGE_ARCHIVE_LIMIT');const row=this.bridgeCount++,o=row*32;this.requestStatus[row]=r.status==='PAIR'?1:r.status==='NO_PAIR'?0:2;this.requestErrors[row]=r.errorCode;this.bridgeMs[row]=this.clock()-bridgeStart;
    this.timingRows[o+0]=r.nativeValidationCpuMs;
    this.timingRows[o+1]=r.prepareValidateCpuMs;
    this.timingRows[o+2]=r.prepareFreezeCpuMs;
    this.timingRows[o+3]=r.prepareSortCpuMs;
    this.timingRows[o+4]=r.prepareRangeCpuMs;
    this.timingRows[o+5]=r.prepareHashPackCpuMs;
    this.timingRows[o+6]=r.prepareCpuMs;
    this.timingRows[o+7]=r.uploadCpuMs;
    this.timingRows[o+8]=r.dispatchCpuMs;
    this.timingRows[o+9]=r.fenceWaitCpuMs;
    this.timingRows[o+10]=r.mapCpuMs;
    this.timingRows[o+11]=r.requestCpuMs;
    this.timingRows[o+12]=r.ownerConsumeCpuMs;
    this.timingRows[o+13]=r.nativeReplyObjectCpuMs;
    this.timingRows[o+14]=r.nativeReturnEncodeCpuMs;
    this.timingRows[o+15]=this.bridgeMs[row];this.timingRows[o+16]=c.ownerGenerationLo;this.timingRows[o+17]=c.ownerGenerationHi;
    this.timingRows[o+18]=c.contextEpochLo;this.timingRows[o+19]=c.contextEpochHi;this.timingRows[o+20]=c.requestSerialLo;this.timingRows[o+21]=c.requestSerialHi;
    this.timingRows[o+22]=c.payloadSerialLo;this.timingRows[o+23]=c.payloadSerialHi;this.timingRows[o+24]=c.poolMutationSerialLo;this.timingRows[o+25]=c.poolMutationSerialHi;
    this.timingRows[o+26]=c.attemptSerialLo;this.timingRows[o+27]=c.attemptSerialHi;this.timingRows[o+28]=c.substepSerialLo;this.timingRows[o+29]=c.substepSerialHi;
    this.timingRows[o+30]=c.candidateRole;this.timingRows[o+31]=c.candidateOrdinal;
    // Bounded command size/steps also bound request rows; refuse rather than silently drop evidence.
    if (this.requestRows.length >= 16384) throw new Error('REQUEST_ARCHIVE_LIMIT');
    if(this.archiveRequests)this.requestRows.push(JSON.stringify({context:c,result:r}));
    if (r.status !== 'PAIR' && r.status !== 'NO_PAIR') this.active=false;
    return r;
  }
  beginAttempt():void{this.requestRows.length=0;this.bridgeCount=0;}
  statusSnapshot():Uint8Array{return this.requestStatus.slice(0,this.bridgeCount);}
  errorsSnapshot():string[]{return this.requestErrors.slice(0,this.bridgeCount);}
  timingSnapshot():Float64Array{return this.timingRows.slice(0,this.bridgeCount*32);}
  bridgeSnapshot():Float64Array{return this.bridgeMs.slice(0,this.bridgeCount);}
  unavailableProbe():T171FaultReply{
    if(this.lastPool===null||this.lastWork===null||this.lastContext===null)throw new Error('PROBE_WITHOUT_REQUEST');
    return gpu.selectPair(this.lastPool,this.lastRadius,this.lastContext,this.lastWork) as T171FaultReply;
  }
  arm(kind:string):T159NativeReply { return gpu.armTestInjectionV3({revision:'P011_GPU_TEST_INJECTION_V3',runId:this.config.runId,kind:kind}) as T159NativeReply; }
  reset(): T159NativeReply { this.active=false; return gpu.reset() as T159NativeReply; }
}
