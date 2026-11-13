const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),crypto=require('node:crypto');
const out=path.resolve(process.argv[2]),route=process.argv[3];if(!['m1','m2'].includes(route))throw Error('ROUTE');
cp.execFileSync(process.execPath,[require.resolve('../t485/generate.cjs'),out,route],{stdio:'inherit'});
const main=path.join(out,'entry/src/main'),base=path.join(main,'cpp/gpu_resident');
const read=n=>fs.readFileSync(path.join(__dirname,n),'utf8').replaceAll('\r\n','\n');
function one(s,a,b){if(s.split(a).length!==2)throw Error('T492_ANCHOR '+a);return s.replace(a,b);}
function edit(file,fn){const p=path.join(main,file);fs.writeFileSync(p,fn(fs.readFileSync(p,'utf8').replaceAll('\r\n','\n')));}
const source=cp.execFileSync('git',['rev-parse','HEAD'],{encoding:'utf8'}).trim();
fs.writeFileSync(path.join(base,'t492_source.h'),'#pragma once\ninline constexpr char kT492Route[]="'+route+'";\ninline constexpr char kT492Source[]="'+source+'+T492-overlay-v1";\n');
for(const name of ['timeline_snapshot.h','execution_diagnostics.h'])fs.copyFileSync('entry/src/main/cpp/gpu_resident/'+name,path.join(base,name));
edit('cpp/gpu_resident/app_bridge.cpp',s=>{
 s='#include "timeline_snapshot.h"\n#include "execution_diagnostics.h"\n#include "t492_source.h"\n#ifdef _WIN32\n#include <process.h>\n#endif\n'+s;
 s=one(s,' struct WorkPermit {',' struct WorkPermit {');
 const progress=' '+(route==='m2'?'m2route':'m1')+'::ProgressClockV2 progress;';
 s=one(s,progress,read('bridge.inc')+'\n'+progress);
 // Closed or foreign callbacks are observations only. Preserve original ABI reply.
 s=one(s,'  CheckEvidenceLocked();\n\n  checkingPulse=',`  CheckEvidenceLocked();
  if(session!=progress.session||!progress.open){
   if(auto* group=diagnostics.Find(DiagnosticScopeLocked()))if(group->cause.set&&group->cause.latePulses!=UINT64_MAX)++group->cause.latePulses;
   DiagnosticLocked("TERMINAL_OR_STALE_PULSE",0,session,seq,callback);
   return progressV2?"STALE_PAGE_SESSION":"PAGE_PULSE_ACCEPTED";
  }
  checkingPulse=`);
 // Only an open current session may transition to GAP in this call.
 s=one(s,'  const auto oldReceive=progress.receiveNs;','  const bool wasOpen=progress.open;\n  const auto oldReceive=progress.receiveNs;');
 s=one(s,'}else if(progressV2&&!progress.open){','}else if(progressV2&&wasOpen&&!progress.open){');
 s=one(s,'  jobLive=false;recovery.Fault(why);','  DiagnosticFaultLocked(why);\n  jobLive=false;recovery.Fault(why);');
 s=one(s,'  RecoveryNote("OLD_PERMIT_TOMBSTONE"','  DiagnosticLocked("RESET_NATIVE_REQUEST",permit.nonce,epoch.load(),recovery.serial);\n  RecoveryNote("OLD_PERMIT_TOMBSTONE"');
 s=one(s,'const char* bootReason="UI_RESET",uint64_t width=0,uint64_t height=0){std::lock_guard<std::mutex> l(mutex);',`const char* bootReason="UI_RESET",uint64_t width=0,uint64_t height=0,uint64_t actionSerial=0){std::lock_guard<std::mutex> l(mutex);
 #ifdef GRS_APP_M1
  const auto diagnosticOldEpoch=epoch.load();diagnostics.ResetRecord(DiagnosticPointLocked("RESET_NATIVE_ENTER",permit.nonce,actionSerial,diagnosticOldEpoch,recovery.serial));
 #endif`);
 s=one(s,' RecoveryRequestLocked(!replace);',' RecoveryRequestLocked(!replace);\n diagnostics.ResetRecord(DiagnosticPointLocked("RESET_NATIVE_RECEIPT",permit.nonce,actionSerial,diagnosticOldEpoch,epoch.load()));');
 s=one(s,'  recovery.Note({kind,Stamp()', '  DiagnosticLocked(kind,permit.nonce,a,b,c,code);\n  recovery.Note({kind,Stamp()');
 s=one(s,'FreezePermitLocked(seen,held,isolation,run->NextNonce(),run->NextKind());RecoveryNote', 'FreezePermitLocked(seen,held,isolation,run->NextNonce(),run->NextKind());DiagnosticIntentLocked();RecoveryNote');
 s=one(s,'  CaptureAcceptedLocked(a);',`  DiagnosticLocked("ROOT_ACCEPT",(uint64_t(a.nonce.hi)<<32)|a.nonce.lo,(uint64_t(a.source.hi)<<32)|a.source.lo,(uint64_t(a.physics.hi)<<32)|a.physics.lo,a.completion[27]);
  if(a.completion[27]==1)DiagnosticLocked("SOURCE_ACCEPT",(uint64_t(a.nonce.hi)<<32)|a.nonce.lo,ui.request,ui.admitted,ui.rejected,ui.receipt);
  CaptureAcceptedLocked(a);`);
 s=one(s,'CaptureOfferLocked(w,result);return result;', 'CaptureOfferLocked(w,result);DiagnosticOfferLocked(w,result);return result;');
 s=one(s,' void MarkLocked(BridgeStage stage,uint64_t seen=0,uint64_t last=0,uint64_t delta=0,int code=0,uint32_t carry=0,uint32_t q=0){',` void MarkLocked(BridgeStage stage,uint64_t seen=0,uint64_t last=0,uint64_t delta=0,int code=0,uint32_t carry=0,uint32_t q=0){
 #ifdef GRS_APP_M1
  static constexpr const char* names[]={"INVALID","INVALIDATE","INIT_BEGIN","READY","READY_REJECTED","WAIT_ENTER","WAKE","FIRST_TICK","ADVANCE_BEGIN","ADVANCE_END","ANCHOR_SET","PUMP_ENTER","PUMP_EXIT","DRAW_ENTER","DRAW_EXIT","SWAP_CHECK_ENTER","SWAP_CHECK_EXIT","PAUSE_ENTER","PAUSE_EXIT","OFFER","CONSUME","STALE","ROOT_SUBMIT","ROOT_POLL"};
  if(uint32_t(stage)>0&&uint32_t(stage)<24)DiagnosticLocked(names[uint32_t(stage)],permit.nonce,last,delta,q,uint32_t(code));
 #endif`);
 s=one(s,'  timeline.Add(r);CaptureDetailLocked(r);',`  static constexpr const char* names[]={"BATCH_CALL","FLUSH","CONTINUATION","INFLIGHT","REAL_POLL","ACCEPTED","GAP","PRESENTATION","STAGING_FLUSH","COLD_FIXTURE","COLD_STAGING_FLUSH","COLD_BATCH","COLD_FENCE","COLD_CLEANUP","COLD_BATCH_FLUSH","ISOLATION_CALL","ISOLATION_REVOKE","PAGE_PULSE","PLAN_GRANT","QUOTA_BALANCE","JOB_INTENT","PROGRESS_TIMEOUT","SOURCE_BOUNDARY","BATCH_FLUSH_ENTER","BATCH_FLUSH_EXIT"};
  if(r.stage>=43&&r.stage<=67){auto point=DiagnosticPointLocked(names[r.stage-43],r.root,s.a,s.b,s.c,s.code);point.callStart=s.start;point.callEnd=s.end;diagnostics.Add(point);}
  else {++diagnostics.unknownStageCount;DiagnosticLocked("UNKNOWN_TAIL_STAGE",r.root,r.stage);}
  timeline.Add(r);CaptureDetailLocked(r);`);
 // Submit observer still emits into legacy timeline; independent summary survives its overflow.
 s=one(s,'  bootProbe.Root(e,{"SUBMIT_OBSERVER",',`  static constexpr const char* names[]={"SUBMIT_ENTER","SUBMIT_EXIT","SHADER_ENTER","SHADER_EXIT","PROGRAM_ENTER","PROGRAM_EXIT","RESOURCE_ENTER","RESOURCE_EXIT","PREPARE_ENTER","PREPARE_EXIT","DISPATCH_ENTER","DISPATCH_EXIT","FENCE_ENTER","FENCE_EXIT","POLL_ENTER","POLL_EXIT","SUBMIT_ROOT_BEGIN","SUBMIT_CANCEL","SUBMIT_WARMUP"};
  if(row.stage>=24&&row.stage<=42)DiagnosticLocked(names[row.stage-24],row.root,row.last,row.delta,row.carry,row.code);
  bootProbe.Root(e,{"SUBMIT_OBSERVER",`);
 s=one(s,'  auto retire=[&]()->bool{',`  #ifdef GRS_APP_M1
  diagnostic492::ownerObserver={this,[](void* value,const char* stage,uint64_t a,uint64_t b,uint32_t code,uint32_t gl,uint32_t egl){auto& app=*static_cast<App*>(value);std::lock_guard<std::mutex> lock(app.mutex);app.DiagnosticLocked(stage,app.permit.nonce,a,b,0,code,gl,egl);}};
  struct ClearDiagnosticObserver{~ClearDiagnosticObserver(){diagnostic492::ownerObserver={};}} clearDiagnosticObserver;
  #endif
  auto retire=[&]()->bool{`);
 s=one(s,'     const bool native=eglSwapBuffers(v.d,v.s)==EGL_TRUE;',`     {std::lock_guard<std::mutex> lock(v.app->mutex);v.app->DiagnosticLocked("EGL_SWAP_ENTER",(uint64_t(v.root.nonce.hi)<<32)|v.root.nonce.lo);}
     const bool native=eglSwapBuffers(v.d,v.s)==EGL_TRUE;
     const uint32_t diagnosticSwapEgl=native?0:eglGetError();
     {std::lock_guard<std::mutex> lock(v.app->mutex);v.app->DiagnosticLocked("EGL_SWAP_RETURN",(uint64_t(v.root.nonce.hi)<<32)|v.root.nonce.lo,0,0,0,native?0:1,0,diagnosticSwapEgl);}`);
 // Preserve consumed EGL failure in Scene's original raw-error receipt.
 s=one(s,'return native&&allowed;', 'if(!native)diagnostic492::lastSwapError=diagnosticSwapEgl;return native&&allowed;');
 s=one(s,'     if(!PermitLocked(presenting,Stamp())){','     DiagnosticLocked("POST_DRAW_PERMIT_ENTER",permit.nonce);\n     if(!PermitLocked(presenting,Stamp())){');
 s=one(s,'      ui.Memory(gpu->AllocatedBytes(),receipt.frameTargetBytes);','      ui.Memory(gpu->AllocatedBytes(),receipt.frameTargetBytes);');
 s=one(s,'    Mark(BridgeStage::DrawEnd,seen,last,0,receipt.current?0:1);',`    #ifdef GRS_APP_M1
    {std::lock_guard<std::mutex> lock(mutex);DiagnosticLocked("POST_DRAW_PERMIT_EXIT",permit.nonce,receipt.current,receipt.swapSucceeded,0,receipt.current?0:1,receipt.rawGl,receipt.rawEgl);}
    #endif
    Mark(BridgeStage::DrawEnd,seen,last,0,receipt.current?0:1);`);
 s=one(s,'napi_value newRun(napi_env','#ifdef GRS_APP_M1\n'+read('napi.inc')+'\n#endif\nnapi_value newRun(napi_env');
 const apis=['timelineSnapshotBegin','timelineSnapshotPage','timelineSnapshotEnd','executionDiagnostics','recordDiagnosticAction','newRunAction'];
 s=one(s,'{"timeline",nullptr,timeline','\n#ifdef GRS_APP_M1\n'+apis.map(n=>' {"'+n+'",nullptr,'+n+',nullptr,nullptr,nullptr,napi_default,nullptr},').join('\n')+'\n#endif\n{"timeline",nullptr,timeline');
 return s;
});
// Probe exact Scene call boundaries without changing GLSL, draw order or permit conditions.
for(const target of ['m1',...(route==='m2'?['m2route']:[])])edit('cpp/gpu_resident/'+target+'/scene.cpp',s=>{
 s='#include "../execution_diagnostics.h"\n'+s;
 s=one(s,' glGenVertexArrays(1,&vertices);', ' diagnostic492::ownerObserver.Emit("SCENE_VAO_ENTER");\n glGenVertexArrays(1,&vertices);');
 s=one(s,' const auto vaoError=glGetError();',' const auto vaoError=glGetError();\n diagnostic492::ownerObserver.Emit("SCENE_VAO_EXIT",vertices,instances,vaoError?1:0,vaoError);');
 s=one(s,'  if(!programcache::BuildGraphics(descriptors[i],programs[i],error,generation,cacheEnabled))', '  diagnostic492::ownerObserver.Emit("SCENE_BUILD_ENTER",i);\n  const bool built=programcache::BuildGraphics(descriptors[i],programs[i],error,generation,cacheEnabled);\n  diagnostic492::ownerObserver.Emit("SCENE_BUILD_EXIT",i,programs[i],built?0:1,error);\n  if(!built)');
 s=one(s,'  prepareFailure=reason;', '  diagnostic492::ownerObserver.Emit("SCENE_PREPARE_FAILED",0,0,1);\n  prepareFailure=reason;');
 s=one(s,'success=targets.Ensure(f.width,f.height,value(f.context))==p013::Status::OK;', 'diagnostic492::ownerObserver.Emit("SCENE_FBO_ENTER",f.width,f.height);success=targets.Ensure(f.width,f.height,value(f.context))==p013::Status::OK;diagnostic492::ownerObserver.Emit("SCENE_FBO_EXIT",f.width,f.height,success?0:1);');
 let index=0;
 s=s.replace(/glDrawArrays(?:Instanced)?\([^;]+;/g,call=>{const i=index++;return 'diagnostic492::ownerObserver.Emit("SCENE_LAYER_ENTER",'+i+');'+call+'diagnostic492::ownerObserver.Emit("SCENE_LAYER_EXIT",'+i+');';});
 if(index!==8)throw Error('SCENE_DRAW_COUNT '+index);
 s=one(s,'if(!success)r.rawEgl=eglGetError();','if(!success){r.rawEgl=diagnostic492::lastSwapError?diagnostic492::lastSwapError:eglGetError();diagnostic492::lastSwapError=0;}');
 s=one(s,'if(e!=GL_NO_ERROR&&r.rawGl==0)r.rawGl=e;', 'if(e!=GL_NO_ERROR&&r.rawGl==0){r.rawGl=e;diagnostic492::ownerObserver.Emit("SCENE_GL_ERROR",0,0,1,e);}');
 return s;
});
edit('cpp/p013renderer/fullframe_targets.cpp',s=>{
 s='#include "../gpu_resident/execution_diagnostics.h"\n'+s;
 return one(s,'if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE||encoding!=GL_SRGB||glGetError()!=GL_NO_ERROR){Reset(true);return Status::COLORSPACE_UNSUPPORTED;}',`GLenum raw=0;bool queried=false;const auto status=glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if(status!=GL_FRAMEBUFFER_COMPLETE||encoding!=GL_SRGB||([&]{queried=true;raw=glGetError();return raw;})()!=GL_NO_ERROR){grs::diagnostic492::ownerObserver.Emit("SCENE_FBO_VALIDATE_FAILED",status,uint64_t(encoding),queried?1:2,raw);Reset(true);return Status::COLORSPACE_UNSUPPORTED;}`);
});
edit('cpp/gpu_resident/program_cache/graphics_program_factory.h',s=>{
 s='#include "../execution_diagnostics.h"\n'+s;
 s=s.replace('glProgramBinary(program,artifact->format,artifact->bytes.data(),GLsizei(artifact->bytes.size()));','diagnostic492::ownerObserver.Emit("GRAPHICS_LOAD_ENTER");glProgramBinary(program,artifact->format,artifact->bytes.data(),GLsizei(artifact->bytes.size()));diagnostic492::ownerObserver.Emit("GRAPHICS_LOAD_EXIT");');
 s=s.replace('glCompileShader(shaders[i]);','diagnostic492::ownerObserver.Emit("GRAPHICS_COMPILE_ENTER",i);glCompileShader(shaders[i]);diagnostic492::ownerObserver.Emit("GRAPHICS_COMPILE_EXIT",i);');
 s=s.replace('glLinkProgram(program);','diagnostic492::ownerObserver.Emit("GRAPHICS_LINK_ENTER");glLinkProgram(program);diagnostic492::ownerObserver.Emit("GRAPHICS_LINK_EXIT");');
 return s;
});
for(const name of ['GpuBridgeTimelineCold','GpuM1DemoController','GpuTimelineSnapshotCold'])fs.copyFileSync('entry/src/main/ets/physics/'+name+'.ets',path.join(main,'ets/physics',name+'.ets'));
if(route==='m2')edit('ets/physics/GpuM1DemoController.ets',s=>one(one(s,'w[1]=2;','w[1]=3;'),'configureM1(k,kw,3,2)','configureM1(k,kw,7,3)'));
edit('cpp/types/libgrsm1/index.d.ts',s=>s+'\nexport const timelineSnapshotBegin:()=>string;\nexport const timelineSnapshotPage:(id:string,cursor:number)=>string;\nexport const timelineSnapshotEnd:(id:string)=>string;\nexport const executionDiagnostics:()=>string;\nexport const recordDiagnosticAction:(raw:string)=>string;\nexport const newRunAction:(serial:string)=>string;\n');
require('./pages.cjs')(main,edit,one);
const manifest={revision:'T492_DIAGNOSTIC_OVERLAY_V1',route,sourceCommit:source,device:'NOT_RUN',files:{}};
for(const name of ['app_bridge.cpp','timeline_snapshot.h','execution_diagnostics.h','t492_source.h','program_cache/graphics_program_factory.h','program_cache/program_factory.h','program_cache/artifact_store.h','../p013renderer/fullframe_targets.cpp','m1/scene.cpp',...(route==='m2'?['m2route/scene.cpp']:[])])manifest.files[name]=crypto.createHash('sha256').update(fs.readFileSync(path.join(base,name))).digest('hex');
manifest.integrationFiles={};
for(const name of ['physics/GpuBridgeTimelineCold','physics/GpuTimelineSnapshotCold','physics/GpuM1DemoController','physics/GpuRecoveryPulse','pages/GpuResidentM1','pages/GpuSandWaterExperience'])manifest.integrationFiles[name+'.ets']=crypto.createHash('sha256').update(fs.readFileSync(path.join(main,'ets',name+'.ets'))).digest('hex');
fs.writeFileSync(path.join(out,'t492-manifest.json'),JSON.stringify(manifest,null,2)+'\n');
