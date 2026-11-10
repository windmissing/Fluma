const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process');
const out=path.resolve(process.argv[2]),profile=process.argv[3];
if(!['regular','experiment'].includes(profile))throw Error('profile');
cp.execFileSync(process.execPath,[require.resolve('../t438/generate.cjs'),out,profile],{stdio:'inherit'});
const main=path.join(out,'entry/src/main');
const local=n=>fs.readFileSync(path.join(__dirname,n),'utf8').replaceAll('\r\n','\n');
function edit(rel,fn){const file=path.join(main,rel);fs.writeFileSync(file,fn(fs.readFileSync(file,'utf8').replaceAll('\r\n','\n')));}
function one(s,a,b){if(s.split(a).length!==2)throw Error('T456_ANCHOR '+a);return s.replace(a,b);}
fs.copyFileSync(path.join(__dirname,'boot_probe.h'),path.join(main,'cpp/gpu_resident/boot_probe.h'));
edit('cpp/CMakeLists.txt',s=>s+'\ntarget_compile_definitions(grsm1 PRIVATE GRS_T456_BOOT)\n');
const guard=body=>'\n#ifdef GRS_T456_BOOT\n'+body+'\n#endif\n';
edit('cpp/gpu_resident/app_bridge.cpp',s=>{
 s=one(s,'#include "run.h"','#include "run.h"'+guard('#include "boot_probe.h"\n#ifndef GRS_BRIDGE_HOST_TEST\n#include <unistd.h>\n#endif'));
 s=one(s,' std::condition_variable wake;std::thread thread;', ' std::condition_variable wake;std::thread thread;'+guard(' boot456::Probe bootProbe;uint64_t bootSurface=0;\n'+local('bridge.inc')));
 s=one(s,' void invalidate(void* w,bool replace){std::lock_guard<std::mutex> l(mutex);',` void invalidate(void* w,bool replace,const char* bootReason="UI_RESET",uint64_t width=0,uint64_t height=0){std::lock_guard<std::mutex> l(mutex);`+guard('const auto bootBefore=epoch.load();if(replace)++bootSurface;'));
 s=one(s,'if(replace){if(w&&OH_NativeWindow_NativeObjectReference(w)!=0){',`if(replace){const int refResult=w?OH_NativeWindow_NativeObjectReference(w):0;`+guard('bootTransitionLocked("CALLBACK_WINDOW_REFERENCE",bootBefore,width,height,refResult);')+'if(refResult!=0){');
 s=one(s,'permission=phase="FAULT";status="{\\"state\\":\\"FAULT\\",\\"stage\\":\\"WINDOW_REFERENCE\\"}";',`permission=phase="FAULT";status="{\\"state\\":\\"FAULT\\",\\"stage\\":\\"WINDOW_REFERENCE\\"}";`+guard('bootTransitionLocked(bootReason,bootBefore,width,height,refResult);\nbootProbe.Begin(epoch,bootSurface,false,0,K,Kw,progressV2,Stamp());bootNote(epoch,"CALLBACK_WINDOW_REFERENCE",true,true,refResult);'));
 s=one(s,' MarkLocked(BridgeStage::Invalidate);wake.notify_all();}',guard('bootTransitionLocked(bootReason,bootBefore,width,height);')+' MarkLocked(BridgeStage::Invalidate);wake.notify_all();}');
 s=one(s,'if(active==value)return false;active=value;','if(active==value)return false;'+guard('bootTransitionLocked(value?"ACTIVE_RESUME":"ACTIVE_BACKGROUND",epoch,active,value);\nif(!value)bootNote(epoch,"ACTIVE_BACKGROUND",true,true);')+'active=value;');
 s=one(s,'if(!progress.BeginSession())return 0;','if(!progress.BeginSession())return 0;'+guard('bootTransitionLocked("PAGE_SESSION_BEGIN",epoch,progress.session);bootIdentityLocked(epoch);'));
 s=one(s,'  const auto original=progress.fault;','  const auto original=progress.fault;'+guard('bootNote(epoch,progressV2?"V2_PULSE_CLOCK_GAP":"V1_ANCHOR_CLOCK_GAP",true,true,0,0,0,permit.nonce,origin,stamp,elapsed);\nbootNote(epoch,"CLOCK_RAW_DOMAINS",false,false,0,0,0,permit.nonce,observedPulse.observedUiNs,observedPulse.observedReceiveNs,permit.deadline);'));
 s=one(s,'  if(!valid&&app.permit.epoch==app.epoch.load()){',guard('if(!valid&&app.permit.epoch){const bool late=app.progressV2&&stamp>app.permit.deadline;\napp.bootNote(app.permit.epoch,late?"V2_GPU_DEADLINE":"ROOT_REVOKED",true,true,int(cut),0,0,m1::NonceOf(root),app.permitStarted,app.permit.deadline,stamp);}')+'  if(!valid&&app.permit.epoch==app.epoch.load()){');
 s=one(s,'  commit(data);\n  app.AcceptLocked(out);','  commit(data);\n  app.AcceptLocked(out);'+guard('app.bootProbe.Root(app.permit.epoch,{"ROOT_ACCEPT",app.Stamp(),BridgeTid(),m1::NonceOf(root),root.kind,out.bank,out.publication});'));
 const a='    if(active&&window\n#ifdef GRS_APP_M1\n       &&!requiresReset\n#endif\n       &&OH_NativeWindow_NativeObjectReference(window)==0)held=window;';
 s=one(s,a,`#ifdef GRS_T456_BOOT
    bootProbe.Begin(seen,bootSurface,window!=nullptr,BridgeTid(),capacity,waterCapacity,progressV2,Stamp());
    bootIdentityLocked(seen);
    if(!active)bootNote(seen,"WAIT_ACTIVE");
    else if(!window)bootNote(seen,"WAIT_WINDOW");
    else if(requiresReset)bootNote(seen,"RESET_REQUIRED",true,true);
    else {
     bootNote(seen,"OWNER_WINDOW_REFERENCE_ENTER");
     const int ref=OH_NativeWindow_NativeObjectReference(window);
     bootNote(seen,"OWNER_WINDOW_REFERENCE_EXIT",ref!=0,ref!=0,ref);
     if(ref==0)held=window;
    }
#else
${a}
#endif
`);
 s=one(s,'#if !defined(GRS_BRIDGE_HOST_TEST) || defined(GRS_BRIDGE_HOST_GL)\n    display=eglGetDisplay',`    uint32_t bootRawEgl=0;
#if defined(GRS_T456_BOOT) && (!defined(GRS_BRIDGE_HOST_TEST) || defined(GRS_BRIDGE_HOST_GL))
    bool good=bootCreateEgl(seen,held,display,context,surface,bootRawEgl);
#elif !defined(GRS_BRIDGE_HOST_TEST) || defined(GRS_BRIDGE_HOST_GL)
    display=eglGetDisplay`);
 s=one(s,'std::to_string(eglGetError())+"}");failed=true;continue;}','std::to_string(bootRawEgl?bootRawEgl:eglGetError())+"}");failed=true;continue;}');
 s=one(s,'{EGLint viewportWidth=0,viewportHeight=0;eglQuerySurface(display,surface,EGL_WIDTH,&viewportWidth);eglQuerySurface(display,surface,EGL_HEIGHT,&viewportHeight);',`{EGLint viewportWidth=0,viewportHeight=0;
#ifdef GRS_T456_BOOT
     bootQuery(seen,display,surface,EGL_WIDTH,&viewportWidth);bootQuery(seen,display,surface,EGL_HEIGHT,&viewportHeight);
#else
     eglQuerySurface(display,surface,EGL_WIDTH,&viewportWidth);eglQuerySurface(display,surface,EGL_HEIGHT,&viewportHeight);
#endif`);
 s=one(s,'    transportEpoch=seen;transportIdentity=id;gpu=std::make_unique<AppTransport>();','    transportEpoch=seen;transportIdentity=id;'+guard('{std::lock_guard<std::mutex> lock(mutex);bootIdentityLocked(seen,runId);}\nbootNote(seen,"TRANSPORT_CREATE_ENTER");')+'gpu=std::make_unique<AppTransport>();');
 s=one(s,'if(!app.timelinePaused)app.timeline.Add(row);','if(!app.timelinePaused)app.timeline.Add(row);'+guard('app.bootRootEvent(app.transportEpoch,row);'));
 s=one(s,'     app.AddTailLocked(sample,begin);','     app.AddTailLocked(sample,begin);'+guard('app.bootProbe.Root(app.transportEpoch,{"TAIL_OBSERVER",sample.start,BridgeTid(),sample.root,uint32_t(sample.stage),sample.end,sample.ordinal,sample.code});'));
 s=one(s,'    if(code!=TransportCode::OK){publish(',guard('bootNote(seen,"TRANSPORT_CREATE_EXIT",code!=TransportCode::OK,code!=TransportCode::OK,int(code),0,gpu->Stats().firstGlError,0,gpu->Stats().allocatedBytes);')+'    if(code!=TransportCode::OK){publish(');
 s=one(s,'     MarkLocked(committed?BridgeStage::Ready:BridgeStage::ReadyRejected,seen);',guard('bootIdentityLocked(seen,runId);\nbootNote(seen,committed?(AllowedLocked()?"READY_COMMIT":"WAIT_PAGE_PULSE_OR_SENSOR"):"READY_REJECTED",!committed,!committed,committed,0,0,0,epoch,active,window==held);')+'     MarkLocked(committed?BridgeStage::Ready:BridgeStage::ReadyRejected,seen);');
 s=one(s,'   CommitReadyLocked(epoch,window,transportIdentity,input);CaptureReadyLocked();','   CommitReadyLocked(epoch,window,transportIdentity,input);CaptureReadyLocked();'+guard('bootIdentityLocked(epoch);bootNote(epoch,"V2_READY_COMMIT");'));
 s=one(s,'     const bool native=eglSwapBuffers(v.d,v.s)==EGL_TRUE;',`     const bool native=eglSwapBuffers(v.d,v.s)==EGL_TRUE;`+guard('v.app->bootProbe.Root(v.epoch,{"SWAP_RETURN",v.app->Stamp(),BridgeTid(),m1::NonceOf(v.root),0,0,0,native});'));
 s=one(s,'    Mark(BridgeStage::DrawEnd,seen,last,0,receipt.current?0:1);',guard('bootProbe.Draw(seen,{receipt.stage,Stamp(),BridgeTid(),m1::NonceOf(presenting),receipt.frameTargetBytes,0,0,receipt.current,receipt.rawEgl,receipt.rawGl},!receipt.current);')+'    Mark(BridgeStage::DrawEnd,seen,last,0,receipt.current?0:1);');
 return s;
});
edit('cpp/gpu_resident/app_bridge.cpp',s=>{
 s=one(s,'     FreezePermitLocked(seen,held,isolation,run->NextNonce(),run->NextKind());','     FreezePermitLocked(seen,held,isolation,run->NextNonce(),run->NextKind());'+guard('bootProbe.Intent(seen,permitKind,{"ROOT_INTENT",Stamp(),BridgeTid(),run->NextNonce(),permitKind,permitStarted,permit.deadline});'));
 s=one(s,'    if(activeTailProbe)gpu->tailObserver=gpu->coldObserver;',`#ifdef GRS_T456_BOOT
    if(bootProbe.WantsTail(seen))gpu->tailObserver=gpu->coldObserver;
#else
    if(activeTailProbe)gpu->tailObserver=gpu->coldObserver;
#endif`);
 s=one(s,'    gpu->tailObserver=(activeTailProbe||(evidence&&evidence->armed))?gpu->coldObserver:TailObserver{};',`#ifdef GRS_T456_BOOT
    gpu->tailObserver=(bootProbe.WantsTail(seen)||activeTailProbe||(evidence&&evidence->armed))?gpu->coldObserver:TailObserver{};
#else
    gpu->tailObserver=(activeTailProbe||(evidence&&evidence->armed))?gpu->coldObserver:TailObserver{};
#endif`);
 s=one(s,'    Mark(BridgeStage::DrawBegin,seen,last);Hook(BridgeStage::DrawBegin);',guard('bootProbe.Root(seen,{"DRAW_ENTER",Stamp(),BridgeTid(),m1::NonceOf(presenting),uint64_t(width),uint64_t(height)});')+'    Mark(BridgeStage::DrawBegin,seen,last);Hook(BridgeStage::DrawBegin);');
 s=one(s,'     v.app->Mark(BridgeStage::SwapBegin,v.epoch);',guard('v.app->bootProbe.Root(v.epoch,{"SWAP_ENTER",v.app->Stamp(),BridgeTid(),m1::NonceOf(v.root)});')+'     v.app->Mark(BridgeStage::SwapBegin,v.epoch);');
 s=one(s,'void created(OH_NativeXComponent*,void* w){app().invalidate(w,true);}\nvoid changed(OH_NativeXComponent*,void* w){app().invalidate(w,true);}\nvoid destroyed(OH_NativeXComponent*,void*){app().invalidate(nullptr,true);}',`#ifdef GRS_T456_BOOT
void bootSurfaceEvent(OH_NativeXComponent* c,void* w,const char* reason){
 uint64_t width=0,height=0;const int size=(c&&w)?OH_NativeXComponent_GetXComponentSize(c,w,&width,&height):-1;
 auto& a=app();{std::lock_guard<std::mutex> lock(a.mutex);a.bootTransitionLocked("XCOMPONENT_SIZE",a.epoch,width,height,size);}
 a.invalidate(w,true,reason,width,height);
}
void created(OH_NativeXComponent* c,void* w){bootSurfaceEvent(c,w,"SURFACE_CREATED");}
void changed(OH_NativeXComponent* c,void* w){bootSurfaceEvent(c,w,"SURFACE_CHANGED");}
void destroyed(OH_NativeXComponent*,void*){app().invalidate(nullptr,true,"SURFACE_DESTROYED");}
#else
void created(OH_NativeXComponent*,void* w){app().invalidate(w,true);}
void changed(OH_NativeXComponent*,void* w){app().invalidate(w,true);}
void destroyed(OH_NativeXComponent*,void*){app().invalidate(nullptr,true);}
#endif`);
 const nativeExport=`napi_value bootDiagnostics(napi_env e,napi_callback_info){
 auto data=app().bootProbe.Copy();std::ostringstream out;
 boot456::Json(out,data,m1::kExperienceProgressV2?"T456_EXPERIMENT":"T456_REGULAR",uint64_t(getpid()));
 return text(e,out.str());
}`;
 s=one(s,'napi_value status(napi_env e,napi_callback_info)',guard(nativeExport)+'napi_value status(napi_env e,napi_callback_info)');
 s=one(s,'napi_property_descriptor d[]={','napi_property_descriptor d[]={'+guard('{"bootDiagnostics",nullptr,bootDiagnostics,nullptr,nullptr,nullptr,napi_default,nullptr},'));
 s=one(s,'napi_unwrap(e,v,reinterpret_cast<void**>(&c));if(c)OH_NativeXComponent_RegisterCallback(c,&callbacks);',`const auto unwrapped=napi_unwrap(e,v,reinterpret_cast<void**>(&c));
#ifdef GRS_T456_BOOT
 {auto& a=app();std::lock_guard<std::mutex> l(a.mutex);a.bootTransitionLocked("XCOMPONENT_UNWRAP",a.epoch,0,0,unwrapped);}
 if(c){const int registered=OH_NativeXComponent_RegisterCallback(c,&callbacks);auto& a=app();std::lock_guard<std::mutex> l(a.mutex);a.bootTransitionLocked("XCOMPONENT_REGISTER",a.epoch,0,0,registered);}
#else
 (void)unwrapped;if(c)OH_NativeXComponent_RegisterCallback(c,&callbacks);
#endif
`);
 s=one(s,'a.optics=cold;a.requiresReset=false;a.InvalidateLocked();','a.optics=cold;a.requiresReset=false;'+guard('const auto bootBefore=a.epoch.load();')+'a.InvalidateLocked();'+guard('a.bootTransitionLocked("OPTICS_RESET",bootBefore);'));
 s=one(s,'a.K=uint32_t(k);a.Kw=uint32_t(kw);a.requiresReset=false;a.InvalidateLocked();','a.K=uint32_t(k);a.Kw=uint32_t(kw);a.requiresReset=false;'+guard('const auto bootBefore=a.epoch.load();')+'a.InvalidateLocked();'+guard('a.bootTransitionLocked("PROFILE_RESET",bootBefore,a.K,a.Kw);'));
 return s;
});
edit('cpp/types/libgrsm1/index.d.ts',s=>s+'\nexport const bootDiagnostics:()=>string;\n');
for(const page of ['GpuSandWaterExperience','GpuResidentM1'])edit('ets/pages/'+page+'.ets',s=>{
 if(page==='GpuSandWaterExperience')s=`import { common } from '@kit.AbilityKit';\nimport { DiagnosticColdFiles } from '../physics/DiagnosticColdFiles';\n`+s;
 const anchor=page==='GpuResidentM1'?' build(){':'  build() {';
 return one(s,anchor,` private async exportBootProbe():Promise<void>{
  try{
   const context=getContext(this) as common.UIAbilityContext;
   const raw:string=m1.bootDiagnostics();
   const saved=await new DiagnosticColdFiles(context.filesDir).save('t456-boot-'+Date.now()+'.json',raw);
   console.info('WS_EVENT t456_boot '+JSON.stringify(saved));
  }catch(e){console.error('T456 export '+String(e));}
 }
`+anchor).replace(page==='GpuResidentM1'?"Text('沙水 DEMO V4 · GPU M1')":"Text('GPU 沙水体验 · ' + this.progress)",
  page==='GpuResidentM1'?"Button('导出启动诊断').onClick(()=>this.exportBootProbe())\n  Text('沙水 DEMO V4 · GPU M1')":
   "Button('导出启动诊断').onClick(()=>this.exportBootProbe())\n      Text('GPU 沙水体验 · ' + this.progress)");
});
fs.writeFileSync(path.join(out,'t456-manifest.json'),JSON.stringify({revision:'T451_GPU_BOOT_PROBE_V1',base:'ac4abad',profile,device:'NOT_RUN'},null,2));
edit('cpp/gpu_resident/submit_observer.h',s=>one(s,'SubmitRootBegin=40,SubmitCancel,SubmitWarmup','SubmitRootBegin=40,SubmitCancel,SubmitWarmup, BootCapability=140, BootAllocation'));
edit('cpp/gpu_resident/m1/source_transport.cpp',s=>{
 s=one(s,' if(!Good()||bindings<4||blocks<4||alignment<=0||alignment%4||invocations<64||group<64||limit<=0)return fail(TransportCode::CAPABILITY_UNSUPPORTED);',` const bool capsGood=Good()&&bindings>=4&&blocks>=4&&alignment>0&&alignment%4==0&&invocations>=64&&group>=64&&limit>0;
 submitObserver.Emit(SubmitStage::BootCapability,uint64_t(limit),capsGood?0:stats.firstGlError,1,(uint64_t(uint32_t(bindings))<<32)|uint32_t(blocks));
 if(!capsGood)return fail(TransportCode::CAPABILITY_UNSUPPORTED);`);
 return one(s,'stats.allocatedBytes+=n*4;return b!=0&&Good();','stats.allocatedBytes+=n*4;const bool good=b!=0&&Good();const uint32_t object=&b==&work?5u:&b==&scratchEvent?6u:(&b==&banks[0]||&b==&banks[1])?1u:(&b==&events[0]||&b==&events[1])?2u:(&b==&pubs[0]||&b==&pubs[1])?3u:4u;submitObserver.Emit(SubmitStage::BootAllocation,n*4,good?0:stats.firstGlError,object,stats.allocatedBytes);return good;');
});
edit('cpp/gpu_resident/m1/quantum_candidate.cpp',s=>{
 s=one(s,'if(blocks<4||bindings<4||inv<64||group<64||limit<GLint64(eventCapacity)*256){Release();return TransportCode::CAPABILITY_UNSUPPORTED;}',`const bool capable=blocks>=4&&bindings>=4&&inv>=64&&group>=64&&limit>=GLint64(eventCapacity)*256;
 submitObserver.Emit(SubmitStage::BootCapability,uint64_t(limit),capable?0:1,2,uint64_t(eventCapacity)*256);
 if(!capable){Release();return TransportCode::CAPABILITY_UNSUPPORTED;}`);
 return one(s,'allocated+=uint64_t(n)*4;};alloc(candidate,words);','allocated+=uint64_t(n)*4;submitObserver.Emit(SubmitStage::BootAllocation,uint64_t(n)*4,0,&b==&candidate?7u:&b==&workspace?8u:9u,allocated);};alloc(candidate,words);');
});
edit('cpp/gpu_resident/app_bridge.cpp',s=>one(s,'if(!app.timelinePaused)app.timeline.Add(row);',`if(stage==SubmitStage::BootCapability||stage==SubmitStage::BootAllocation){
#ifdef GRS_T456_BOOT
 app.bootNote(app.transportEpoch,stage==SubmitStage::BootCapability?"TRANSPORT_CAPABILITY":"TRANSPORT_BUFFER",false,false,code,0,stage==SubmitStage::BootAllocation?code:0,0,root,ordinal,wallNs);
#endif
 return;
}if(!app.timelinePaused)app.timeline.Add(row);`));
edit('cpp/gpu_resident/submit_observer.h',s=>one(s,' void Emit(SubmitStage s',` void (*log)(void*,const char*)=nullptr;
 void LogFailure(GLuint handle,bool program)const{
  if(!log)return;
  char message[512]{};
  if(program)glGetProgramInfoLog(handle,sizeof(message),nullptr,message);
  else glGetShaderInfoLog(handle,sizeof(message),nullptr,message);
  log(context,message);
 }
 void Emit(SubmitStage s`));
