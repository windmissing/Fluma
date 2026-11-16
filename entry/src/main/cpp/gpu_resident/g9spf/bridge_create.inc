if(g9Selected){
 grs::g9spf::Config config;grs::g9spf::ControlIdentity control;
 {std::lock_guard<std::mutex> lock(mutex);config=g9Config;if(g9Generation==UINT64_MAX){failed=true;continue;}control={runId,1,seen,seen,++g9Generation,permissionRevision,seen,1,seen};g9Identity=control;}
 auto next=std::make_shared<grs::g9spf::Controller>();grs::g9spf::SeedFileIdentity seed;
 auto result=next->ConfigureResearch(config,control,seed);
 if(result!=grs::g9spf::Result::OK){next->Release(true);publish("{\"state\":\"FAULT\",\"stage\":\"G9_CREATE\",\"code\":"+std::to_string(uint32_t(result))+"}");failed=true;continue;}
 EGLint width=0,height=0;eglQuerySurface(display,surface,EGL_WIDTH,&width);eglQuerySurface(display,surface,EGL_HEIGHT,&height);
 if(width<128||height<=0||!g9Scene.PreparePrograms(control.context)||!g9Scene.PrepareTargets(uint32_t(width),uint32_t(height),control.context)){next->Release(true);publish("{\"state\":\"FAULT\",\"stage\":\"G9_SCENE_CREATE\"}");failed=true;continue;}
 {std::lock_guard<std::mutex> lock(mutex);if(!CurrentLocked(seen,held))next->Stop();else{g9Controller=next;phase=permission="G9_WAIT_PAGE_PULSE";status="{\"state\":\"G9_WAIT_PAGE_PULSE\",\"profile\":\"G9_SPF_Q20_V1\"}";}}
 g9Local=next;g9LastDisplay=0;g9LastDraw=0;g9Working=false;continue;
}
