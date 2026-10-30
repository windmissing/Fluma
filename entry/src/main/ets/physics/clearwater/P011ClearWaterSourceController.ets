import { P011ClearWaterEngine,ClearWaterBatchResult } from './P011ClearWaterEngine';
import { ClearWaterSourceRequest,ClearWaterSourceReceipt,ClearWaterStatus as S } from './P011ClearWaterTypes';
import { ClearWaterReceiptRead,ClearWaterReceiptQuery as CQ } from './P011ClearWaterCTypes';
import { ClearWaterFrameClock } from './P011ClearWaterFrameClock';
import { ClearWaterSourceTrace } from './P011ClearWaterSourceTrace';
export class ClearWaterSourceController {
 readonly trace:ClearWaterSourceTrace=new ClearWaterSourceTrace();
 sourceState:number=0;stopReason:string='';feedbackMessage:string='按下后开始供给；只累计连续有效帧';
 sampleCount:number=0;terminalCount:number=0;gestureValidSeconds:number=0;gestureSampledSeconds:number=0;
 readonly clock:ClearWaterFrameClock=new ClearWaterFrameClock();readonly ack:ClearWaterSourceReceipt=new ClearWaterSourceReceipt();readonly terminal:ClearWaterReceiptRead=new ClearWaterReceiptRead();readonly batch:ClearWaterBatchResult=new ClearWaterBatchResult();
 private readonly engine:P011ClearWaterEngine;private readonly configRev:number;private readonly rateSand:number;private readonly rateWater:number;
 private sampledWindow:number=-1;private pointer:number=-1;private gesture:number=0;private seq:number=0;private x:number=64.5;private y:number=200;kind:number=0;feedbackCount:number=0;private feedbackCommit:number=0;
 private readonly request:ClearWaterSourceRequest;
 constructor(engine:P011ClearWaterEngine,configRev:number,rateSand:number,rateWater:number){
  this.engine=engine;this.configRev=configRev;this.rateSand=rateSand;this.rateWater=rateWater;
  this.request={revision:'SAND_WATER_SOURCE_V1',runId:'',runGeneration:0,configRev:configRev,geometryEpoch:1,inputGeneration:1,gestureId:1,sourceId:1,requestSeq:1,kind:0,x:0,y:0,requestedVolume:0,timestamp:0,timestampUnit:'MS',seed:1,clockWindowId:1};
 }
 get configuredWaterRate():number {return this.rateWater;}
 private observe(event:number,beforePointer:number,beforeInput:number,reason:string,boundary:boolean=false,status:number=0,requested:number=0,admitted:number=0,rejected:number=0,raw:number=this.clock.current,previous:number=this.clock.previous,unit:string=this.clock.unit,domain:string=this.clock.domain):void {
  const v=this.trace.packet,c=this.clock;v.fill(0);v[1]=event;v[2]=previous;v[3]=raw;v[4]=raw-previous;v[5]=unit==='NS'?1:unit==='MS'?2:0;v[6]=c.seconds>0?1:0;v[7]=c.seconds;v[8]=c.windowId;
  v[9]=beforePointer;v[10]=this.pointer;v[11]=this.gesture;v[12]=beforeInput;v[13]=this.engine.currentInputGeneration;v[14]=this.engine.runGeneration;v[15]=this.engine.geometryEpoch;v[16]=this.sampleCount;v[17]=this.terminalCount;v[18]=this.engine.currentSourceCommit;v[19]=this.feedbackCount;
  v[20]=requested;v[21]=admitted;v[22]=rejected;v[23]=status;v[24]=this.kind;v[25]=this.gestureValidSeconds;v[26]=this.gestureSampledSeconds;v[27]=this.kind===0?this.rateSand:this.rateWater;v[28]=this.sourceState;v[29]=Number.isFinite(raw)&&Number.isFinite(previous)?1:0;v[30]=unit==='NS'&&Number.isSafeInteger(raw)&&Number.isSafeInteger(previous)?1:0;v[31]=v[4]*(unit==='MS'?1e6:1);
  this.trace.record(domain,reason,this.engine.runId,boundary);
 }
 private stopMessage(reason:string):string {
  if(reason==='GAP')return '已停源：帧间隔超窗；请松开后重新按下';
  if(reason==='UP')return '已停源：手指已抬起；重新按下开始';
  if(reason==='CANCEL')return '已停源：触摸已取消；请重新按下';
  if(reason==='BACKGROUND')return '已停源：页面进入后台；回来后请重新按下';
  if(reason==='CLOCK_DOMAIN_CHANGE'||reason==='BACKWARD_CLOCK'||reason==='INVALID_CLOCK')return '已停源：时钟发生变化（'+reason+'）；请重新按下';
  return '已停源：'+reason+'；请重新按下';
 }
 stop(reason:string,geometry:number=this.engine.geometryEpoch):S {
  const pointer=this.pointer,input=this.engine.currentInputGeneration;this.pointer=-1;
  // Latch the actual gesture terminal. Repeated no-pointer maintenance stops
  // still execute the original generation change, but cannot erase GAP/UP.
  if(pointer!==-1||this.sourceState!==2){this.sourceState=2;this.stopReason=reason;this.feedbackMessage=this.stopMessage(reason);}
  const result=this.engine.stopSourceGeneration(reason,this.engine.currentInputGeneration+1,geometry);
  this.observe(3,pointer,input,reason,pointer!==-1,result);this.clock.reset();return result;
 }
 select(kind:number):void {if(kind!==0&&kind!==1)throw new Error('CW_SOURCE_KIND');this.stop('KIND_SWITCH');this.kind=kind;}
 down(pointer:number,x:number,y:number):boolean {if(this.pointer!==-1||!this.position(x,y))return false;this.stop('DOWN');this.pointer=pointer;this.gesture++;this.seq=0;
  this.sourceState=1;this.stopReason='';this.feedbackMessage=this.kind===1?'正在供水：仅累计连续有效帧':'正在供沙：仅累计连续有效帧';this.gestureValidSeconds=0;this.gestureSampledSeconds=0;
  this.observe(1,-1,this.engine.currentInputGeneration,'DOWN',true);return true;}
 move(pointer:number,x:number,y:number):boolean {const before=this.pointer,valid=pointer===this.pointer&&this.position(x,y);this.observe(2,before,this.engine.currentInputGeneration,valid?'MOVE':'MOVE_IGNORED');return valid;}
 up(pointer:number):void {if(pointer===this.pointer)this.stop('UP');}
 private position(x:number,y:number):boolean {if(!Number.isFinite(x)||x<0||x>=128||!Number.isFinite(y)||y<0||y>256)return false;this.x=x;this.y=y;return true;}
 sample():S {
  if(this.pointer===-1||this.clock.seconds<=0||this.sampledWindow===this.clock.windowId)return S.OK;
  this.sampledWindow=this.clock.windowId;
  this.sampleCount++;this.gestureSampledSeconds+=this.clock.seconds;
  const e=this.engine,c=this.clock;const status=e.bindSourceClockWindow({id:c.windowId,previous:c.previous,current:c.current,unit:c.unit,inputGeneration:e.currentInputGeneration,geometryEpoch:e.geometryEpoch});if(status!==S.OK){this.stop('CLOCK_REJECTED');return status;}
  const r=this.request;r.runId=e.runId;r.runGeneration=e.runGeneration;r.inputGeneration=e.currentInputGeneration;r.geometryEpoch=e.geometryEpoch;r.gestureId=this.gesture;r.sourceId=this.gesture;r.requestSeq=++this.seq;r.kind=this.kind;r.x=this.x;r.y=this.y;r.requestedVolume=(this.kind===0?this.rateSand:this.rateWater)*c.seconds;r.timestamp=c.current;r.timestampUnit=c.unit;r.clockWindowId=c.windowId;
  if(r.requestedVolume<=0)return S.OK;
  this.observe(5,this.pointer,e.currentInputGeneration,'SAMPLE',false,0,r.requestedVolume);
  const result=e.enqueueSourceInto(r,this.ack);if(result!==S.OK)return result;
  e.commitQueuedSourcesInto({runId:e.runId,runGeneration:e.runGeneration,configRev:this.configRev,inputGeneration:e.currentInputGeneration,geometryEpoch:e.geometryEpoch},this.batch);
  if(e.copySourceReceiptInto(r,this.terminal)===CQ.TERMINAL){const t=this.terminal.receipt;if((t.disposition==='ACCEPTED'||t.disposition==='PARTIAL_CAPACITY')&&t.admitted>0&&t.transactionId!==null&&t.kindSuccessSeq!==null&&t.sourceCommit>this.feedbackCommit){this.feedbackCount++;this.feedbackCommit=t.sourceCommit;}this.terminalCount++;this.observe(6,this.pointer,e.currentInputGeneration,t.disposition,false,t.status,r.requestedVolume,t.admitted,t.rejected);return t.status;}
  return S.PUBLICATION_INVALID;
 }
 tick(raw:number,unit:string,domain:string):boolean {
  if(this.clock.unit.length>0&&(this.clock.unit!==unit||this.clock.domain!==domain)){this.stop('CLOCK_DOMAIN_CHANGE');}
  const previousRaw=this.clock.current;
  const valid=this.clock.tick(raw,unit,domain);if(valid&&this.pointer!==-1)this.gestureValidSeconds+=this.clock.seconds;
  this.observe(4,this.pointer,this.engine.currentInputGeneration,this.clock.reason,false,0,0,0,0,raw,previousRaw,unit,domain);if(!valid&&this.clock.reason!=='FIRST_OR_CHANGED_CLOCK'){this.stop(this.clock.reason);}return valid;
 }
 reset():S {this.stop('RESET');const status=this.engine.resetClearWaterInto();this.feedbackCommit=0;this.feedbackCount=0;return status;}
}
