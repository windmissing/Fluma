// V3 request-only matrix. Synthetic snapshots are not public-injection transactions.
import systemDateTime from '@ohos.systemDateTime';
import { P011ParticleField } from './P011ParticleField';
import { configureHot } from './P011ParticleHot';
import { defaultP011ParticleParams } from './P011ParticleM2Field';
import { T171NativeBinding } from './T171Native';
import { T171Command,T171Config } from './T171Protocol';
import { t159ResetMetadata,t159Role } from './T159Metadata';
function now():number{return systemDateTime.getUptime(systemDateTime.TimeType.ACTIVE,false);}
export interface T171MicroCell {nmax:number;family:string;K:number;h:number;}
export function microCells():T171MicroCell[]{
 const cells:T171MicroCell[]=[];for(const nmax of [192,256,512])for(const family of ['PAIR','NO_PAIR'])cells.push({nmax:nmax,family:family,K:nmax<=256?256:512,h:nmax<=256?8:9});return cells;
}
export function runMicroV3(c:T171Command,emit:(text:string)=>void):void{
 const cells=microCells();
 // Three fixed warmups per cell, then six interleaved blocks x four requests/cell.
 for(let block=-1;block<6;block++)for(let index=0;index<6;index++){
  const cell=cells[(index+Math.max(0,block))%6],setupStart=now(),params=defaultP011ParticleParams();params.nmax=cell.nmax;
  const pool=new P011ParticleField(128,cell.nmax);
  for(let i=0;i<cell.nmax;i++)if(pool.allocate({mass:.1,x:64.5,y:100+(i%4)*.1,vx:0,vy:0,phase:0,sourceKind:0,sourceId:cell.family==='PAIR'?17:i+17})<0)throw new Error('MICRO_POOL_FAILED');
  const p=pool.snapshot(),work=configureHot({cols:128,nmax:cell.nmax,requestCapacity:2048,motionCapacity:cell.nmax,eventCapacity:2048,lineageCapacity:4096,params:params});
  work.runToken=1;work.seed=13;work.paramsRevision=1;work.inputRevision=1;t159ResetMetadata();
  const config:T171Config={nmax:cell.nmax,shaderDirectory:c.config.shaderDirectory,buildManifestSha256:c.config.buildManifestSha256,sourceManifestSha256:c.config.sourceManifestSha256,
   runId:c.config.runId,configId:c.config.configId+'|micro:'+cell.nmax.toString()+'|family:'+cell.family,diagnosticOracle:true,fullDump:true,
   buildShaderSha256:c.config.buildShaderSha256,queryShaderSha256:c.config.queryShaderSha256,reduceShaderSha256:c.config.reduceShaderSha256};
  const native=new T171NativeBinding(config);native.clock=now;
  try{
   emit(JSON.stringify({phase:'MICRO_READY',cell:cell,block:block,setupMs:now()-setupStart,poolSnapshot:p,groupCount:cell.family==='PAIR'?1:cell.nmax,mergeHeightRadius:params.mergeHeightRadius,yMin:p.pY[0],yMax:p.pY[3],result:native.ready,contextPolicy:'NEW_CONTEXT_PER_CELL_BLOCK_SETUP_OUTSIDE_REQUEST'}));
   if(native.ready.status!=='READY')throw new Error('MICRO_READY_FAILED');
   const count=block<0?3:4;
   for(let request=0;request<count;request++){
    t159Role(5);const started=now(),pair=native.selectPair(p,params.mergeHeightRadius,work),bridgeMs=now()-started;
    emit(JSON.stringify({phase:'MICRO_REQUEST',cell:cell,block:block,request:request,warmup:block<0,live:p.liveCount,uploadPayloadBytes:256+cell.K*16+cell.K*32+128,sourcePoolBytes:55*p.nmax+2048,
     result:pair,nativeRequest:native.requestRows[native.requestRows.length-1],bridgeMs:bridgeMs,gpuTimer:'NOT_MEASURED',fenceLabel:'CPU_CONDITION_WAIT_NOT_GPU_EXECUTION_TIME'}));
    if(pair.status!=='PAIR'&&pair.status!=='NO_PAIR')throw new Error('MICRO_REQUEST_FAILED');
   }
  }finally{const reset=native.reset();emit(JSON.stringify({phase:'MICRO_RESET',cell:cell,block:block,result:reset}));if(reset.status!=='RESET'||reset.liveBuffers!==0)throw new Error('MICRO_RETIREMENT_UNCONFIRMED');}
 }
}
