static uint64_t requests507[2]{},ids507[2]{};static bool done507[2]{};static int export507=-1;
bool TraceCheckpoint507(App& a,unsigned index){
 if(done507[index])return true;
 if(!requests507[index]){const auto scope=a.ColdScopeLocked();uint64_t request=0,id=0;const auto code=a.coldTrace.Begin(scope,scope,a.Stamp(),index?ids507[0]:0,cold503::ProfileWords(a.K,a.Kw),request,id,size_t(16*a.K+16*a.Kw+2304));assert(std::string(code)=="REQUESTED");requests507[index]=request;ids507[index]=id;return false;}
 const auto state=a.coldTrace.StateOf(requests507[index],a.Stamp(),a.ColdScopeLocked());assert(std::string(state.result)=="REQUESTED"||std::string(state.result)=="BUILDING"||std::string(state.result)=="TRACE_READY");
 if(std::string(state.result)=="TRACE_READY")export507=int(index);return false;
}
static void SaveTrace507(App& a,const std::string& directory){
 if(export507<0)return;const unsigned index=unsigned(export507);export507=-1;cold503::TraceManager::Read read;
 {std::lock_guard<std::mutex> lock(a.mutex);assert(std::string(a.coldTrace.Acquire(ids507[index],0,a.Stamp(),a.ColdScopeLocked(),read))=="OK");assert(a.coldTrace.Capacity()<=cold503::TraceBudget&&a.coldTrace.Reserved()==0);}
 assert(read.block[27]==(index?2u:1u));
 std::array<char,cold503::PageBytes> buffer{};const auto pages=(256+read.stateWords+read.eventWords+1023)/1024;
 for(uint32_t page=0;page<=pages;++page){cold503::Writer w(buffer.data(),buffer.size());cold503::TracePage(w,read,page);assert(w.Good());std::ofstream(directory+"/trace-"+std::to_string(index)+"-"+std::to_string(page)+".json").write(w.Data(),w.Size());}
 {std::lock_guard<std::mutex> lock(a.mutex);assert(std::string(a.coldTrace.Release(read,a.Stamp(),a.ColdScopeLocked()))=="TRACE_READY");if(index)assert(std::string(a.coldTrace.End(ids507[index],a.Stamp(),a.ColdScopeLocked()))=="ENDED");}
 done507[index]=true;
 if(index){
  auto& gpu=*static_cast<AppTransport*>(observedTransport507);assert(gpu.Freeze(true)==TransportCode::OK);
  const size_t capacity=cold503::ProfileWords(a.K,a.Kw)-256;auto span=std::unique_ptr<uint32_t[]>(new uint32_t[capacity]);size_t state=0,events=0;GLenum raw=0;const char* stage="";
  assert(gpu.CaptureSpan(span.get(),capacity,state,events,raw,stage)==TransportCode::OK);
  std::vector<uint32_t> oldState,oldEvents;assert(gpu.Capture(oldState,oldEvents)==TransportCode::OK);assert(oldState.size()==state&&oldEvents.size()==events&&std::equal(oldState.begin(),oldState.end(),span.get())&&std::equal(oldEvents.begin(),oldEvents.end(),span.get()+state));
  assert(gpu.CaptureSpan(span.get(),0,state,events,raw,stage)==TransportCode::PROTOCOL_REJECTED);
  mapFault507=1;assert(gpu.CaptureSpan(span.get(),capacity,state,events,raw,stage)==TransportCode::GL_FAILED&&raw==GL_INVALID_ENUM&&std::string(stage)=="STATE_MAP");
  unmapFault507=1;assert(gpu.CaptureSpan(span.get(),capacity,state,events,raw,stage)==TransportCode::GL_FAILED&&std::string(stage)=="STATE_UNMAP");
  auto& completion=const_cast<Words256&>(gpu.LastCompletion());const auto saved=completion[53];completion[53]=UINT32_MAX;assert(gpu.CaptureSpan(span.get(),capacity,state,events,raw,stage)==TransportCode::PROTOCOL_REJECTED);completion[53]=saved;
  auto display=eglGetCurrentDisplay();auto surface=eglGetCurrentSurface(EGL_DRAW);auto context=eglGetCurrentContext();assert(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));assert(gpu.Freeze(false)==TransportCode::WRONG_OWNER);assert(eglMakeCurrent(display,surface,surface,context));assert(gpu.Freeze(false)==TransportCode::OK);
  std::ofstream(directory+"/span-checks.json")<<"{\"spanVsLegacy\":true,\"smallCapacity\":true,\"mapFailure\":true,\"unmapFailure\":true,\"completionOverflow\":true,\"restoreWrongOwner\":true}";
  cold503::SmallDiagnostic summary;{std::lock_guard<std::mutex> lock(a.mutex);const auto scope=a.ColdScopeLocked();assert(std::string(a.diagnosticGate.Begin(scope,scope,2097152))=="OK");summary.Copy(a.diagnostics,scope);}
  {cold503::GateLease lease{a.diagnosticGate,a.mutex};auto copy=std::make_unique<diagnostic492::Capture>();auto buffer=std::unique_ptr<char[]>(new char[2097152]);{std::lock_guard<std::mutex> lock(a.mutex);*copy=a.diagnostics;++a.diagnosticGate.copied;}cold503::Writer writer(buffer.get(),2097152);cold503::DiagnosticJson(writer,*copy,summary,2097152,false);assert(writer.Good());std::ofstream(directory+"/bounded-diagnostic.json").write(writer.Data(),writer.Size());}
 }
}
