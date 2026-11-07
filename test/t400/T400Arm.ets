import { P051DryAdmissionReceipt } from '../P051DryParticleAdmission';

export class T400Birth {
  readonly slot:number;
  readonly id:number;
  readonly generation:number;
  readonly mass:number;
  readonly x:number;
  readonly y:number;
  constructor(slot:number,id:number,generation:number,mass:number,x:number,y:number){
    this.slot=slot;this.id=id;this.generation=generation;this.mass=mass;this.x=x;this.y=y;
  }
}

export class T400Arm {
  static readonly MAX_STEPS=8;
  static readonly MAX_CALLBACKS=16;
  static readonly MAX_WALL_MS=15000;
  readonly id:number;
  readonly run:string;
  readonly epoch:number;
  readonly startedMs:number;
  readonly mode:string;
  readonly fixture:string;
  wallMs=0;
  steps=0;
  callbacks=0;
  bound=false;
  seenId=false;
  admissionState:string;
  admission:P051DryAdmissionReceipt|null=null;
  births:T400Birth[]=[];
  terminal='';
  limit='';
  constructor(id:number,run:string,epoch:number,startedMs:number,mode:string,fixture:string){
    this.id=id;this.run=run;this.epoch=epoch;this.startedMs=startedMs;
    this.mode=mode;this.fixture=fixture;
    this.admissionState=fixture.length===0?'NOT_APPLICABLE':'NOT_ENTERED';
  }
  observe(now:number):void {
    if(Number.isFinite(now))this.wallMs=Math.max(this.wallMs,now-this.startedMs);
  }
  limitReached():string {
    if(this.wallMs>=T400Arm.MAX_WALL_MS)return 'WALL_15S';
    if(this.steps>=T400Arm.MAX_STEPS)return 'SUCCESSFUL_SUBSTEPS_8';
    if(this.callbacks>=T400Arm.MAX_CALLBACKS)return 'FOREGROUND_CALLBACKS_16';
    return '';
  }
  waitingStatus():string {
    if(this.terminal.length!==0)return this.terminal;
    if(!this.bound)return 'Z08_WAITING_BIND';
    if(this.admissionState==='NOT_ENTERED')return 'Z08_WAITING_ADMISSION';
    return this.seenId?'Z08_WAITING_T3':'Z08_WAITING_TARGET_ID';
  }
}