for(const file of ['source_transport.cpp','quantum_candidate.cpp','full_transport.cpp'])edit('cpp/gpu_resident/m1/'+file,s=>{
 s=s.replace(/if\(!((?:ok|good))\)\{glDeleteShader\(sh\);/g, 'if(!$1){submitObserver.LogFailure(sh,false);glDeleteShader(sh);');
 s=s.replace(/if\(!((?:ok|good))\)\{Release\(\);return TransportCode.SHADER_FAILED;\}/g,
    'if(!$1){submitObserver.LogFailure('+ (file==='full_transport.cpp'?'quantum.program':'program') +',true);Release();return TransportCode::SHADER_FAILED;}');
 if(file==='source_transport.cpp')s=s.replace('if(!ok)return fail(TransportCode::SHADER_FAILED);','if(!ok){submitObserver.LogFailure(program,true);return fail(TransportCode::SHADER_FAILED);}');
 return s;
});
edit('cpp/gpu_resident/app_bridge.cpp',s=>{
 s=one(s,'}};\n#ifdef GRS_APP_M1\n    gpu->coldObserver=',`}};
#ifdef GRS_T456_BOOT
    gpu->submitObserver.log=[](void* ptr,const char* text){auto& a=*static_cast<App*>(ptr);a.bootProbe.ShaderLog(a.transportEpoch,text);};
#endif
#ifdef GRS_APP_M1
    gpu->coldObserver=`);
 s=s.replaceAll('    QuotaTraceLocked(TailStage::ProgressTimeout,1);',guard('bootNote(permit.epoch,"V2_GPU_PROGRESS_TIMEOUT",true,true,0,0,0,permit.nonce,permitStarted,permit.deadline,Stamp());')+'    QuotaTraceLocked(TailStage::ProgressTimeout,1);');
 return s;
});
