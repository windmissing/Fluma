import { ClearWaterInitialState, ClearWaterStatus, ClearWaterAllocation } from './P011ClearWaterTypes';
// Independent phase-0 pool namespace. No legacy medium, M2 or transaction API.
export class ClearWaterPool {
  readonly pMass: Float64Array; readonly pX: Float64Array; readonly pY: Float64Array;
  readonly pVx: Float64Array; readonly pVy: Float64Array;
  readonly pPhase: Uint8Array; readonly pSource: Uint8Array; readonly pState: Uint8Array;
  readonly pId: Uint32Array; readonly pSourceId: Uint32Array; readonly freeSlots: Int32Array;
  readonly colParticleMassH: Float64Array = new Float64Array(128);
  readonly colParticleMassL: Float64Array = new Float64Array(128);
  readonly prev: Int32Array; readonly next: Int32Array;
  liveCount: number = 0; freeCount: number; nextId: number = 1; head: number = -1; tail: number = -1;
  readonly nmax: number;
  constructor(k: number){
    this.nmax=k;this.freeCount=k;this.pMass=new Float64Array(k);this.pX=new Float64Array(k);this.pY=new Float64Array(k);
    this.pVx=new Float64Array(k);this.pVy=new Float64Array(k);this.pPhase=new Uint8Array(k);this.pSource=new Uint8Array(k);this.pState=new Uint8Array(k);
    this.pId=new Uint32Array(k);this.pSourceId=new Uint32Array(k);this.freeSlots=new Int32Array(k);this.prev=new Int32Array(k);this.next=new Int32Array(k);this.clear();
  }
  clear(): void {
    this.pMass.fill(0);this.pX.fill(0);this.pY.fill(0);this.pVx.fill(0);this.pVy.fill(0);this.pPhase.fill(0);this.pSource.fill(0);this.pState.fill(0);
    this.pId.fill(0);this.pSourceId.fill(0);this.colParticleMassH.fill(0);this.colParticleMassL.fill(0);this.prev.fill(-1);this.next.fill(-1);
    for(let i=0;i<this.nmax;i++)this.freeSlots[i]=this.nmax-1-i;
    this.liveCount=0;this.freeCount=this.nmax;this.nextId=1;this.head=this.tail=-1;
  }
  copyFrom(p: ClearWaterPool): void {
    this.pMass.set(p.pMass);this.pX.set(p.pX);this.pY.set(p.pY);this.pVx.set(p.pVx);this.pVy.set(p.pVy);
    this.pPhase.set(p.pPhase);this.pSource.set(p.pSource);this.pState.set(p.pState);this.pId.set(p.pId);this.pSourceId.set(p.pSourceId);
    this.freeSlots.set(p.freeSlots);this.colParticleMassH.set(p.colParticleMassH);this.colParticleMassL.set(p.colParticleMassL);this.prev.set(p.prev);this.next.set(p.next);
    this.liveCount=p.liveCount;this.freeCount=p.freeCount;this.nextId=p.nextId;this.head=p.head;this.tail=p.tail;
  }
  birth(mass: number,x: number,y: number,sourceId: number): boolean {
    if(this.freeCount===0||this.nextId>0xffffffff)return false;
    const i=this.freeSlots[--this.freeCount];this.pMass[i]=mass;this.pX[i]=x;this.pY[i]=y;this.pVx[i]=this.pVy[i]=0;
    this.pPhase[i]=0;this.pSource[i]=1;this.pSourceId[i]=sourceId;this.pState[i]=1;this.pId[i]=this.nextId++;
    this.prev[i]=this.tail;this.next[i]=-1;if(this.tail<0)this.head=i;else this.next[this.tail]=i;this.tail=i;
    this.liveCount++;this.colParticleMassH[Math.floor(x)]+=mass;return true;
  }
  allocations(prefix: string): ClearWaterAllocation[] {
    const out: ClearWaterAllocation[]=[];
    const f64: Float64Array[]=[this.pMass,this.pX,this.pY,this.pVx,this.pVy,this.colParticleMassH,this.colParticleMassL];
    const names: string[]=['pMass','pX','pY','pVx','pVy','colParticleMassH','colParticleMassL'];
    for(let i=0;i<f64.length;i++)out.push({name:prefix+'.'+names[i],type:'Float64Array',length:f64[i].length,byteLength:f64[i].byteLength,lifetime:'COLD_RUN_FIXED'});
    const u8: Uint8Array[]=[this.pPhase,this.pSource,this.pState];const un: string[]=['pPhase','pSource','pState'];
    for(let i=0;i<u8.length;i++)out.push({name:prefix+'.'+un[i],type:'Uint8Array',length:u8[i].length,byteLength:u8[i].byteLength,lifetime:'COLD_RUN_FIXED'});
    const u32: Uint32Array[]=[this.pId,this.pSourceId];for(let i=0;i<2;i++)out.push({name:prefix+(i===0?'.pId':'.pSourceId'),type:'Uint32Array',length:this.nmax,byteLength:u32[i].byteLength,lifetime:'COLD_RUN_FIXED'});
    const i32: Int32Array[]=[this.freeSlots,this.prev,this.next];const ni: string[]=['freeSlots','prev','next'];
    for(let i=0;i<3;i++)out.push({name:prefix+'.'+ni[i],type:'Int32Array',length:this.nmax,byteLength:i32[i].byteLength,lifetime:'COLD_RUN_FIXED'});return out;
  }
}
export interface ClearWaterAStateJSON {
  W:Float64Array;bh:Float64Array;mh:Float64Array;S:Float64Array;eta:Float64Array;P:Float64Array;R:Float64Array;auditMass:Float64Array;
  initialWater:number;initialSand:number;I_water:number;I_sand:number;waterSuccessCount:number;sandSuccessCount:number;sourceCommit:number;physicsCommit:number;pool:ClearWaterPool;
}
export class ClearWaterState {
  // Preserve V1 cold JSON byte order/schema. Optional B metadata/bindings are
  // exported only by snapshotB(), never smuggled into the A serialized view.
  toJSON():ClearWaterAStateJSON {return {W:this.W,bh:this.bh,mh:this.mh,S:this.S,eta:this.eta,P:this.P,R:this.R,auditMass:this.auditMass,
    initialWater:this.initialWater,initialSand:this.initialSand,I_water:this.I_water,I_sand:this.I_sand,waterSuccessCount:this.waterSuccessCount,sandSuccessCount:this.sandSuccessCount,sourceCommit:this.sourceCommit,physicsCommit:this.physicsCommit,pool:this.pool};}
  private bBound: ArrayBuffer[]=[];
  bStorageBuffers():ArrayBuffer[] {return this.bBound;}
  bindBStorage():void {this.bBound=[this.W.buffer as ArrayBuffer,this.bh.buffer as ArrayBuffer,this.mh.buffer as ArrayBuffer,this.S.buffer as ArrayBuffer,this.eta.buffer as ArrayBuffer,this.P.buffer as ArrayBuffer,this.R.buffer as ArrayBuffer,this.auditMass.buffer as ArrayBuffer,this.pool.pMass.buffer as ArrayBuffer,this.pool.pX.buffer as ArrayBuffer,this.pool.pY.buffer as ArrayBuffer,this.pool.pVx.buffer as ArrayBuffer,this.pool.pVy.buffer as ArrayBuffer,this.pool.pPhase.buffer as ArrayBuffer,this.pool.pSource.buffer as ArrayBuffer,this.pool.pState.buffer as ArrayBuffer,this.pool.pId.buffer as ArrayBuffer,this.pool.pSourceId.buffer as ArrayBuffer,this.pool.freeSlots.buffer as ArrayBuffer,this.pool.colParticleMassH.buffer as ArrayBuffer,this.pool.colParticleMassL.buffer as ArrayBuffer,this.pool.prev.buffer as ArrayBuffer,this.pool.next.buffer as ArrayBuffer,this.originSourceCommit!.buffer as ArrayBuffer,this.waitReason!.buffer as ArrayBuffer,this.bHeader!.buffer as ArrayBuffer];}
  bStorageValid():boolean {return this.W.buffer===this.bBound[0]&&this.W.byteLength===128*8&&this.bh.buffer===this.bBound[1]&&this.bh.byteLength===128*8&&this.mh.buffer===this.bBound[2]&&this.mh.byteLength===128*8&&this.S.buffer===this.bBound[3]&&this.S.byteLength===128*8&&this.eta.buffer===this.bBound[4]&&this.eta.byteLength===128*8&&this.P.buffer===this.bBound[5]&&this.P.byteLength===128*8&&this.R.buffer===this.bBound[6]&&this.R.byteLength===128*8&&this.auditMass.buffer===this.bBound[7]&&this.auditMass.byteLength===128*8&&this.pool.pMass.buffer===this.bBound[8]&&this.pool.pMass.byteLength===this.pool.nmax*8&&this.pool.pX.buffer===this.bBound[9]&&this.pool.pX.byteLength===this.pool.nmax*8&&this.pool.pY.buffer===this.bBound[10]&&this.pool.pY.byteLength===this.pool.nmax*8&&this.pool.pVx.buffer===this.bBound[11]&&this.pool.pVx.byteLength===this.pool.nmax*8&&this.pool.pVy.buffer===this.bBound[12]&&this.pool.pVy.byteLength===this.pool.nmax*8&&this.pool.pPhase.buffer===this.bBound[13]&&this.pool.pPhase.byteLength===this.pool.nmax*1&&this.pool.pSource.buffer===this.bBound[14]&&this.pool.pSource.byteLength===this.pool.nmax*1&&this.pool.pState.buffer===this.bBound[15]&&this.pool.pState.byteLength===this.pool.nmax*1&&this.pool.pId.buffer===this.bBound[16]&&this.pool.pId.byteLength===this.pool.nmax*4&&this.pool.pSourceId.buffer===this.bBound[17]&&this.pool.pSourceId.byteLength===this.pool.nmax*4&&this.pool.freeSlots.buffer===this.bBound[18]&&this.pool.freeSlots.byteLength===this.pool.nmax*4&&this.pool.colParticleMassH.buffer===this.bBound[19]&&this.pool.colParticleMassH.byteLength===128*8&&this.pool.colParticleMassL.buffer===this.bBound[20]&&this.pool.colParticleMassL.byteLength===128*8&&this.pool.prev.buffer===this.bBound[21]&&this.pool.prev.byteLength===this.pool.nmax*4&&this.pool.next.buffer===this.bBound[22]&&this.pool.next.byteLength===this.pool.nmax*4&&this.originSourceCommit!.buffer===this.bBound[23]&&this.originSourceCommit!.byteLength===this.pool.nmax*8&&this.waitReason!.buffer===this.bBound[24]&&this.waitReason!.byteLength===this.pool.nmax*1&&this.bHeader!.buffer===this.bBound[25]&&this.bHeader!.byteLength===16*8;}
  originSourceCommit:Float64Array|null=null;waitReason:Uint8Array|null=null;bHeader:Float64Array|null=null;
  enableB():void {if(this.originSourceCommit!==null)throw new Error('CW_B_ALREADY_BOUND');this.originSourceCommit=new Float64Array(this.pool.nmax);this.waitReason=new Uint8Array(this.pool.nmax);this.bHeader=new Float64Array(16);}
  readonly W: Float64Array=new Float64Array(128);readonly bh: Float64Array=new Float64Array(128);readonly mh: Float64Array=new Float64Array(128);
  readonly S: Float64Array=new Float64Array(128);readonly eta: Float64Array=new Float64Array(128);readonly P: Float64Array=new Float64Array(128);readonly R: Float64Array=new Float64Array(128);
  readonly pool: ClearWaterPool;
  private readonly auditMass: Float64Array = new Float64Array(128);
  initialWater: number=0;initialSand: number=0;I_water: number=0;I_sand: number=0;
  waterSuccessCount: number=0;sandSuccessCount: number=0;sourceCommit: number=0;physicsCommit: number=0;
  constructor(k: number){this.pool=new ClearWaterPool(k);}
  reset(initial: ClearWaterInitialState | null): void {
    if(this.originSourceCommit!==null)this.originSourceCommit.fill(0);if(this.waitReason!==null)this.waitReason.fill(0);if(this.bHeader!==null)this.bHeader.fill(0);
    this.W.fill(0);this.bh.fill(0);this.mh.fill(0);this.pool.clear();
    if(initial!==null){this.W.set(initial.W);this.bh.set(initial.bh);this.mh.set(initial.mh);}
    this.I_water=this.I_sand=this.waterSuccessCount=this.sandSuccessCount=this.sourceCommit=this.physicsCommit=0;
    this.rebuild();this.initialWater=this.waterTotal();this.initialSand=this.sandTotal();
  }
  copyFrom(s: ClearWaterState): void {
    if(this.originSourceCommit!==null){if(s.originSourceCommit===null||s.waitReason===null||s.bHeader===null)throw new Error('CW_B_METADATA_REQUIRED');this.originSourceCommit.set(s.originSourceCommit);this.waitReason!.set(s.waitReason);this.bHeader!.set(s.bHeader);}
    this.W.set(s.W);this.bh.set(s.bh);this.mh.set(s.mh);this.S.set(s.S);this.eta.set(s.eta);this.P.set(s.P);this.R.set(s.R);this.pool.copyFrom(s.pool);
    this.initialWater=s.initialWater;this.initialSand=s.initialSand;this.I_water=s.I_water;this.I_sand=s.I_sand;
    this.waterSuccessCount=s.waterSuccessCount;this.sandSuccessCount=s.sandSuccessCount;this.sourceCommit=s.sourceCommit;this.physicsCommit=s.physicsCommit;
  }
  rebuild(): void {
    this.P.fill(0);let i=this.pool.head,count=0;
    while(i>=0&&i<this.pool.nmax&&count<this.pool.nmax){this.P[Math.floor(this.pool.pX[i])]+=this.pool.pMass[i];i=this.pool.next[i];count++;}
    for(let c=0;c<128;c++){this.S[c]=(((this.bh[c]+0)+this.mh[c])+0);this.eta[c]=this.S[c]+this.W[c];this.R[c]=this.eta[c]+this.P[c];}
  }
  waterTotal(): number {let v=0;for(let c=0;c<128;c++)v+=this.W[c];return v;}
  sandTotal(): number {
    let bed=0,mass=0;for(let c=0;c<128;c++)bed+=this.S[c];
    let i=this.pool.head,count=0;
    while(i>=0&&i<this.pool.nmax&&count<this.pool.nmax){mass+=this.pool.pMass[i];i=this.pool.next[i];count++;}
    return bed+mass;
  }
  audit(marks: Uint8Array, capacity: number): ClearWaterStatus {
    const p=this.pool;marks.fill(0);this.auditMass.fill(0);
    if(!Number.isSafeInteger(p.nextId)||p.nextId<1||p.nextId>0x100000000||!Number.isSafeInteger(p.liveCount)||!Number.isSafeInteger(p.freeCount)||p.freeCount<0||p.liveCount<0||p.liveCount+p.freeCount!==p.nmax)return ClearWaterStatus.AUDIT_FAILED;
    let i=p.head,prior=-1,id=0,count=0;
    while(i!==-1){if(!Number.isInteger(i)||i<0||i>=p.nmax||marks[i]||count>=p.nmax)return ClearWaterStatus.AUDIT_FAILED;
      marks[i]=1;count++;if(p.prev[i]!==prior||p.pId[i]<=id||p.pId[i]>=p.nextId||(p.pState[i]!==1&&p.pState[i]!==2)||p.pPhase[i]!==0||p.pSource[i]!==1||p.pSourceId[i]===0||
        !Number.isFinite(p.pMass[i])||p.pMass[i]<=0||!Number.isFinite(p.pX[i])||p.pX[i]<0||p.pX[i]>=128||!Number.isFinite(p.pY[i])||p.pY[i]<0||p.pY[i]>capacity||
        !Number.isFinite(p.pVx[i])||!Number.isFinite(p.pVy[i]))return ClearWaterStatus.AUDIT_FAILED;
      this.auditMass[Math.floor(p.pX[i])]+=p.pMass[i];id=p.pId[i];prior=i;i=p.next[i];}
    if(count!==p.liveCount||prior!==p.tail)return ClearWaterStatus.AUDIT_FAILED;
    for(let j=0;j<p.freeCount;j++){const slot=p.freeSlots[j];if(slot<0||slot>=p.nmax||marks[slot]||p.pState[slot]!==0)return ClearWaterStatus.AUDIT_FAILED;marks[slot]=1;}
    for(let c=0;c<128;c++)if(!Number.isFinite(this.W[c])||this.W[c]<0||!Number.isFinite(this.bh[c])||this.bh[c]<0||!Number.isFinite(this.mh[c])||this.mh[c]<0||
      !Number.isFinite(this.S[c])||this.S[c]!==(((this.bh[c]+0)+this.mh[c])+0)||this.eta[c]!==this.S[c]+this.W[c]||!Number.isFinite(this.P[c])||this.P[c]<0||
      this.P[c]!==this.auditMass[c]||this.P[c]!==p.colParticleMassH[c]||p.colParticleMassL[c]!==0||this.R[c]!==this.eta[c]+this.P[c]||!Number.isFinite(this.R[c])||this.R[c]>capacity)return ClearWaterStatus.AUDIT_FAILED;
    if(!Number.isFinite(this.I_water)||this.I_water<0||!Number.isFinite(this.I_sand)||this.I_sand<0||
      !Number.isSafeInteger(this.sourceCommit)||this.sourceCommit<0||!Number.isSafeInteger(this.physicsCommit)||this.physicsCommit<0||
      !Number.isSafeInteger(this.waterSuccessCount)||this.waterSuccessCount<0||!Number.isSafeInteger(this.sandSuccessCount)||this.sandSuccessCount<0)return ClearWaterStatus.AUDIT_FAILED;
    const ew=this.initialWater+this.I_water,es=this.initialSand+this.I_sand;
    if(!Number.isFinite(ew)||!Number.isFinite(es)||ew<0||es<0||Math.abs(this.waterTotal()-ew)/Math.max(ew,1e-9)>=0.001||Math.abs(this.sandTotal()-es)/Math.max(es,1e-9)>=0.001)return ClearWaterStatus.AUDIT_FAILED;
    return ClearWaterStatus.OK;
  }
  allocations(prefix: string): ClearWaterAllocation[] {
    const out=this.pool.allocations(prefix+'.pool'),arrays: Float64Array[]=[this.W,this.bh,this.mh,this.S,this.eta,this.P,this.R];
    const names: string[]=['W','bh','mh','S','eta','P','R'];for(let i=0;i<7;i++)out.push({name:prefix+'.'+names[i],type:'Float64Array',length:128,byteLength:arrays[i].byteLength,lifetime:'COLD_RUN_FIXED'});
    out.push({name:prefix+'.auditMass',type:'Float64Array',length:128,byteLength:1024,lifetime:'COLD_RUN_FIXED'});
    if(this.originSourceCommit!==null){out.push({name:prefix+'.originSourceCommit',type:'Float64Array',length:this.pool.nmax,byteLength:this.pool.nmax*8,lifetime:'COLD_RUN_FIXED'});out.push({name:prefix+'.waitReason',type:'Uint8Array',length:this.pool.nmax,byteLength:this.pool.nmax,lifetime:'COLD_RUN_FIXED'});out.push({name:prefix+'.bHeader',type:'Float64Array',length:16,byteLength:128,lifetime:'COLD_RUN_FIXED'});}return out;
  }
}
