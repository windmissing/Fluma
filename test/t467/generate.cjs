const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]),target=process.argv[3];
if(!['regular','experience','t410','t437'].includes(target))throw Error('target');
cp.execFileSync(process.execPath,[require.resolve('../t456/generate.cjs'),out,target==='regular'?'regular':'experiment'],{stdio:'inherit'});
const main=path.join(out,'entry/src/main'),base=path.join(main,'cpp/gpu_resident'),cpp=path.dirname(base);
// Apply existing frozen-source integration edits to this same isolated project.
if(target==='t437'){
 for(const n of ['full_transport.h','full_transport.cpp','quantum_candidate.h','quantum_candidate.cpp'])fs.writeFileSync(path.join(base,'m1',n),cp.execFileSync('git',['show','b54dc48:entry/src/main/cpp/gpu_resident/m1/'+n]));
 const source=fs.readFileSync(require.resolve('../t434/generate.cjs'),'utf8');
 new Function('fs','path','out','cpp','base',source.slice(source.indexOf('const bridge=')))(fs,path,out,cpp,base);
}
if(target==='t410'||target==='t437'){
 const source=fs.readFileSync(require.resolve('../t450/generate.cjs'),'utf8');
 new Function('fs','path','cp','out','target','experiment','__dirname',source.slice(source.indexOf('const main=')))(fs,path,cp,out,target,true,path.dirname(require.resolve('../t450/generate.cjs')));
}
const read=n=>fs.readFileSync(path.join(__dirname,n),'utf8').replaceAll('\r\n','\n');
function edit(p,f){const file=path.join(main,p);fs.writeFileSync(file,f(fs.readFileSync(file,'utf8').replaceAll('\r\n','\n')));}
function one(s,a,b){if(s.split(a).length!==2)throw Error('T467_SOURCE_DRIFT '+a);return s.replace(a,b);}
function span(s,a,b,v){const i=s.indexOf(a),j=s.indexOf(b,i);if(i<0||j<0)throw Error('T467_SPAN '+a);return s.slice(0,i)+v+s.slice(j);}
fs.copyFileSync('entry/src/main/cpp/gpu_resident/recovery_v3.h',path.join(base,'recovery_v3.h'));
edit('cpp/gpu_resident/submit_observer.h',s=>one(s,'ProgressTimeout,SourceBoundary};','ProgressTimeout,SourceBoundary,BatchFlushEnter,BatchFlushExit};'));
edit('cpp/gpu_resident/bridge_timeline.h',s=>one(s,
 '#ifdef GRS_PROGRESS_V2_EXPERIMENT\n "T404_STAGE65_V1":\n#else\n "T398_STAGE59_V1":\n#endif',
 ' "T477_STAGE67_V1":'));
