// Isolated V3 only. Physical work always calls the original public transactions.
import worker,{MessageEvents} from '@ohos.worker';
import fs from '@ohos.file.fs';
import util from '@ohos.util';
import systemDateTime from '@ohos.systemDateTime';
import gpu from 'libp011_gpu_pair_prototype.so';
import { WaterSandField } from '../physics/WaterSandField';
import { defaultP011ParticleParams } from '../physics/P011ParticleM2Field';
import { T171NativeBinding,T171FaultReply } from '../physics/T171Native';
import { T159NativeReply } from '../physics/T159Native';
import { T171Command,fixtureFor,scheduleFor } from '../physics/T171Protocol';
import { t159Arm,t159Disarm } from '../physics/T159Selector';
import { t159ResetMetadata,t171ResetAttemptMetadata,t171RoleCounts } from '../physics/T159Metadata';
import { t171Evidence } from '../physics/T171Evidence';
import { runMicroV3 } from '../physics/T171Micro';
const port=worker.workerPort;let active=false;
function now():number{return systemDateTime.getUptime(systemDateTime.TimeType.ACTIVE,false);}
function writeLine(fd:number,text:string):void {
 const bytes=new util.TextEncoder().encodeInto(text+'\n');let offset=0;
 while(offset<bytes.byteLength){const n=fs.writeSync(fd,bytes.buffer.slice(offset));if(n<=0)throw new Error('V3_SHORT_WRITE');offset+=n;}fs.fsyncSync(fd);
}
port.onmessage=(event:MessageEvents):void=>{
 const c=event.data as T171Command,received=now();if(active){port.postMessage({status:'FAULT',error:'BUSY'});return;}
 active=true;let native:T171NativeBinding|null=null;let fd=-1;let fault='';const serviceStart=now();
 try {
  const fixture=fixtureFor(c),schedule=c.mode==='MICRO'?[]:scheduleFor(c);const f=fs.openSync(c.output,fs.OpenMode.CREATE|fs.OpenMode.READ_WRITE|fs.OpenMode.TRUNC);fd=f.fd;
  writeLine(fd,JSON.stringify({revision:c.revision,phase:'COMMAND',command:c,fixture:fixture,clock:'OHOS_UPTIME_ACTIVE_MS',nativeClock:'CXX_STEADY_CLOCK_MS_SEPARATE_DOMAIN',receivedUptimeMs:received,queueMs:received-c.queuedUptimeMs,gpuTimer:'NOT_MEASURED'}));
  if(c.mode==='MICRO'){runMicroV3(c,(text:string):void=>{writeLine(fd,text);});return;}
  t159ResetMetadata();t171Evidence.clock=now;let gpuSetupMs:number|null=null;
  if(c.fault==='OWNER_EXHAUSTION'){
   const arm=gpu.armTestInjectionV3({revision:'P011_GPU_TEST_INJECTION_V3',runId:c.config.runId,kind:c.fault}) as T159NativeReply;
   writeLine(fd,JSON.stringify({phase:'FAULT_ARM',result:arm}));if(arm.status!=='ARMED')throw new Error('V3_ARM_FAILED');
  }
  let gpuCold=true;
  for(let ordinal=0;ordinal<schedule.length;ordinal++){
   const run=schedule[ordinal];t171ResetAttemptMetadata();t171Evidence.reset();const buildStart=now();
   const field=new WaterSandField(128,512),params=defaultP011ParticleParams();params.nmax=fixture.nmax;if(JSON.stringify(params)!==JSON.stringify(fixture.parameters))throw new Error('V3_PARAMETER_MANIFEST_MISMATCH');
   if(!field.setParticleRoute(params).ok)throw new Error('V3_ROUTE_FAILED');field.setNaturalObservationRouteId('dual-field-particle-v1');
   const fixtureBuildMs=now()-buildStart,before=field.t171ArchiveState();let physicalMs:number|null=null,attemptError='',setupMs:number|null=null;
   try {
    if(run.route==='GPU'&&native===null){const setup=now();native=new T171NativeBinding(c.config);native.clock=now;native.archiveRequests=c.oracleMode==='ON';setupMs=now()-setup;gpuSetupMs=setupMs;
     writeLine(fd,JSON.stringify({phase:'READY',ordinal:ordinal,result:native.ready}));if(native.ready.status!=='READY')throw new Error('READY_FAILED:'+native.ready.errorCode);
     if(ordinal===0&&c.fault!=='NONE'&&c.fault!=='SHADER_SHA_MISMATCH'&&c.fault!=='OWNER_EXHAUSTION'){const arm=native.arm(c.fault);writeLine(fd,JSON.stringify({phase:'FAULT_ARM',result:arm}));if(arm.status!=='ARMED')throw new Error('V3_ARM_FAILED');}
    }
    if(native!==null)native.beginAttempt();
    t159Arm(run.route,run.route==='GPU'?native:null);const physicalStart=now();
    try{
     for(let input=0;input<2;input++){
      const heavy=new Float64Array(128),light=new Float64Array(128);heavy[fixture.column]=(input===0?fixture.nmax:fixture.packages)*fixture.massPackage;
      const receipt=field.injectGestureSand({perColumnHeavy:heavy,perColumnLight:light,sourceId:fixture.sourceId,sequence:fixture.sequences[input],inputTime:fixture.inputTimes[input],x:fixture.x,y:fixture.y,seed:fixture.seed});
      if(receipt===null||Math.abs(receipt.admittedH-heavy[fixture.column])>1e-10)throw new Error('V3_LEGAL_ADMISSION_FAILED');
     }
     for(let step=0;step<c.steps;step++){const result=field.step(1/60);if(!result.ok)throw new Error('V3_STEP_FAILED:'+String(result.errorCode));}
    }finally{physicalMs=now()-physicalStart;t159Disarm();}
   }catch(error){attemptError=(error as Error).message;t159Disarm();}
   const encodeStart=now(),after=field.t171ArchiveState(),common=t171Evidence.exportJson(c.oracleMode==='ON');
   const result=JSON.stringify({revision:'T171_ATTEMPT_V3',phase:'ATTEMPT',ordinal:ordinal,run:run,fixtureId:fixture.fixtureId,fixtureSha256:fixture.fixtureSha256,
    config:c.config,candidateRoleCalls:t171RoleCounts(),oracleMode:c.oracleMode,status:attemptError.length?'FAULT':'TRANSACTION_RETURNED',error:attemptError,synthetic:true,
    coldRouteSample:run.route==='GPU'?setupMs!==null:ordinal<3,fixtureBuildMs:fixtureBuildMs,coldGpuSetupMs:setupMs,retainedGpuSetupMs:gpuSetupMs,physicalAttemptMs:physicalMs,
    before:before,after:after,common:common,requestRows:run.route==='GPU'&&native!==null?native.requestRows:[],
    nativeRequestStatus:run.route==='GPU'&&native!==null?native.statusSnapshot():null,nativeRequestErrors:run.route==='GPU'&&native!==null?native.errorsSnapshot():null,
    nativeTimingStride:32,nativeTimings:run.route==='GPU'&&native!==null?native.timingSnapshot():null,
    bridgeMs:run.route==='GPU'&&native!==null?native.bridgeSnapshot():null,encodingPhase:'COLD_AFTER_PHYSICAL_RETURN',gpuTimer:'NOT_MEASURED'});
   if(run.route==='GPU')gpuCold=false;const encodeMs=now()-encodeStart,archiveStart=now();writeLine(fd,result);
   writeLine(fd,JSON.stringify({phase:'ARCHIVE_TIMING',ordinal:ordinal,encodingMs:encodeMs,writeAndFsyncMs:now()-archiveStart,scope:'PREVIOUS_ATTEMPT_LINE_ONLY'}));
   if(t171Evidence.overflow)throw new Error('V3_EVIDENCE_OVERFLOW_INVALID_MEASUREMENT');
   if(c.mode==='FAULT'&&ordinal===0&&c.fault!=='OWNER_EXHAUSTION'){
    const reply=native?.lastResult,ready=native?.ready as T171FaultReply;
    let observed=false;
    if(c.fault==='QUARANTINE_GL')observed=reply!==null&&reply!==undefined&&reply.execStatus==='GL_ERROR'&&reply.rawGlError!==0&&reply.quarantined&&reply.errorCode==='TEST_ILLEGAL_UPLOAD_TARGET';
    else if(c.fault==='STALE_REQUEST_HI')observed=reply!==null&&reply!==undefined&&reply.errorCode==='REQUEST_CONTEXT_OR_INPUT'&&reply.testConsumed;
    else if(c.fault==='STALE_CONSUME_HI')observed=reply!==null&&reply!==undefined&&reply.execStatus==='OK'&&reply.consumeStatus==='STALE'&&reply.testConsumed;
    else if(c.fault==='SHADER_SHA_MISMATCH')observed=ready!==undefined&&ready.status==='ERROR'&&ready.errorCode==='SHADER_SHA_MISMATCH';
    writeLine(fd,JSON.stringify({phase:'FAULT_OBSERVATION',kind:c.fault,observed:observed,result:reply,ready:ready,physicsBefore:before,physicsAfter:after}));
    if(!observed||native===null)throw new Error('V3_INJECTION_NOT_OBSERVED');
    if(c.fault==='QUARANTINE_GL'||c.fault==='STALE_CONSUME_HI'){const second=native.unavailableProbe();writeLine(fd,JSON.stringify({phase:'UNAVAILABLE_SECOND_REQUEST',result:second}));if(second.errorCode!=='BINDING_UNAVAILABLE')throw new Error('V3_SECOND_REQUEST_NOT_REFUSED');}
    const retired=native.reset();writeLine(fd,JSON.stringify({phase:'FAULT_RESET',result:retired}));if(retired.status!=='RESET'||retired.liveBuffers!==0)throw new Error('V3_FAULT_RESET_UNCONFIRMED');
    if(c.fault==='SHADER_SHA_MISMATCH')c.config.shaderDirectory=c.recoveryShaderDirectory;
    native=null;continue;
   }
   if(attemptError.length){
    fault='ATTEMPT_FAILURES_PRESENT';
    if(run.route==='GPU'&&native!==null){const reset=native.reset();writeLine(fd,JSON.stringify({phase:'FAILED_ATTEMPT_RESET',ordinal:ordinal,result:reset}));if(reset.status!=='RESET')throw new Error('V3_RETIREMENT_UNCONFIRMED');native=null;}
    // Retain the failed attempt; continue the fixed schedule without replacement.
   }
  }
 }catch(error){fault=(error as Error).message;}
 finally{
  t159Disarm();const cleanupStart=now();let cleanup:T159NativeReply|null=null;
  if(native!==null){cleanup=native.reset();if(cleanup.status!=='RESET'||cleanup.liveBuffers!==0)fault+='|CLEANUP_UNCONFIRMED';}const cleanupMs=now()-cleanupStart;
  if(fd>=0){
   try{writeLine(fd,JSON.stringify({phase:'CLEANUP',result:cleanup,cleanupMs:cleanupMs}));
    if(c.fault==='OWNER_EXHAUSTION'){const exhausted=gpu.configure(c.config) as T159NativeReply;writeLine(fd,JSON.stringify({phase:'OWNER_EXHAUSTION_SECOND_CONFIG',result:exhausted,recovery:'NEW_OS_PROCESS_REQUIRED_NOT_ENV_RESET'}));if(exhausted.errorCode!=='OWNER_GENERATION_EXHAUSTED')throw new Error('V3_EXHAUSTION_NOT_OBSERVED');}
    writeLine(fd,JSON.stringify({phase:'COMMAND_COMPLETE',error:fault,status:fault.length?'FAULT':'COMPLETE',serviceThroughCleanupMs:now()-serviceStart,commandThroughCleanupMs:now()-c.queuedUptimeMs,completionWallMs:Date.now(),uiReceive:'NOT_YET_MEASURED'}));
   }catch(error){fault+='|ARCHIVE_FAILED:'+(error as Error).message;}finally{fs.closeSync(fd);}
  }
  port.postMessage({revision:'T171_UI_REPORT_V3',status:fault.length?'FAULT':'COMPLETE',error:fault,output:c.output,queuedUptimeMs:c.queuedUptimeMs,postedUptimeMs:now()});active=false;
 }
};
