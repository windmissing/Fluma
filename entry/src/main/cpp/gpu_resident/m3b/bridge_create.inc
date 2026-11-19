// T653/T650 configuration-stage observations on the existing GL owner.
if(m5Selected){
 using namespace grs::m3b::init650;
 grs::m3b::PhysicalConfigV5 cold;grs::m3b::ControlIdentityV5 identityV5;uint64_t ticket=0;
 {std::lock_guard<std::mutex> lock(mutex);cold=m5Config;ticket=m5InitTicket;
  if(!ticket||m5InitEpoch!=seen||!m5InitStore.Live(ticket)||m5Generation==UINT64_MAX){failed=true;continue;}
  uint64_t permissionValue=seen;
#ifdef GRS_APP_M1
  permissionValue=permissionRevision;
#endif
  identityV5={runId,1,seen,seen,++m5Generation,permissionValue,seen,1,seen};m5Identity=identityV5;
 }
 m5InitStore.Bind(ticket,seen,uint64_t(reinterpret_cast<uintptr_t>(held)),uint64_t(reinterpret_cast<uintptr_t>(context)),&identityV5);
 m5InitStore.Emit(ticket,Code::WINDOW_RETAIN,"EXIT",0,Life::OWNER_CREATE);
 m5InitStore.Emit(ticket,Code::EGL_READY,"EXIT",0);
 std::array<uint32_t,8> shaderSha{},angleSha{},layoutSha{};
 hash(grs::m3b::RootKernelShaV5(),shaderSha);hash(grs::kAngleSha256,angleSha);hash(grs::m3b::kDesignSha,layoutSha);
 auto next=std::make_shared<grs::m3b::ControllerV5>();next->SetInitDiagnostics({&m5InitStore,ticket});
 m5InitStore.Emit(ticket,Code::CONTROLLER_CONFIGURE,"ENTER");
 auto result=next->Configure(*grs::m3b::SelectProfile(capacity,waterCapacity),identityV5,seen,cold,shaderSha,angleSha,layoutSha);
 m5InitStore.Emit(ticket,Code::CONTROLLER_CONFIGURE,"EXIT",int32_t(result),result==grs::m3b::ControlResult::OK?Life::CONTROLLER_CONFIGURED:Life::FAILED);
 if(result!=grs::m3b::ControlResult::OK){next->Release(true);publish("{\"state\":\"FAULT\",\"stage\":\"M5_CREATE\"}");failed=true;continue;}
 m5InitStore.Emit(ticket,Code::SCENE_PROGRAM,"ENTER");
 bool sceneReady=false;{grs::m3b::init650::ProgramScope watch({&m5InitStore,ticket},Code::SCENE_PROGRAM,4,{});sceneReady=m5Scene.PreparePrograms(identityV5.context);}
 m5InitStore.Emit(ticket,Code::SCENE_PROGRAM,"EXIT",sceneReady?1:0,sceneReady?Life::NONE:Life::FAILED);
 if(!sceneReady){next->Release(true);publish("{\"state\":\"FAULT\",\"stage\":\"M5_SCENE_CREATE\"}");failed=true;continue;}
 EGLint m5Width=0,m5Height=0;eglQuerySurface(display,surface,EGL_WIDTH,&m5Width);eglQuerySurface(display,surface,EGL_HEIGHT,&m5Height);
 if(m5Width<=0||m5Height<=0){m5InitStore.Emit(ticket,Code::SCENE_TARGETS,"CHECK",-1,Life::FAILED);next->Release(true);publish("{\"state\":\"FAULT\",\"stage\":\"M5_FRAME_SIZE\"}");failed=true;continue;}
 m5InitStore.Emit(ticket,Code::SCENE_TARGETS,"ENTER");
 bool targetsReady=m5Width>0&&m5Height>0&&m5Scene.PrepareTargets(uint32_t(m5Width),uint32_t(m5Height),identityV5.context);
 m5InitStore.Emit(ticket,Code::SCENE_TARGETS,"EXIT",targetsReady?1:0,targetsReady?Life::SCENE_READY:Life::FAILED);
 if(!targetsReady){next->Release(true);publish("{\"state\":\"FAULT\",\"stage\":\"M5_FRAME_TARGET_CREATE\"}");failed=true;continue;}
 {std::lock_guard<std::mutex> lock(mutex);
  if(!CurrentLocked(seen,held)||ticket!=m5InitTicket||m5InitEpoch!=seen||!m5InitStore.Live(ticket)){next->Stop();m5InitStore.Cancel(ticket);}
  else{m5Controller=next;m5InitStore.Emit(ticket,Code::PUBLISH_CONTROLLER,"EXIT",0,Life::PUBLISHED_WAIT_PAGE_PULSE);phase=permission="M5_WAIT_PAGE_PULSE";status="{\"state\":\"M5_WAIT_PAGE_PULSE\",\"wire\":5,\"features\":31}";}
 }
 m5Local=next;m5LastDisplay=0;m5Working=false;continue;
}