fs.copyFileSync('entry/src/main/ets/physics/GpuBridgeTimelineCold.ets',path.join(main,'ets/physics/GpuBridgeTimelineCold.ets'));
edit('cpp/gpu_resident/app_bridge.cpp',s=>{
 s=one(s,'const bool runnable=run&&!failed', 'const bool runnable=AllowedLocked()&&run&&!failed');
 s=one(s,'#include "boot_probe.h"','#include "boot_probe.h"\n#include "recovery_v3.h"');
 s=one(s,' m1::ProgressClockV2 progress;',read('recovery_bridge.inc')+'\n m1::ProgressClockV2 progress;');
 s=span(s,' void TryReadyV2Locked(){',' const char* PagePulseLocked',` void TryReadyV2Locked(){
  const auto stamp=Stamp();
  if(!v2ResourcesReady||!recovery.CanReady(epoch,window,transportIdentity,progress.session)||!active||!window||!progress.Fresh(stamp)||!ui.Fresh(stamp))return;
  if(!AllowedLocked()){
   progress.ready=progressV2;progress.baseline=false;
   if(CommitReadyLocked(epoch,window,transportIdentity,input)){recovery.Ready();RecoveryNote("READY_COMMIT",2);CaptureReadyLocked();bootIdentityLocked(epoch);bootNote(epoch,"RECOVERY_READY_COMMIT");}
  }
 }
 uint64_t BeginPageSessionLocked(){
  return RecoverySessionLocked(recovery.serial,recovery.epoch,recovery.candidate.run,1);
 }
`);
 s=one(s,'  if(!progressV2&&!progress.open)progress.open=true;','');
 s=one(s,'  const auto result=progress.Pulse(session,seq,callback,observedPulse.observedReceiveNs);',`  const auto oldReceive=progress.receiveNs;
  const auto result=progress.Pulse(session,seq,callback,observedPulse.observedReceiveNs);
  RecoveryNote("PAGE_PULSE",1,callback,oldReceive,observedPulse.observedReceiveNs,std::strcmp(result,"PAGE_PULSE_ACCEPTED")!=0);`);
 s=s.replace('if(evidence&&evidence->armed){if(progressV2)progress.Revoke("WINDOW_OR_RUN_CHANGED");CaptureRevokeLocked("WINDOW_OR_RUN_CHANGED");}', 'if(evidence&&evidence->armed){if(progressV2)progress.Revoke(replace?"WINDOW_OR_RUN_CHANGED":"RUN_RESET");CaptureRevokeLocked(replace?"WINDOW_OR_RUN_CHANGED":"RUN_RESET");}');
 s=one(s,'  phase="CLOCK_GAP";firstFailure=', '  recovery.Fault(progressV2?"V2_PULSE_CLOCK_GAP":"V1_ANCHOR_CLOCK_GAP");RecoveryNote("CONTROL_REVOKED",3,origin,stamp,elapsed);\n  phase="CLOCK_GAP";firstFailure=');
 s=one(s,'  jobLive=false;\n  if(progressV2)', '  jobLive=false;recovery.Fault(why);\n  if(progressV2)');
 s=one(s,'const auto bootBefore=epoch.load();if(replace)++bootSurface;',`const auto bootBefore=epoch.load();if(epoch==UINT64_MAX||permissionRevision==UINT64_MAX){recovery.Fault("EPOCH_OR_PERMISSION_OVERFLOW");requiresReset=true;BridgeState::RevokeLocked("IDENTITY_OVERFLOW");return;}if(replace)++bootSurface;`);
 s=span(s,' if(progressV2){\n  progress.Revoke("RUN_RESET")','\n#endif\n#ifdef GRS_PROGRESS_V2_EXPERIMENT',` RecoveryRequestLocked(!replace);
`);
 // Every cold selection/reset requires a new identity-bound resource handoff.
 s=one(s,'FreezeEvidenceLocked("RUN_RESET");progress.ResetRun();v2ResourcesReady=false;', 'FreezeEvidenceLocked("RUN_RESET");progress.ResetRun();progress.open=false;progress.hasPulse=false;v2ResourcesReady=false;');
 s=one(s,'    if(active&&window\n#ifdef GRS_APP_M1','    if(active&&window\n#ifdef GRS_APP_M1');
 s=one(s,'   run.reset();if(gpu)gpu->ContextLost();gpu.reset();scene.Reset(false);',`#ifdef GRS_APP_M1
   {std::lock_guard<std::mutex> lock(mutex);recovery.Retiring(epoch);RecoveryNote("CONTEXT_TOMBSTONES",2,run&&run->HasFlight(),gpu&&gpu->Owner().HasLeases(),permit.nonce);if(run&&run->HasFlight())RecoveryNote("RETIRE_ROOT",2,run->NextNonce(),transportEpoch);if(gpu)for(size_t slot=0;slot<2;++slot){U64 g;if(gpu->Owner().RetirementLease(slot,g))RecoveryNote("RETIRE_LEASE",2,slot,(uint64_t(g.hi)<<32)|g.lo,transportEpoch);}}
#endif
   run.reset();if(gpu)gpu->ContextLost();gpu.reset();scene.Reset(false);`);
 const old='if(display!=EGL_NO_DISPLAY){eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(surface!=EGL_NO_SURFACE)eglDestroySurface(display,surface);if(context!=EGL_NO_CONTEXT)eglDestroyContext(display,context);eglTerminate(display);}display=EGL_NO_DISPLAY;surface=EGL_NO_SURFACE;context=EGL_NO_CONTEXT;';
 s=one(s,old,`if(display!=EGL_NO_DISPLAY){
    if(!eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT)){retirePending=true;return false;}
    if(surface!=EGL_NO_SURFACE){if(!eglDestroySurface(display,surface)){retirePending=true;return false;}surface=EGL_NO_SURFACE;}
    if(context!=EGL_NO_CONTEXT){if(!eglDestroyContext(display,context)){retirePending=true;return false;}context=EGL_NO_CONTEXT;}
    if(!eglTerminate(display)){retirePending=true;return false;}
   }display=EGL_NO_DISPLAY;surface=EGL_NO_SURFACE;context=EGL_NO_CONTEXT;retirePending=false;`);
 s=one(s,'  auto retire=[&]{','  bool retirePending=false;\n  auto retire=[&]()->bool{');
 s=one(s,'held=nullptr;last=frame=render=lastAccepted=0;};','held=nullptr;last=frame=render=lastAccepted=0;\n#ifdef GRS_APP_M1\n{std::lock_guard<std::mutex> lock(mutex);recovery.Retired(epoch);RecoveryNote("CONTEXT_RETIRED",2);}\n#endif\nreturn true;};');
 s=one(s,'   if(currentEpoch!=seen','   if(retirePending||currentEpoch!=seen');
 s=one(s,'   ){retire();seen=currentEpoch;', '   ){if(!retire())continue;seen=currentEpoch;');
 s=span(s,'     if(progressV2){\n      v2ResourcesReady=true;', '\n#endif\n     committed=CommitReadyLocked',`     {
      v2ResourcesReady=recovery.Resources(seen,held,id);input=gpu->InputGeneration();identity=id;
      committed=CurrentLocked(seen,held)&&v2ResourcesReady;phase=permission="WAIT_NEW_SESSION";
      status="{\\"state\\":\\"WAIT_NEW_SESSION\\"}";RecoveryNote("RESOURCES_READY",2,runId);TryReadyV2Locked();
     }
`);
 s=one(s,'\n     committed=CommitReadyLocked(seen,held,id,gpu->InputGeneration());','\n#ifndef GRS_APP_M1\n     committed=CommitReadyLocked(seen,held,id,gpu->InputGeneration());\n#endif');
 s=one(s,'if(session!=a.progress.session)return text(e,"STALE_PAGE_SESSION");','if(session!=a.progress.session||!a.progress.open||a.recovery.session!=session)return text(e,"STALE_PAGE_SESSION");');
 s=one(s,'controlAnchor=last;}','RecoveryNote("ANCHOR_SET",2,controlAnchor,last);controlAnchor=last;}');
 s=one(s,'     app.AddTailLocked(sample,begin);',`     if(sample.stage==TailStage::BatchFlushEnter||sample.stage==TailStage::BatchFlushExit){
      app.RecoveryNote(sample.stage==TailStage::BatchFlushEnter?"FLUSH_ENTER":"FLUSH_EXIT",2,sample.a,sample.b,sample.start);
      if(sample.stage==TailStage::BatchFlushEnter){app.recovery.calling=true;app.recovery.inCall={"FLUSH_ENTER",sample.start,BridgeTid(),app.recovery.serial,app.recovery.epoch,app.recovery.session,app.permissionRevision,sample.root,sample.a,sample.b,0,2,0};app.recovery.inCall.root=sample.root;app.recovery.inCall.stamp=sample.start;app.recovery.inCall.a=sample.a;app.recovery.inCall.b=sample.b;}
      else app.recovery.calling=false;
     }
     app.AddTailLocked(sample,begin);`);
 s=s.replace('if(bootProbe.WantsTail(seen))gpu->tailObserver=gpu->coldObserver;', 'gpu->tailObserver=gpu->coldObserver;').replace('gpu->tailObserver=(bootProbe.WantsTail(seen)||activeTailProbe||(evidence&&evidence->armed))?gpu->coldObserver:TailObserver{};', 'gpu->tailObserver=gpu->coldObserver;');
 s=one(s,'     FreezePermitLocked(seen,held,isolation,run->NextNonce(),run->NextKind());','     FreezePermitLocked(seen,held,isolation,run->NextNonce(),run->NextKind());RecoveryNote(\"ROOT_INTENT\",2,permitPlan,permitStarted,permit.deadline);');
 s=one(s,'   if(run&&!failed&&!gap){', '   if(run&&!failed&&!gap){\n    {std::lock_guard<std::mutex> lock(mutex);if(recovery.phase==recovery467::Phase::WAIT_NEW_SESSION){TryReadyV2Locked();if(!AllowedLocked())continue;}}');
 s=one(s,'napi_value newRun(napi_env', '#ifdef GRS_APP_M1\n'+read('recovery_napi.inc')+'\n#endif\nnapi_value newRun(napi_env');
 s=one(s,'{"newRun",nullptr,newRun','\n#ifdef GRS_APP_M1\n{"recoveryState",nullptr,recoveryState,nullptr,nullptr,nullptr,napi_default,nullptr},{"beginRecoverySession",nullptr,beginRecoverySession,nullptr,nullptr,nullptr,napi_default,nullptr},\n#endif\n{"newRun",nullptr,newRun');
 s=one(s,' return text(e,out.str());\n}\n#endif\nnapi_value status', ' auto raw=out.str();if(!raw.empty())raw.pop_back();out.str("");out.clear();out<<raw<<",\\"recovery\\":";{auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);a.RecoveryJson(out,true);}out<<"}";return text(e,out.str());\n}\n#endif\nnapi_value status');
 s=s.replaceAll('a.InvalidateLocked();','a.InvalidateLocked();a.RecoveryRequestLocked(true);');
 return s;
});
edit('cpp/gpu_resident/m1/source_owner.h',s=>one(s,' bool HasLeases()const', ' bool RetirementLease(size_t slot,U64& generation)const{if(slot>=2)return false;const auto& p=pubs[slot];generation=p.generation;return p.leased||p.releasePending;}\n bool HasLeases()const'));
edit('cpp/gpu_resident/m1/full_transport.cpp',s=>{
 const begin=s.indexOf('void FullM1Transport::FlushBatch('),end=s.indexOf('TransportCode FullM1Transport::ObserveBatch',begin);
 const a=s.slice(begin,end).replace(' glFlush();',' tailObserver.Emit({TailStage::BatchFlushEnter,(uint64_t(pending.nonce.hi)<<32)|pending.nonce.lo,before,0,from,passCursor,0,uint32_t(final),batchesIssued});\n glFlush();').replace(' const auto after=tailObserver.Stamp();',' const auto after=tailObserver.Stamp();\n tailObserver.Emit({TailStage::BatchFlushExit,(uint64_t(pending.nonce.hi)<<32)|pending.nonce.lo,before,after,from,passCursor,0,uint32_t(final),batchesIssued});');
 return s.slice(0,begin)+a+s.slice(end);
});
fs.copyFileSync(path.join(__dirname,'GpuRecoveryPulse.ets'),path.join(main,'ets/physics/GpuRecoveryPulse.ets'));
edit('cpp/types/libgrsm1/index.d.ts',s=>s+'\nexport const recoveryState:()=>string;\nexport const beginRecoverySession:(serial:string,epoch:string,lo:string,hi:string,page:string)=>string;\n');
require('./pages.cjs')(main,edit,one,target);
fs.writeFileSync(path.join(out,'t467-manifest.json'),JSON.stringify({schema:'T467_RECOVERY_BUILD_V1',target,clock:target==='regular'?'V1':'EXPLICIT_V2',prefix:target==='t437',base:'T456/T450/T434',device:'NOT_RUN'},null,2));
