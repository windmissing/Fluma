if(m3Selected){
 if(!m3Local)continue;
 if(failed){m3Working=false;m3Local->DrainRevoked();continue;}
 bool allowed;{std::lock_guard<std::mutex> lock(mutex);allowed=CurrentLocked(seen,held)&&active;}
 if(!allowed){m3Local->Stop();m3Local->DrainRevoked();continue;}
 m3Local->BuildCold(now());
 auto quota=m3Local->Quota(now());
 if(quota[8]){
  auto code=m3Local->Pump(now());
  if(code!=grs::m3a::TransportResultV4::OK&&code!=grs::m3a::TransportResultV4::IN_PROGRESS){publish("{\"state\":\"FAULT\",\"stage\":\"M3_ROOT\",\"code\":"+std::to_string(uint32_t(code))+"}");failed=true;continue;}
 }else{
  // V3 admission owns P/X and SOURCE-only plans; never run a legacy dt clock.
  if(m3Local->Advance(now(),frame+1,frame+1)==grs::m3a::ControlResult::OK)++frame;
 }
 const auto afterQuota=m3Local->Quota(now());m3Working=afterQuota[4]==0u&&(afterQuota[7]>0u||afterQuota[8]>0u);
 const auto h=m3Local->SceneHeaderV4();const uint64_t generation=uint64_t(h[72])|(uint64_t(h[73])<<32);
 if(generation&&generation!=m3LastDisplay&&now()-m3LastDrawNs>=33333333){
  const auto view=m3Local->PublicationView();
  if(!view.valid)continue;
  EGLint width=0,height=0;eglQuerySurface(display,surface,EGL_WIDTH,&width);eglQuerySurface(display,surface,EGL_HEIGHT,&height);
  ResidentSceneFrame f;f.run=u64(m3Identity.run);f.context=u64(seen);f.surface=f.geometry=u64(seen);f.renderFrame=++render;f.width=width;f.height=height;f.boxWidth=float(width);f.boxHeight=float(height);
  struct M3Swap {App* app;grs::m3a::ControllerV4* controller;uint64_t epoch,generation;void* window;EGLDisplay display;EGLSurface surface;};M3Swap args{this,m3Local.get(),seen,generation,held,display,surface};
  auto swap=[](void* ptr){auto& a=*static_cast<M3Swap*>(ptr);{std::lock_guard<std::mutex> lock(a.app->mutex);if(!a.app->CurrentLocked(a.epoch,a.window)||!a.app->active)return false;}if(!a.controller->CanPresent(a.generation,now()))return false;const bool native=eglSwapBuffers(a.display,a.surface)==EGL_TRUE;const bool allowed=a.controller->CanPresent(a.generation,now());std::lock_guard<std::mutex> lock(a.app->mutex);return native&&allowed&&a.app->CurrentLocked(a.epoch,a.window)&&a.app->active;};
  auto receipt=m3Scene.Draw(*m3Local,view.slot,u64(generation),f,swap,&args);
  if(!receipt.current){publish("{\"state\":\"FAULT\",\"stage\":\"M3_SCENE\"}");failed=true;continue;}
  m3LastDisplay=generation;m3LastDrawNs=now();
  std::lock_guard<std::mutex> lock(mutex);if(CurrentLocked(seen,held))status="{\"state\":\"M3_CURRENT\",\"wire\":4,\"features\":15,\"token\":\""+receipt.renderedToken+"\"}";
 }
 continue;
}
