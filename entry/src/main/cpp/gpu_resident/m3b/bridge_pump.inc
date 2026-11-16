if(m5Selected){
 if(!m5Local)continue;
 if(failed){m5Working=false;m5Local->DrainRevoked();continue;}
 bool allowed;{std::lock_guard<std::mutex> lock(mutex);allowed=CurrentLocked(seen,held)&&active;}
 if(!allowed){m5Local->Stop();m5Local->DrainRevoked();continue;}
 m5Local->BuildCold(now());
 auto quota=m5Local->Quota(now());
 if(quota[8]){
  auto code=m5Local->Pump(now());
  if(code!=grs::m3b::TransportResultV5::OK&&code!=grs::m3b::TransportResultV5::IN_PROGRESS){publish("{\"state\":\"FAULT\",\"stage\":\"M5_ROOT\",\"code\":"+std::to_string(uint32_t(code))+"}");failed=true;continue;}
 }else{
  // V3 admission owns P/X and SOURCE-only plans; never run a legacy dt clock.
  if(m5Local->Advance(now(),frame+1,frame+1)==grs::m3b::ControlResult::OK)++frame;
 }
 const auto afterQuota=m5Local->Quota(now());m5Working=afterQuota[4]==0u&&(afterQuota[7]>0u||afterQuota[8]>0u);
 const auto h=m5Local->SceneHeaderV5();const uint64_t generation=uint64_t(h[72])|(uint64_t(h[73])<<32);
 if(generation&&generation!=m5LastDisplay&&now()-m5LastDrawNs>=33333333){
  const auto view=m5Local->PublicationView();
  if(!view.valid)continue;
  EGLint width=0,height=0;eglQuerySurface(display,surface,EGL_WIDTH,&width);eglQuerySurface(display,surface,EGL_HEIGHT,&height);
  ResidentSceneFrame f;f.run=u64(m5Identity.run);f.context=u64(seen);f.surface=f.geometry=u64(seen);f.renderFrame=++render;f.width=width;f.height=height;f.boxWidth=float(width);f.boxHeight=float(height);
  struct M5Swap {App* app;grs::m3b::ControllerV5* controller;uint64_t epoch,generation;void* window;EGLDisplay display;EGLSurface surface;};M5Swap args{this,m5Local.get(),seen,generation,held,display,surface};
  auto swap=[](void* ptr){auto& a=*static_cast<M5Swap*>(ptr);{std::lock_guard<std::mutex> lock(a.app->mutex);if(!a.app->CurrentLocked(a.epoch,a.window)||!a.app->active)return false;}if(!a.controller->CanPresent(a.generation,now()))return false;const bool native=eglSwapBuffers(a.display,a.surface)==EGL_TRUE;const bool allowed=a.controller->CanPresent(a.generation,now());std::lock_guard<std::mutex> lock(a.app->mutex);return native&&allowed&&a.app->CurrentLocked(a.epoch,a.window)&&a.app->active;};
  auto receipt=m5Scene.Draw(*m5Local,view.slot,u64(generation),f,swap,&args);
  if(!receipt.current){publish("{\"state\":\"FAULT\",\"stage\":\"M5_SCENE\"}");failed=true;continue;}
  m5LastDisplay=generation;m5LastDrawNs=now();
  std::lock_guard<std::mutex> lock(mutex);if(CurrentLocked(seen,held))status="{\"state\":\"M5_CURRENT\",\"wire\":5,\"features\":31,\"token\":\""+receipt.renderedToken+"\"}";
 }
 continue;
}
