import { P011PublishedRenderState } from '../physics/P011PublishedRenderState';
import { SingleSandPacketV1, SingleSandReceipt } from './P013SingleSandTypes';
import { windowFixtureKey, inspectWindowFixture, WindowFixtureResult } from './RoiFixture';
import { P051DryAdmissionReceipt } from '../physics/P051DryParticleAdmission';

export class T394Land {
  readonly id:number;
  readonly column:number;
  readonly requested:number;
  readonly granted:number;
  constructor(id:number, column:number, requested:number, granted:number){
    this.id=id;
    this.column=column;
    this.requested=requested;
    this.granted=granted;
  }
}
export class T394Step {
  readonly lands:T394Land[]=[];
  landOverflow=false;
  readonly commit:number;
  readonly dt:number;
  readonly tilt:number;
  constructor(commit:number, dt:number, tilt:number){
    this.commit=commit;
    this.dt=dt;
    this.tilt=tilt;
  }
}
export class T394Input {
  readonly frameNs:number;
  readonly copyMs:number;
  readonly dt:number;
  readonly tilt:number;
  readonly steps:T394Step[];
  readonly admissions:P051DryAdmissionReceipt[];
  constructor(frameNs:number, copyMs:number, dt:number, tilt:number, steps:T394Step[], admissions:P051DryAdmissionReceipt[]){
    this.frameNs=frameNs;
    this.copyMs=copyMs;
    this.dt=dt;
    this.tilt=tilt;
    this.steps=steps;
    this.admissions=admissions;
  }
}
export class T394Hold {
  readonly displayStatus='DISPLAY_NOT_JUDGABLE';
  readonly sequence:number;
  readonly name:string;
  readonly requestId:number;
  readonly token:string;
  readonly frame:number;
  readonly width:number;
  readonly height:number;
  readonly geometry:string;
  readonly startWallMs:number;
  readonly startMonoMs:number;
  constructor(sequence:number, name:string, requestId:number, token:string, frame:number, width:number, height:number, geometry:string, startWallMs:number, startMonoMs:number){
    this.sequence=sequence;
    this.name=name;
    this.requestId=requestId;
    this.token=token;
    this.frame=frame;
    this.width=width;
    this.height=height;
    this.geometry=geometry;
    this.startWallMs=startWallMs;
    this.startMonoMs=startMonoMs;
  }
}
export class T394Archive {
  readonly revision='T394_SINGLE_SAND_PHASE_V1';
  readonly sequence:number;
  readonly name:string;
  readonly phaseStartedMs:number;
  readonly fixtureStatus:string;
  readonly firstFailure:string;
  readonly sample:T394Sample|null;
  readonly input:T394Input|null;
  readonly hold:T394Hold|null;
  readonly receipt:SingleSandReceipt|null;
  readonly prepareNativeCode:number|null;
  constructor(sequence:number, name:string, phaseStartedMs:number, fixtureStatus:string, firstFailure:string, sample:T394Sample|null, input:T394Input|null, hold:T394Hold|null, receipt:SingleSandReceipt|null, prepareNativeCode:number|null=null){
    this.sequence=sequence;
    this.name=name;
    this.phaseStartedMs=phaseStartedMs;
    this.fixtureStatus=fixtureStatus;
    this.firstFailure=firstFailure;
    this.sample=sample;
    this.input=input;
    this.hold=hold;
    this.receipt=receipt;
    this.prepareNativeCode=prepareNativeCode;
  }
}
class T394Particle {
  readonly id:number;
  readonly x:number;
  readonly y:number;
  readonly state:number;
  readonly mass:number;
  constructor(id:number, x:number, y:number, state:number, mass:number){
    this.id=id;
    this.x=x;
    this.y=y;
    this.state=state;
    this.mass=mass;
  }
}
export class T394Authority {
  readonly run:string;
  readonly epoch:number;
  readonly commit:number;
  readonly seal:number;
  readonly certificate:number;
  readonly sourceEpoch:number;
  readonly ledger:string;
  readonly bed:number[]=[];
  readonly particles:T394Particle[]=[];
  readonly bedMass:number;
  readonly inFlight:number;
  readonly sourceMass:number;
  readonly initialBed:number;
  constructor(state:P011PublishedRenderState){
    const f=state.field, pool=f.particleState, source=f.externalSourceState;
    this.run=state.runId;this.epoch=state.stateEpoch;this.commit=state.physicsCommitId;
    this.seal=state.derivedSeal?.serial??0;this.certificate=state.derivedCertificateId;
    this.sourceEpoch=source?.sourceEpoch??0;this.ledger=state.ledgerCommitKey===null?'':JSON.stringify(state.ledgerCommitKey);
    this.initialBed=source?.initialHeavy??0;this.sourceMass=source?.sourceInjectedH??0;
    let bed=0,flight=0;
    for(let c=0;c<128;c++){
      this.bed.push(f.sandTop[c]);bed+=f.sandTop[c];flight+=pool?.colParticleMassH[c]??0;
    }
    this.bedMass=bed;this.inFlight=flight;
    if(pool!==null&&pool!==undefined){
      for(let i=0;i<pool.nmax;i++){
        if(pool.pState[i]!==0&&(pool.pId[i]===1||pool.pId[i]===2))
          this.particles.push(new T394Particle(pool.pId[i],pool.pX[i],pool.pY[i],pool.pState[i],pool.pMass[i]));
      }
    }
  }
}

