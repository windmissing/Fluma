import { P011ParticleSnapshot } from '../P011ParticleField';
import { P011ParticleHotWorkspace } from '../P011ParticleHot';
import { TxGpuMetadata } from './TxGpuMetadata';
import { TxGpuCompatWork,TxGpuContextV2,txSameNumber } from './TxGpuCompatWork';
import { TxGpuFrozen13 } from './TxGpuFrozen13';
export class TxPairResult {status:string='ERROR';slotA:number=-1;slotB:number=-1;errorCode:string='NOT_RUN';}
export interface TxTreeBackend {selectPair(p:P011ParticleSnapshot,radius:number,w:P011ParticleHotWorkspace):TxPairResult;}
export interface TxGpuConfig {
  nmax:number;shaderDirectory:string;runId:string;configId:string;sourceManifestSha256:string;buildManifestSha256:string;
  buildShaderSha256:string;queryShaderSha256:string;reduceShaderSha256:string;diagnosticOracle:boolean;fullDump:boolean;
  backend:string;policy:string;traceCapacity:number;variant:string;
}
export interface TxNativeReply {
  status:string;errorCode:string;slotA:number;slotB:number;ownerGenerationLo:number;ownerGenerationHi:number;contextEpochLo:number;contextEpochHi:number;liveBuffers:number;
  headerWords?:Uint32Array;resultWords?:Uint32Array;identityBytes?:Uint8Array;execStatus?:string;consumeStatus?:string;quarantined?:boolean;resourcesRetired?:boolean;dispatchCount?:number;
  nativeValidationCpuMs?:number;prepareValidateCpuMs?:number;prepareFreezeCpuMs?:number;prepareSortCpuMs?:number;prepareRangeCpuMs?:number;prepareHashPackCpuMs?:number;prepareCpuMs?:number;
  uploadCpuMs?:number;dispatchCpuMs?:number;fenceWaitCpuMs?:number;mapCpuMs?:number;requestCpuMs?:number;ownerConsumeCpuMs?:number;nativeReplyObjectCpuMs?:number;nativeReturnEncodeCpuMs?:number;
}
export interface TxNativeTransport {configure(config:TxGpuConfig):TxNativeReply;selectPair(p:P011ParticleSnapshot,radius:number,c:TxGpuContextV2,w:TxGpuCompatWork):TxNativeReply;reset():TxNativeReply;}
export function txUtf8Bytes(s:string):number {let n=0;for(let i=0;i<s.length;i++){const c=s.charCodeAt(i);if(c<128)n++;else if(c<2048)n+=2;else if(c>=0xd800&&c<=0xdbff&&i+1<s.length&&s.charCodeAt(i+1)>=0xdc00&&s.charCodeAt(i+1)<=0xdfff){n+=4;i++;}else n+=3;}return n;}
export function txShaBlocks(bytes:number):number {return Math.ceil((bytes+9)/64);}
function timing(v:number|undefined):number {return v===undefined?NaN:v;}
// Ledger rows use the same native 16 timings plus explicit real/compat keys,
// digest message lengths/blocks and transfer counts. No GPU timer inference.
export class TxPairBackends {
  readonly metadata:TxGpuMetadata=new TxGpuMetadata();readonly config:TxGpuConfig;
  readonly compat:TxGpuCompatWork=new TxGpuCompatWork();readonly context:TxGpuContextV2=new TxGpuContextV2();
  readonly frozen0:TxGpuFrozen13;readonly frozen1:TxGpuFrozen13;readonly rows:Float64Array;readonly result:TxPairResult=new TxPairResult();readonly cpuTotals:Float64Array=new Float64Array(4);readonly cpuCounts:Uint32Array=new Uint32Array(4);
  readonly errors:string[];readonly states:string[];readonly nativeRows:string[];count:number=0;state:string='COLD';firstError:string='';
  clock:()=>number=():number=>NaN;private tree:TxTreeBackend|null=null;
  attachTree(tree:TxTreeBackend):void {if(this.count!==0||this.metadata.rootId!==0||this.config.backend!=='TX_ADJ_AVL')throw new Error('TX_GPU_HOT_BACKEND_CHANGE');this.tree=tree;}
  private readonly route:string;
  readyReport():string {return JSON.stringify(this.ready);}
  private readonly transport:TxNativeTransport|null;private ready:TxNativeReply|null=null;private slot:number=0;
  constructor(config:TxGpuConfig,transport:TxNativeTransport|null){
    if(config.policy!=='TX_EXACT_STRUCTURE_V1')throw new Error('TX_GPU_UNSUPPORTED_POLICY');
    if(['LEGACY_SCAN','TX_ADJ_AVL','GPU_PAIR_V1'].indexOf(config.backend)<0||!Number.isSafeInteger(config.traceCapacity)||config.traceCapacity<=0)throw new Error('TX_GPU_CONFIG');
    this.config=JSON.parse(JSON.stringify(config)) as TxGpuConfig;this.route=config.backend;this.transport=transport;
    this.frozen0=new TxGpuFrozen13(config.nmax);this.frozen1=new TxGpuFrozen13(config.nmax);this.rows=new Float64Array(config.traceCapacity*64);this.rows.fill(NaN);
    this.nativeRows=new Array<string>(config.traceCapacity).fill('');this.errors=new Array<string>(config.traceCapacity).fill('');this.states=new Array<string>(config.traceCapacity).fill('NOT_RUN');
    if(config.backend==='GPU_PAIR_V1'){
      if(transport===null)throw new Error('TX_GPU_TRANSPORT_REQUIRED');this.ready=transport.configure(this.config);
      if(this.ready.status!=='READY'){this.state='QUARANTINED';this.firstError=this.ready.errorCode;return;}
    }this.state='CONFIGURED';
  }
  cpuScope(kind:number,ms:number):void {this.cpuCounts[kind]++;this.cpuTotals[kind]+=ms;}
  nowPublic():number {return this.now();}
  merged(status:number,ms:number):void {if(this.count>0){this.rows[(this.count-1)*64+57]=status;this.rows[(this.count-1)*64+58]=ms;}}
  private now():number {try{return this.clock();}catch(error){return NaN;}}
  beginRoot():void {if(this.state!=='CONFIGURED'&&this.state!=='CONSUMED')throw new Error('TX_GPU_OWNER_NOT_READY');this.metadata.beginRoot();}
  enterRole(role:number):void {this.metadata.enterRole(role);}
  cancel():void {this.metadata.cancel();this.state='STALE';}
  selectPair(p:P011ParticleSnapshot,radius:number,w:P011ParticleHotWorkspace):TxPairResult {
    if(this.state!=='CONFIGURED'&&this.state!=='CONSUMED')throw new Error('TX_GPU_NOT_REUSABLE:'+this.state);
    if(this.count>=this.config.traceCapacity){this.state='QUARANTINED';throw new Error('TX_GPU_TRACE_INCOMPLETE');}
    const row=this.count++,o=row*64;this.states[row]='ENTER';const start=this.now();
    try{
      this.metadata.bind(p,w);const m=this.metadata,c=this.context,b=this.compat;
      const realStep=w.stepEpoch,realAttempt=w.attemptToken,realRun=w.runToken,realSubstep=w.substepId,realParams=w.paramsRevision,realInput=w.inputRevision,realSeed=w.seed;
      b.bind(w,m.branchGeneration);const revision=m.contentRev,generation=m.generation,root=m.rootId,role=m.role,ordinal=m.ordinals[role];
      this.rows[o+16]=m.ownerId;this.rows[o+17]=root;this.rows[o+18]=m.viewSlot;this.rows[o+19]=m.branchGeneration;this.rows[o+20]=revision;this.rows[o+21]=realStep;this.rows[o+22]=realAttempt;this.rows[o+23]=realSubstep;this.rows[o+24]=b.stepEpoch;this.rows[o+25]=b.attemptToken;this.rows[o+26]=role;this.rows[o+27]=ordinal;this.rows[o+28]=m.requestSerial;this.rows[o+29]=this.slot;this.rows[o+52]=generation;this.rows[o+53]=realRun;this.rows[o+54]=realParams;this.rows[o+55]=realInput;this.rows[o+56]=realSeed;
      let r:TxPairResult;
      if(this.route==='LEGACY_SCAN')r=this.legacy(p,radius);
      else if(this.route==='TX_ADJ_AVL'){if(this.tree===null)throw new Error('TX_ADJ_AVL_NOT_IMPLEMENTED');r=this.tree.selectPair(p,radius,w);}
      else {
        const frozen=this.slot===0?this.frozen0:this.frozen1;const freezeStart=this.now();frozen.freeze(p);this.rows[o+30]=this.now()-freezeStart;this.state='SLOT_FROZEN';
        c.runId=this.config.runId+'|runToken:'+b.runToken.toString();c.stateId='owner:'+b.ownerId.toString()+'|step:'+b.stepEpoch.toString()+'|attempt:'+b.attemptToken.toString()+'|substep:'+b.substepId.toString();c.configId=this.config.configId;c.buildManifestSha256=this.config.buildManifestSha256;
        c.ownerGenerationLo=this.ready!.ownerGenerationLo;c.ownerGenerationHi=this.ready!.ownerGenerationHi;c.contextEpochLo=this.ready!.contextEpochLo;c.contextEpochHi=this.ready!.contextEpochHi;
        c.requestSerialLo=c.payloadSerialLo=m.requestSerial%4294967296;c.requestSerialHi=c.payloadSerialHi=Math.floor(m.requestSerial/4294967296);
        c.poolMutationSerialLo=revision%4294967296;c.poolMutationSerialHi=Math.floor(revision/4294967296);c.attemptSerialLo=b.attemptToken%4294967296;c.attemptSerialHi=Math.floor(b.attemptToken/4294967296);c.substepSerialLo=b.substepId%4294967296;c.substepSerialHi=Math.floor(b.substepId/4294967296);
        c.candidateRole=role;c.candidateOrdinal=ordinal;c.slotIndex=this.slot;c.paramsRevision=b.paramsRevision;c.inputRevision=b.inputRevision;c.seed=b.seed;
        this.state='SUBMITTED';const bridgeStart=this.now(),reply=this.transport!.selectPair(p,radius,c,b);this.rows[o+15]=this.now()-bridgeStart;this.state='FENCED';
        this.nativeTimes(o,reply);const encodeStart=this.now();this.nativeRows[row]=JSON.stringify(reply);this.rows[o+50]=this.now()-encodeStart;this.rows[o+51]=txUtf8Bytes(this.nativeRows[row]);
        if(reply.status!=='PAIR'&&reply.status!=='NO_PAIR'){this.state=reply.quarantined?'QUARANTINED':'STALE';throw new Error('TX_GPU_NATIVE:'+reply.errorCode);}
        if(c.runId!==this.config.runId+'|runToken:'+realRun.toString()||c.stateId!=='owner:'+m.ownerId.toString()+'|step:'+m.branchGeneration.toString()+'|attempt:'+m.branchGeneration.toString()+'|substep:'+realSubstep.toString()||c.configId!==this.config.configId||c.buildManifestSha256!==this.config.buildManifestSha256||c.candidateRole!==role||c.candidateOrdinal!==ordinal||c.slotIndex!==this.slot||c.requestSerialLo+c.requestSerialHi*4294967296!==m.requestSerial||c.payloadSerialLo+c.payloadSerialHi*4294967296!==m.requestSerial||c.poolMutationSerialLo+c.poolMutationSerialHi*4294967296!==revision||b.ownerId!==m.ownerId||b.runToken!==realRun||b.stepEpoch!==m.branchGeneration||b.attemptToken!==m.branchGeneration||b.substepId!==realSubstep||!txSameNumber(b.paramsRevision,realParams)||!txSameNumber(b.inputRevision,realInput)||!txSameNumber(b.seed,realSeed)||!m.active||m.generation!==generation||m.rootId!==root||m.contentRev!==revision||m.branchGeneration!==this.rows[o+19]||w.ownerId!==m.ownerId||w.stepEpoch!==realStep||w.attemptToken!==realAttempt||w.runToken!==realRun||w.substepId!==realSubstep||!txSameNumber(w.paramsRevision,realParams)||!txSameNumber(w.inputRevision,realInput)||!txSameNumber(w.seed,realSeed)||!frozen.same(p))throw new Error('TX_GPU_REAL_VIEW_STALE');
        this.rows[o+31]=55*p.nmax+16*p.cols;this.rows[o+32]=26; // bounded adapter view wrappers (not zero-allocation claim)
        let K=1,h=0;while(K<p.nmax){K*=2;h++;}
        const snapshotBytes=128+55*p.nmax+16*p.cols,payloadBytes=48*K;
        const revisions=['GPU_SEGMENTED_ID_PAIR_V1','CPU_RN64_MERGE_RANGE_V1','P011_GPU_PAIR_WIRE_V1','P011_GPU_PAIR_PROTOTYPE_V1'];let identityBytes=228;
        for(let j=0;j<revisions.length;j++)identityBytes+=4+txUtf8Bytes(revisions[j]);identityBytes+=12+txUtf8Bytes(c.runId)+txUtf8Bytes(c.stateId)+txUtf8Bytes(c.configId);
        if(reply.identityBytes!==undefined&&reply.identityBytes.byteLength!==identityBytes)throw new Error('TX_GPU_IDENTITY_LENGTH_MISMATCH');
        this.rows[o+33]=snapshotBytes;this.rows[o+34]=txShaBlocks(snapshotBytes);this.rows[o+35]=payloadBytes;this.rows[o+36]=txShaBlocks(payloadBytes);this.rows[o+37]=identityBytes;this.rows[o+38]=txShaBlocks(identityBytes);
        // Successful protocol replies prove prepare completed, not a GPU timer.
        const empty=reply.headerWords!==undefined?reply.headerWords[9]<2:p.liveCount<2;
        this.rows[o+39]=empty?0:256+16*K+32*K+128;this.rows[o+40]=reply.dispatchCount===undefined?NaN:reply.dispatchCount;this.rows[o+59]=empty?0:h+3;this.rows[o+41]=empty?0:1;this.rows[o+42]=reply.resultWords===undefined?NaN:reply.resultWords.byteLength;
        this.rows[o+43]=c.ownerGenerationLo;this.rows[o+44]=c.ownerGenerationHi;this.rows[o+45]=c.contextEpochLo;this.rows[o+46]=c.contextEpochHi;
        this.slot=1-this.slot;r=this.result;r.status=reply.status;r.slotA=reply.slotA;r.slotB=reply.slotB;r.errorCode=reply.errorCode;
      }
      if(r.status==='PAIR'&&!this.validPair(p,radius,r.slotA,r.slotB))throw new Error('TX_GPU_CPU_PREDICATE_REJECT');
      if(r.status!=='PAIR'&&r.status!=='NO_PAIR')throw new Error('TX_BACKEND_ERROR:'+r.errorCode);
      this.rows[o+47]=r.slotA;this.rows[o+48]=r.slotB;this.states[row]=r.status;this.state='CONSUMED';return r;
    }catch(error){const message=(error as Error).message;if(this.firstError.length===0)this.firstError=message;this.errors[row]=message;this.states[row]='FAULT';if(this.state!=='QUARANTINED')this.state='STALE';throw new Error(message);}
    finally{this.rows[o+49]=this.now()-start;}
  }
  private validPair(p:P011ParticleSnapshot,radius:number,a:number,b:number):boolean {return Number.isInteger(a)&&Number.isInteger(b)&&a>=0&&b>=0&&a<p.nmax&&b<p.nmax&&p.pState[a]!==0&&p.pState[b]!==0&&p.pId[a]<p.pId[b]&&p.pPhase[a]===p.pPhase[b]&&p.pSource[a]===p.pSource[b]&&p.pSourceId[a]===p.pSourceId[b]&&p.pState[a]===p.pState[b]&&Math.floor(p.pX[a])===Math.floor(p.pX[b])&&Math.abs(p.pY[a]-p.pY[b])<=radius;}
  private legacy(p:P011ParticleSnapshot,radius:number):TxPairResult {
    const r=this.result;r.status='NO_PAIR';r.slotA=r.slotB=-1;r.errorCode='NONE';let aid=Infinity,bid=Infinity;
    for(let a=0;a<p.nmax;a++)if(p.pState[a]!==0)for(let b=0;b<p.nmax;b++)if(this.validPair(p,radius,a,b)&&(p.pId[a]<aid||p.pId[a]===aid&&p.pId[b]<bid)){aid=p.pId[a];bid=p.pId[b];r.status='PAIR';r.slotA=a;r.slotB=b;}return r;
  }
  private nativeTimes(o:number,r:TxNativeReply):void {
    this.rows[o]=timing(r.nativeValidationCpuMs);this.rows[o+1]=timing(r.prepareValidateCpuMs);this.rows[o+2]=timing(r.prepareFreezeCpuMs);this.rows[o+3]=timing(r.prepareSortCpuMs);this.rows[o+4]=timing(r.prepareRangeCpuMs);this.rows[o+5]=timing(r.prepareHashPackCpuMs);this.rows[o+6]=timing(r.prepareCpuMs);this.rows[o+7]=timing(r.uploadCpuMs);this.rows[o+8]=timing(r.dispatchCpuMs);this.rows[o+9]=timing(r.fenceWaitCpuMs);this.rows[o+10]=timing(r.mapCpuMs);this.rows[o+11]=timing(r.requestCpuMs);this.rows[o+12]=timing(r.ownerConsumeCpuMs);this.rows[o+13]=timing(r.nativeReplyObjectCpuMs);this.rows[o+14]=timing(r.nativeReturnEncodeCpuMs);
  }
  retire():TxNativeReply|null {
    this.metadata.cancel();if(this.transport===null){this.state='RETIRED';return null;}
    const r=this.transport.reset();this.state=r.status==='RESET'&&r.liveBuffers===0?'RETIRED':'QUARANTINED';return r;
  }
}
