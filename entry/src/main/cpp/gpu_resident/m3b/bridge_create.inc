// Existing owner has retired old route/EGL before this explicit route branch.
if(m5Selected){
 grs::m3b::PhysicalConfigV5 cold;grs::m3b::ControlIdentityV5 identityV5;
 {std::lock_guard<std::mutex> lock(mutex);cold=m5Config;
  if(m5Generation==UINT64_MAX){failed=true;continue;}
  uint64_t permissionValue=seen;
#ifdef GRS_APP_M1
  permissionValue=permissionRevision;
#endif
  identityV5={runId,1,seen,seen,++m5Generation,permissionValue,seen,1,seen};m5Identity=identityV5;
 }
 std::array<uint32_t,8> shaderSha{},angleSha{},layoutSha{};
 hash(grs::m3b::RootKernelShaV5(),shaderSha);hash(grs::kAngleSha256,angleSha);hash(grs::m3b::kDesignSha,layoutSha);
 auto next=std::make_shared<grs::m3b::ControllerV5>();
 auto result=next->Configure(*grs::m3b::SelectProfile(capacity,waterCapacity),identityV5,seen,cold,shaderSha,angleSha,layoutSha);
 if(result!=grs::m3b::ControlResult::OK){next->Release(true);publish("{\"state\":\"FAULT\",\"stage\":\"M5_CREATE\"}");failed=true;continue;}
 if(!m5Scene.PreparePrograms(identityV5.context)){next->Release(true);publish("{\"state\":\"FAULT\",\"stage\":\"M5_SCENE_CREATE\"}");failed=true;continue;}
 EGLint m5Width=0,m5Height=0;eglQuerySurface(display,surface,EGL_WIDTH,&m5Width);eglQuerySurface(display,surface,EGL_HEIGHT,&m5Height);
 if(m5Width<=0||m5Height<=0||!m5Scene.PrepareTargets(uint32_t(m5Width),uint32_t(m5Height),identityV5.context)){next->Release(true);publish("{\"state\":\"FAULT\",\"stage\":\"M5_FRAME_TARGET_CREATE\"}");failed=true;continue;}
 {std::lock_guard<std::mutex> lock(mutex);
  if(!CurrentLocked(seen,held)){next->Stop();}else{m5Controller=next;phase=permission="M5_WAIT_PAGE_PULSE";status="{\"state\":\"M5_WAIT_PAGE_PULSE\",\"wire\":5,\"features\":31}";}
 }
 m5Local=next;m5LastDisplay=0;m5Working=false;continue;
}
