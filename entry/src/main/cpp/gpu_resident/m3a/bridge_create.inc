// Existing owner has retired old route/EGL before this explicit route branch.
if(m3Selected){
 grs::m3a::PhysicalConfigV4 cold;grs::m3a::ControlIdentityV4 identityV4;
 {std::lock_guard<std::mutex> lock(mutex);cold=m3Config;
  if(m3Generation==UINT64_MAX){failed=true;continue;}
  uint64_t permissionValue=seen;
#ifdef GRS_APP_M1
  permissionValue=permissionRevision;
#endif
  identityV4={runId,1,seen,seen,++m3Generation,permissionValue,seen,1,seen};m3Identity=identityV4;
 }
 std::array<uint32_t,8> shaderSha{},angleSha{},layoutSha{};
 hash(grs::m3a::RootKernelShaV4(),shaderSha);hash(grs::kAngleSha256,angleSha);hash(grs::m3a::kDesignSha,layoutSha);
 auto next=std::make_shared<grs::m3a::ControllerV4>();
 auto result=next->Configure(*grs::m3a::SelectProfile(capacity,waterCapacity),identityV4,seen,cold,shaderSha,angleSha,layoutSha);
 if(result!=grs::m3a::ControlResult::OK){next->Release(true);publish("{\"state\":\"FAULT\",\"stage\":\"M3_CREATE\"}");failed=true;continue;}
 if(!m3Scene.PreparePrograms(identityV4.context)){next->Release(true);publish("{\"state\":\"FAULT\",\"stage\":\"M3_SCENE_CREATE\"}");failed=true;continue;}
 EGLint m3Width=0,m3Height=0;eglQuerySurface(display,surface,EGL_WIDTH,&m3Width);eglQuerySurface(display,surface,EGL_HEIGHT,&m3Height);
 if(m3Width<=0||m3Height<=0||!m3Scene.PrepareTargets(uint32_t(m3Width),uint32_t(m3Height),identityV4.context)){next->Release(true);publish("{\"state\":\"FAULT\",\"stage\":\"M3_FRAME_TARGET_CREATE\"}");failed=true;continue;}
 {std::lock_guard<std::mutex> lock(mutex);
  if(!CurrentLocked(seen,held)){next->Stop();}else{m3Controller=next;phase=permission="M3_WAIT_PAGE_PULSE";status="{\"state\":\"M3_WAIT_PAGE_PULSE\",\"wire\":4,\"features\":15}";}
 }
 m3Local=next;m3LastDisplay=0;m3Working=false;continue;
}
