 void bootNote(uint64_t e,const char* stage,bool fail=false,bool terminal=false,int64_t result=0,uint32_t rawEgl=0,uint32_t rawGl=0,
               uint64_t root=0,uint64_t a=0,uint64_t b=0,uint64_t c=0){
  bootProbe.Mark(e,{stage,Stamp(),BridgeTid(),root,a,b,c,result,rawEgl,rawGl},fail,terminal);
 }
 void bootIdentityLocked(uint64_t e,uint64_t candidate=0){
  bootProbe.Identity(e,candidate,(uint64_t(identity.run.hi)<<32)|identity.run.lo,readyEpoch,progress.session,nativeViewportWidth,nativeViewportHeight);
 }
 void bootTransitionLocked(const char* reason,uint64_t before,uint64_t a=0,uint64_t b=0,int64_t result=0){
  bootProbe.TransitionAt({reason,Stamp(),BridgeTid(),before,epoch.load(),bootSurface,a,b,result},true);
 }
 void bootRootEvent(uint64_t e,const BridgeTimeRow& row){
  bootProbe.Root(e,{"SUBMIT_OBSERVER",row.ns,row.tid,row.root,row.stage,row.carry,row.delta,row.code});
 }
 bool bootCreateEgl(uint64_t e,void* held,EGLDisplay& display,EGLContext& context,EGLSurface& surface,uint32_t& raw){
  auto stamp=[&]{return Stamp();};
  auto error=[&]{raw=eglGetError();return raw;};
  auto emit=[&](const boot456::Point& point,bool fail){bootProbe.Mark(e,point,fail,fail);};
  auto call=[&](const char* enter,const char* exit,auto fn,auto good){
   return boot456::ObserveCall(enter,exit,BridgeTid(),fn,good,error,stamp,emit);
  };
  auto yes=[](EGLBoolean v){return v==EGL_TRUE;};
  display=call("EGL_GET_DISPLAY_ENTER","EGL_GET_DISPLAY_EXIT",[]{return eglGetDisplay(EGL_DEFAULT_DISPLAY);},[](EGLDisplay v){return v!=EGL_NO_DISPLAY;});
  if(display==EGL_NO_DISPLAY)return false;
  EGLint major=0,minor=0,count=0;EGLConfig cfg{};
  if(!call("EGL_INITIALIZE_ENTER","EGL_INITIALIZE_EXIT",[&]{return eglInitialize(display,&major,&minor);},yes))return false;
  bootNote(e,"EGL_VERSION",false,false,0,0,0,0,major,minor);
  if(!call("EGL_BIND_API_ENTER","EGL_BIND_API_EXIT",[]{return eglBindAPI(EGL_OPENGL_ES_API);},yes))return false;
  const EGLint attrs[]={EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
  const EGLint ctx[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
  if(!call("EGL_CHOOSE_CONFIG_ENTER","EGL_CHOOSE_CONFIG_EXIT",[&]{return eglChooseConfig(display,attrs,&cfg,1,&count);},yes))return false;
  bootNote(e,"EGL_CONFIG_COUNT",count!=1,count!=1,count,0,0,0,EGL_WINDOW_BIT,EGL_OPENGL_ES3_BIT);
  if(count!=1){raw=eglGetError();return false;}
  context=call("EGL_CREATE_CONTEXT_ENTER","EGL_CREATE_CONTEXT_EXIT",[&]{return eglCreateContext(display,cfg,EGL_NO_CONTEXT,ctx);},[](EGLContext v){return v!=EGL_NO_CONTEXT;});
  // Match the original call order even when creating the context failed.
  const uint32_t contextError=raw;
  surface=call("EGL_CREATE_SURFACE_ENTER","EGL_CREATE_SURFACE_EXIT",[&]{return eglCreateWindowSurface(display,cfg,reinterpret_cast<EGLNativeWindowType>(held),nullptr);},[](EGLSurface v){return v!=EGL_NO_SURFACE;});
  if(context==EGL_NO_CONTEXT){raw=contextError;return false;}
  if(surface==EGL_NO_SURFACE)return false;
  return call("EGL_MAKE_CURRENT_ENTER","EGL_MAKE_CURRENT_EXIT",[&]{return eglMakeCurrent(display,surface,surface,context);},yes)==EGL_TRUE;
 }
 EGLBoolean bootQuery(uint64_t e,EGLDisplay d,EGLSurface s,EGLint attr,EGLint* value){
  bootNote(e,"EGL_QUERY_SURFACE_ENTER",false,false,attr);
  const auto result=eglQuerySurface(d,s,attr,value);
  const uint32_t raw=result?0:eglGetError();
  bootNote(e,"EGL_QUERY_SURFACE_EXIT",!result,!result,result,raw,0,0,attr,result?*value:0);
  return result;
 }
