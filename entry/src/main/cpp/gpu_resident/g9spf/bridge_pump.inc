if(g9Selected){
 if(!g9Local)continue;
 if(failed){g9Working=false;g9Local->Drain();continue;}
 bool allowed;{std::lock_guard<std::mutex> lock(mutex);allowed=CurrentLocked(seen,held)&&active;}
 if(!allowed){g9Local->Stop();g9Local->Drain();continue;}
 g9Local->BuildCold(now());auto quota=g9Local->Quota(now());
 if(quota[8]){
  const auto result=g9Local->PumpBounded(now());
  if(result!=grs::g9spf::Result::OK&&result!=grs::g9spf::Result::IN_PROGRESS){publish("{\"state\":\"FAULT\",\"stage\":\"G9_ROOT\",\"code\":"+std::to_string(uint32_t(result))+"}");failed=true;continue;}
 }else if(g9Local->BeginRoot(now(),frame+1,seen)==grs::g9spf::Result::OK)++frame;
 quota=g9Local->Quota(now());g9Working=quota[4]==0u&&((quota[5]&(32u|64u))!=0u||quota[8]!=0u);
 if(quota[4]==1u||quota[4]==2u){publish("{\"state\":\"STALE\",\"stage\":\"G9_PLANNING_BLOCKED\"}");continue;}
 const auto tail=g9Local->LastCompletion();const auto generation=grs::g9spf::Pair(tail.data(),30);
 if(generation&&generation!=g9LastDisplay&&now()-g9LastDraw>=33333333){
  EGLint width=0,height=0;eglQuerySurface(display,surface,EGL_WIDTH,&width);eglQuerySurface(display,surface,EGL_HEIGHT,&height);
  struct SwapArgs{App* app;uint64_t epoch;void* window;EGLDisplay display;EGLSurface surface;};SwapArgs args{this,seen,held,display,surface};
  auto swap=[](void* ptr){auto& a=*static_cast<SwapArgs*>(ptr);{std::lock_guard<std::mutex> lock(a.app->mutex);if(!a.app->CurrentLocked(a.epoch,a.window)||!a.app->active)return false;}return eglSwapBuffers(a.display,a.surface)==EGL_TRUE;};
  std::string token;
  if(!g9Scene.Draw(*g9Local,uint32_t(width),uint32_t(height),swap,&args,token)){publish("{\"state\":\"FAULT\",\"stage\":\"G9_SCENE\"}");failed=true;continue;}
  g9LastDisplay=generation;g9LastDraw=now();publish("{\"state\":\"G9_CURRENT\",\"profile\":\"G9_SPF_Q20_V1\",\"token\":\""+token+"\"}");
 }
 continue;
}
