import { P011ClearWaterEngine,ClearWaterBatchResult } from './P011ClearWaterEngine';
import { ClearWaterSourceRequest,ClearWaterSourceReceipt,ClearWaterStatus as S } from './P011ClearWaterTypes';
import { ClearWaterReceiptRead,ClearWaterReceiptQuery as CQ } from './P011ClearWaterCTypes';
import { ClearWaterFrameClock } from './P011ClearWaterFrameClock';
export class ClearWaterSourceController {
 readonly clock:ClearWaterFrameClock=new ClearWaterFrameClock();readonly ack:ClearWaterSourceReceipt=new ClearWaterSourceReceipt();readonly terminal:ClearWaterReceiptRead=new ClearWaterReceiptRead();readonly batch:ClearWaterBatchResult=new ClearWaterBatchResult();
 private readonly engine:P011ClearWaterEngine;private readonly configRev:number;private readonly rateSand:number;private readonly rateWater:number;
 private sampledWindow:number=-1;private pointer:number=-1;private gesture:number=0;private seq:number=0;private x:number=64.5;private y:number=200;kind:number=0;feedbackCount:number=0;private feedbackCommit:number=0;
 private readonly request:ClearWaterSourceRequest;
 constructor(engine:P011ClearWaterEngine,configRev:number,rateSand:number,rateWater:number){
  this.engine=engine;this.configRev=configRev;this.rateSand=rateSand;this.rateWater=rateWater;
  this.request={revision:'SAND_WATER_SOURCE_V1',runId:'',runGeneration:0,configRev:configRev,geometryEpoch:1,inputGeneration:1,gestureId:1,sourceId:1,requestSeq:1,kind:0,x:0,y:0,requestedVolume:0,timestamp:0,timestampUnit:'MS',seed:1,clockWindowId:1};
 }
 stop(reason:string,geometry:number=this.engine.geometryEpoch):S {this.pointer=-1;this.clock.reset();return this.engine.stopSourceGeneration(reason,this.engine.currentInputGeneration+1,geometry);}
 select(kind:number):void {if(kind!==0&&kind!==1)throw new Error('CW_SOURCE_KIND');this.stop('KIND_SWITCH');this.kind=kind;}
 down(pointer:number,x:number,y:number):boolean {if(this.pointer!==-1||!this.position(x,y))return false;this.stop('DOWN');this.pointer=pointer;this.gesture++;this.seq=0;return true;}
 move(pointer:number,x:number,y:number):boolean {return pointer===this.pointer&&this.position(x,y);}
 up(pointer:number):void {if(pointer===this.pointer)this.stop('UP');}
 private position(x:number,y:number):boolean {if(!Number.isFinite(x)||x<0||x>=128||!Number.isFinite(y)||y<0||y>256)return false;this.x=x;this.y=y;return true;}
 sample():S {
  if(this.pointer===-1||this.clock.seconds<=0||this.sampledWindow===this.clock.windowId)return S.OK;
  this.sampledWindow=this.clock.windowId;
  const e=this.engine,c=this.clock;const status=e.bindSourceClockWindow({id:c.windowId,previous:c.previous,current:c.current,unit:c.unit,inputGeneration:e.currentInputGeneration,geometryEpoch:e.geometryEpoch});if(status!==S.OK){this.stop('CLOCK_REJECTED');return status;}
  const r=this.request;r.runId=e.runId;r.runGeneration=e.runGeneration;r.inputGeneration=e.currentInputGeneration;r.geometryEpoch=e.geometryEpoch;r.gestureId=this.gesture;r.sourceId=this.gesture;r.requestSeq=++this.seq;r.kind=this.kind;r.x=this.x;r.y=this.y;r.requestedVolume=(this.kind===0?this.rateSand:this.rateWater)*c.seconds;r.timestamp=c.current;r.timestampUnit=c.unit;r.clockWindowId=c.windowId;
  if(r.requestedVolume<=0)return S.OK;
  const result=e.enqueueSourceInto(r,this.ack);if(result!==S.OK)return result;
  e.commitQueuedSourcesInto({runId:e.runId,runGeneration:e.runGeneration,configRev:this.configRev,inputGeneration:e.currentInputGeneration,geometryEpoch:e.geometryEpoch},this.batch);
  if(e.copySourceReceiptInto(r,this.terminal)===CQ.TERMINAL){const t=this.terminal.receipt;if((t.disposition==='ACCEPTED'||t.disposition==='PARTIAL_CAPACITY')&&t.admitted>0&&t.transactionId!==null&&t.kindSuccessSeq!==null&&t.sourceCommit>this.feedbackCommit){this.feedbackCount++;this.feedbackCommit=t.sourceCommit;}return t.status;}
  return S.PUBLICATION_INVALID;
 }
 tick(raw:number,unit:string,domain:string):boolean {
  if(this.clock.unit.length>0&&(this.clock.unit!==unit||this.clock.domain!==domain)){this.stop('CLOCK_DOMAIN_CHANGE');}
  const valid=this.clock.tick(raw,unit,domain);if(!valid&&this.clock.reason!=='FIRST_OR_CHANGED_CLOCK'){this.stop(this.clock.reason);}return valid;
 }
 reset():S {this.stop('RESET');const status=this.engine.resetClearWaterInto();this.feedbackCommit=0;this.feedbackCount=0;return status;}
}
