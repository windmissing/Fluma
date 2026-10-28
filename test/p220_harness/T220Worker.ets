// T220 isolated clear-water B device harness worker (test-only).
import worker,{MessageEvents} from '@ohos.worker';
import fs from '@ohos.file.fs';
import util from '@ohos.util';
import systemDateTime from '@ohos.systemDateTime';
import { P011ClearWaterEngine,ClearWaterBatchResult } from '../physics/clearwater/P011ClearWaterEngine';
import { ClearWaterStatus,ClearWaterSourceReceipt,ClearWaterStepResult,ClearWaterSourceRequest } from '../physics/clearwater/P011ClearWaterTypes';
import { ClearWaterBoundaryIdentity,ClearWaterClockWindow } from '../physics/clearwater/P011ClearWaterEngine';
import { ClearWaterRunConfig } from '../physics/clearwater/P011ClearWaterTypes';
import { ClearWaterBConfig } from '../physics/clearwater/P011ClearWaterBTypes';
import { ClearWaterBAttachResult } from '../physics/clearwater/P011ClearWaterBTypes';
const port=worker.workerPort;
let active:boolean=false;
function now():number{return systemDateTime.getUptime(systemDateTime.TimeType.ACTIVE,false);}
class T220Config implements ClearWaterRunConfig{
  revision:string='CLEAR_WATER_RUN_CONFIG_V1';configRev:number=1;seed:number=1;cols:number=128;capacity:number=256;
  particleCapacity:number=192;massPackage:number=.125;kappaW:number=1;dragWet:number=1;settlingSpeed:number=1;
  rateSand:number=1;rateWater:number=1;maxSourceVolume:number=256;dtWindowPolicy:string='CW_ACTIVE_TICK_DT_V1';opticsRev:number=1;
  sigmaWater:number=0;tintLinear:number[]=[0,0,0];surfaceGain:number=0;queueCapacity:number=64;dedupCapacity:number=4096;
  eventCapacity:number=4096;calibrationStatus:string='PENDING_CALIBRATION';
}
class T220BConfig implements ClearWaterBConfig {
  revision:string='CW_B_CONFIG_V1';configRev:number=1;stepRevision:string='CW_B_STEP_V1';eventRevision:string='CW_B_EVENT40_V1';
  lineageRevision:string='CW_B_LINEAGE3_V1';priorityRevision:string='CW_B_SERIAL_PRIORITY_V1';contactRevision:string='CW_POINT_CONTACT_V1';
  accRevision:string='CW_MASS_ACCUM70_V1';gModel:number=9.8;dragAir:number=.01;maxAvalancheFlux:number=24;e:number=8;
  eventCapacity:number=0;lineageCapacity:number=0;calibrationStatus:string='PENDING_CALIBRATION';
}
class T220ClockWindow implements ClearWaterClockWindow{
  id:number=1;previous:number=0;current:number=16;unit:string='MS';inputGeneration:number=1;geometryEpoch:number=1;
}
class T220Boundary implements ClearWaterBoundaryIdentity{
  runId:string='';runGeneration:number=1;configRev:number=1;inputGeneration:number=1;geometryEpoch:number=1;
}
class T220PoolPart{liveCount:number=0;freeCount:number=0;nextId:number=0;}
class T220BudgetPart{totalUsed:number=0;totalRemaining:number=0;}
class T220Msg{
  status:string='';error:string='';runId:string='';output:string='';
}
class T220Row{
  op:string='';enqueueMs:number=0;status:number=0;admitted:number=0;kind:number=0;dtMs:number=0;angle:number=0;stepMs:number=0;
}
class T220Request implements ClearWaterSourceRequest{
  revision:string='SAND_WATER_SOURCE_V1';runId:string='';runGeneration:number=1;configRev:number=1;geometryEpoch:number=1;
  inputGeneration:number=1;gestureId:number=1;sourceId:number=1;requestSeq:number=1;kind:number=0;x:number=64.5;y:number=100;
  requestedVolume:number=.125;timestamp:number=16;timestampUnit:string='MS';seed:number=1;clockWindowId:number=1;
}
function writeLine(fd:number,text:string):void{
  const bytes=new util.TextEncoder().encodeInto(text+'\n');let offset:number=0;
  while(offset<bytes.byteLength){const n:number=fs.writeSync(fd,bytes.buffer.slice(offset));if(n<=0)throw new Error('T220_SHORT_WRITE');offset+=n;}
  fs.fsyncSync(fd);
}
function handle(ev:MessageEvents):void{
  const busy=new T220Msg();busy.status='FAULT';busy.error='BUSY';
  if(active){port.postMessage(busy);return;}
  active=true;let fd:number=-1;let fault:string='';
  const data=ev.data as Record<string,Object>;
  const serviceStart:number=now();
  try{
    const runId=data['runId'] as string;
    const scenario=(data['scenario'] as string)||'matrix';
    const k=(data['k'] as number)||192;
    const outPath='/data/storage/el2/base/haps/entry/files/t220-'+runId+'.jsonl';
    const f=fs.openSync(outPath,fs.OpenMode.CREATE|fs.OpenMode.READ_WRITE|fs.OpenMode.TRUNC);fd=f.fd;
    writeLine(fd,'{"phase":"COMMAND","runId":"'+runId+'","scenario":"'+scenario+'","k":'+k+',"receivedUptimeMs":'+now()+'}');
    const config=new T220Config();config.particleCapacity=k;
    const bcfg=new T220BConfig();bcfg.eventCapacity=8*k+254;bcfg.lineageCapacity=2*(8*k+254);
    const e=new P011ClearWaterEngine(config,null);
    const attach=new ClearWaterBAttachResult();
    const attachStatus=e.attachBInto(bcfg,attach);
    writeLine(fd,'{"phase":"ATTACH","status":'+attachStatus+',"payloadBytes":'+attach.payloadBytes+'}');
    if(attachStatus!==ClearWaterStatus.OK)throw new Error('T220_ATTACH_FAILED');
    const win=new T220ClockWindow();win.id=1;win.previous=0;win.current=16;win.unit='MS';
    win.inputGeneration=e.currentInputGeneration;win.geometryEpoch=e.geometryEpoch;
    const bindSt=e.bindSourceClockWindow(win);
    if(bindSt!==ClearWaterStatus.OK)throw new Error('T220_BIND_FAILED');
    let okSteps=0,failSteps=0;
    const rows:T220Row[]=[];
    const submit=(seq:number,kind:number,x:number,volume:number):ClearWaterSourceReceipt=>{
      const rq=new T220Request();rq.runId=e.runId;rq.runGeneration=e.runGeneration;rq.geometryEpoch=e.geometryEpoch;
      rq.inputGeneration=e.currentInputGeneration;rq.requestSeq=seq;rq.kind=kind;rq.x=x;rq.requestedVolume=volume;
      const out=new ClearWaterSourceReceipt();const t0=now();
      e.enqueueSourceInto(rq,out);
      const bnd=new T220Boundary();bnd.runId=e.runId;bnd.runGeneration=e.runGeneration;bnd.configRev=1;
      bnd.inputGeneration=e.currentInputGeneration;bnd.geometryEpoch=e.geometryEpoch;
      e.commitQueuedSourcesInto(bnd,new ClearWaterBatchResult());
      const row=new T220Row();row.op='source';row.enqueueMs=now()-t0;row.status=out.status;row.admitted=out.admitted;row.kind=kind;
      rows.push(row);
      return out;};
    const doStep=(dt:number,angle:number):void=>{
      const out=new ClearWaterStepResult();const t0=now();
      e.advanceSubstepInto(dt,angle,out);
      const row=new T220Row();row.op='step';row.dtMs=dt;row.angle=angle;row.stepMs=now()-t0;row.status=out.status;
      rows.push(row);
      if(out.status===ClearWaterStatus.OK&&out.stepped)okSteps++;else failSteps++;};
    if(scenario==='matrix'){
      for(let seq=1;seq<=4;seq++){const kinds=seq<=2?0:1;const volumes=[.5,200,1,255][(seq-1)%4];submit(seq,kinds,10+seq*8,volumes);}
      for(let s=0;s<32;s++)doStep(1/60,(s%8-4)*5);
      const bad=new ClearWaterSourceReceipt();
      const badSt=e.enqueueSourceInto(makeReq(e,99,0,200,1),bad);
      const row=new T220Row();row.op='invalid-source';row.status=badSt;
      rows.push(row);
      let guard=0;
      while(guard++<200){const o=submit(100+guard,0,20+guard%100,.125);if(o.status!==ClearWaterStatus.OK)break;}
    }else if(scenario==='land'){
      submit(1,0,64,.25);
      for(let s=0;s<200;s++)doStep(1/60,0);
    }else{
      submit(1,1,64,1);submit(2,0,64,.125);
      for(let s=0;s<32;s++)doStep(1/60,s%2?5:-5);
    }
    const snapB=e.snapshotB();const snap=snapB.A;
    const ledger=e.allocationLedger();
    const poolPart=new T220PoolPart();poolPart.liveCount=snap.state.pool.liveCount;poolPart.freeCount=snap.state.pool.freeCount;poolPart.nextId=snap.state.pool.nextId;
    const budgetPart=new T220BudgetPart();budgetPart.totalUsed=snapB.budgetScalars[3];budgetPart.totalRemaining=snapB.budgetScalars[2];
    const finalLine='{"phase":"FINAL","scenario":"'+scenario+'","okSteps":'+okSteps+',"failSteps":'+failSteps+
      ',"W":'+JSON.stringify(Array.from<number>(snap.state.W))+',"S":'+JSON.stringify(Array.from<number>(snap.state.S))+
      ',"I_water":'+snap.state.I_water+',"I_sand":'+snap.state.I_sand+
      ',"pool":'+JSON.stringify(poolPart)+',"budget":'+JSON.stringify(budgetPart)+
      ',"allocation":'+JSON.stringify(ledger)+',"budgetScalars":'+JSON.stringify(Array.from<number>(snapB.budgetScalars))+
      ',"arenaHeader":'+JSON.stringify(Array.from<number>(snapB.arenaHeader))+'}';
    writeLine(fd,finalLine);
    const rowsLine='{"phase":"SCENARIOS","rows":'+JSON.stringify(rows as T220Row[])+'}';
    writeLine(fd,rowsLine);
    writeLine(fd,'{"phase":"COMMAND_COMPLETE","status":"COMPLETE","error":"","serviceMs":'+(now()-serviceStart)+'}');
  }catch(error){fault=(error as Error).message;
    if(fd>=0){const esc=fault.replace(/"/g,'\\\"');try{writeLine(fd,'{"phase":"COMMAND_COMPLETE","status":"FAULT","error":"'+esc+'"}');}catch(e){}}
    const fm=new T220Msg();fm.status='FAULT';fm.error=fault;port.postMessage(fm);return;}
  finally{if(fd>=0)fs.closeSync(fd);}
  const cm=new T220Msg();cm.status='COMPLETE';port.postMessage(cm);
  active=false;
}
function makeReq(e:P011ClearWaterEngine,seq:number,kind:number,x:number,volume:number):ClearWaterSourceRequest{
  const rq=new T220Request();rq.runId=e.runId;rq.runGeneration=e.runGeneration;rq.geometryEpoch=e.geometryEpoch;
  rq.inputGeneration=e.currentInputGeneration;rq.requestSeq=seq;rq.kind=kind;rq.x=x;rq.requestedVolume=volume;
  return rq;}
port.onmessage=handle;