export function t394Geometry(p:SingleSandPacketV1):string{
  return [p.session,p.surface,p.context,p.geometry,p.width,p.height,p.x,p.y,p.w,p.h,p.inverted?1:0].join(':');
}
export function t394DynamicRoi(p:SingleSandPacketV1):Int32Array|null{
  const out=new Int32Array(12);
  const left=Math.ceil(Math.max(0,p.x)),right=Math.floor(Math.min(p.width,p.x+p.w));
  const base=p.height-p.y-p.h;
  const bottom=Math.ceil(Math.max(0,base)),top=Math.floor(Math.min(p.height,base+p.h));
  const side=Math.min(64,Math.max(16,2*Math.ceil(2*p.w/p.logicalW+3)));
  for(let i=0;i<3;i++){
    const col=i===0?32:i===1?96:64,y=i===2?200:128;
    const x=Math.floor(p.x+(p.inverted?1-col/128:col/128)*p.w)-side/2;
    const row=Math.floor(base+(p.inverted?1-y/256:y/256)*p.h)-side/2;
    if(!Number.isSafeInteger(x)||!Number.isSafeInteger(row)||x<left+4||row<bottom+4||
       x+side>right-4||row+side>top-4)return null;
    out.set([x,row,side,side],i*4);
  }
  return out;
}

export class T394Sample {
  readonly token:string;
  readonly fixtureKey:string;
  readonly fixture:WindowFixtureResult;
  readonly geometry:string;
  readonly width:number;
  readonly height:number;
  readonly frame:number;
  readonly digest:string;
  readonly rawHash:string;
  readonly displayHash:string;
  readonly pbHash:string;
  readonly particleHash:string;
  readonly authority:T394Authority;
  readonly displayBed:number[];
  readonly projectionXY:number[];
  readonly projectionMeta:number[];
  readonly copyMs:number;
  readonly packMs:number;
  readonly drawMs:number;
  readonly frameNs:number;
  constructor(p:SingleSandPacketV1, authority:T394Authority, fixture:WindowFixtureResult,
    profile:Float32Array,xy:Float32Array,meta:Uint32Array,copyMs:number,packMs:number,drawMs:number,frameNs:number){
    this.token=p.token;this.fixtureKey=windowFixtureKey(p);this.fixture=fixture;
    this.geometry=t394Geometry(p);this.width=p.width;this.height=p.height;this.frame=p.frame;
    this.digest=p.digest;this.rawHash=p.rawHash;this.displayHash=p.displayHash;this.pbHash=p.pbHash;this.particleHash=p.particleHash;
    this.authority=authority;this.displayBed=Array.from(profile.subarray(0,128));
    this.projectionXY=Array.from(xy.subarray(0,p.count*2));this.projectionMeta=Array.from(meta.subarray(0,p.count*2));
    this.copyMs=copyMs;this.packMs=packMs;this.drawMs=drawMs;this.frameNs=frameNs;
  }
}

export class T394Phase {
  sequence=0;
  name='LIVE';
  locked=false;
  startedMs=0;
  status='WAITING_LOCKED_FRAME';
  sample:T394Sample|null=null;
  firstFailure='';
  begin(name:string,locked:boolean,ms:number):void{
    this.sequence++;this.name=name;this.locked=locked;this.startedMs=ms;
    this.sample=null;this.firstFailure='';this.status='WAITING_LOCKED_FRAME';
  }
  reject(cut:string):void{
    this.sample=null;this.status=cut;
    if(this.firstFailure.length===0)this.firstFailure=cut;
  }
  accept(p:SingleSandPacketV1,r:SingleSandReceipt,state:P011PublishedRenderState,
    profile:Float32Array,xy:Float32Array,meta:Uint32Array,copyMs:number,packMs:number,drawMs:number,frameNs:number):T394Sample|null{
    if(!r.currentFrameSucceeded||r.presentationState!=='CURRENT'||!r.swapSucceeded||r.actualToken!==p.token||
       r.requestedToken!==p.token||p.run!==state.runId||p.epoch!==state.stateEpoch||p.commit!==state.physicsCommitId){
      this.reject('STALE_INPUT');return null;
    }
    const fixture=inspectWindowFixture(p,profile,xy,meta);
    const sample=new T394Sample(p,new T394Authority(state),fixture,profile,xy,meta,copyMs,packMs,drawMs,frameNs);
    this.sample=sample;this.status=fixture.status;
    return sample;
  }
}
