bool EvidenceU64(napi_env,napi_value,const char*,uint64_t&);
bool EvidenceString(napi_env,napi_value,const char*,std::string&,size_t);
bool ColdScopeArg(napi_env e,napi_value arg,cold503::Scope& scope){
 napi_value global,json,parse,object,control;size_t bytes=0;
 if(napi_get_value_string_utf8(e,arg,nullptr,0,&bytes)!=napi_ok||bytes>4096)return false;
 if(napi_get_global(e,&global)!=napi_ok||napi_get_named_property(e,global,"JSON",&json)!=napi_ok||napi_get_named_property(e,json,"parse",&parse)!=napi_ok||napi_call_function(e,json,parse,1,&arg,&object)!=napi_ok)return false;
 std::string route,source;if(!EvidenceU64(e,object,"pid",scope.pid)||!EvidenceString(e,object,"route",route,128)||!EvidenceString(e,object,"source",source,128)||napi_get_named_property(e,object,"control",&control)!=napi_ok)return false;
 std::strncpy(scope.route.data(),route.c_str(),128);std::strncpy(scope.source.data(),source.c_str(),128);auto& s=scope.control;
 return EvidenceU64(e,control,"epoch",s.epoch)&&EvidenceU64(e,control,"run",s.run)&&EvidenceU64(e,control,"context",s.context)&&EvidenceU64(e,control,"surface",s.surface)&&EvidenceU64(e,control,"session",s.session)&&EvidenceU64(e,control,"resetSerial",s.reset)&&EvidenceU64(e,control,"permissionRevision",s.permission);
}
napi_value ColdText(napi_env e,cold503::Writer& writer){napi_value value=nullptr;if(!writer.Good())return text(e,"{\"state\":\"INCOMPLETE\",\"reason\":\"COLD_ENVELOPE_BUDGET\"}");if(napi_create_string_utf8(e,writer.Data(),writer.Size(),&value)!=napi_ok)return nullptr;return value;}
napi_value coldCapabilities(napi_env e,napi_callback_info){
 auto& a=app();cold503::Scope scope;{std::lock_guard<std::mutex> lock(a.mutex);a.ColdMaintainLocked();scope=a.ColdScopeLocked();}
 std::array<char,4096> raw{};cold503::Writer w(raw.data(),raw.size());w.Raw("{\"revision\":\"T503_BOUNDED_COLD_V1\",\"traceRevision\":\"T503_M2_COLD_TRACE_V1\",\"payloadRevision\":\"T445_M2_TRACE_V3\",\"traceSupported\":");w.Bool(!std::strcmp(kT492Route,"m2"));w.Raw(",\"traceSlots\":2,\"traceCapacityBytes\":55410688,\"readers\":1,\"builders\":1,\"diagnosticExports\":1,\"diagnosticBytes\":2097152,\"pageBytes\":16384,\"pageWords\":1024,\"ttlNs\":\"60000000000\",\"scope\":");cold503::ScopeJson(w,scope);w.Raw("}");return ColdText(e,w);
}
napi_value coldTraceBeginV1(napi_env e,napi_callback_info info){
 size_t n=2;napi_value args[2];cold503::Scope expected;uint64_t replace=0;if(napi_get_cb_info(e,info,&n,args,nullptr,nullptr)!=napi_ok||n!=2||!ColdScopeArg(e,args[0],expected)||!PageU64(e,args[1],replace))return text(e,"{\"state\":\"INVALID_COLD_ARGUMENT\"}");
 auto& a=app();cold503::TraceManager::Receipt receipt;
 {std::lock_guard<std::mutex> lock(a.mutex);uint64_t request=0,id=0;const auto actual=a.ColdScopeLocked();const auto result=a.coldTrace.Begin(expected,actual,a.Stamp(),replace,cold503::ProfileWords(a.K,a.Kw),request,id,size_t(16*a.K+16*a.Kw+2304));receipt={request,id,actual,result};if(request)a.wake.notify_one();}
 std::array<char,4096> raw{};cold503::Writer w(raw.data(),raw.size());cold503::ReceiptJson(w,receipt);return ColdText(e,w);
}
napi_value coldTraceStateV1(napi_env e,napi_callback_info info){
 size_t n=1;napi_value arg;uint64_t id=0;if(napi_get_cb_info(e,info,&n,&arg,nullptr,nullptr)!=napi_ok||n!=1||!PageU64(e,arg,id))return text(e,"{\"state\":\"INVALID_COLD_ARGUMENT\"}");
 auto& a=app();cold503::TraceManager::Receipt receipt;{std::lock_guard<std::mutex> lock(a.mutex);receipt=a.coldTrace.StateOf(id,a.Stamp(),a.ColdScopeLocked());}
 std::array<char,4096> raw{};cold503::Writer w(raw.data(),raw.size());cold503::ReceiptJson(w,receipt);return ColdText(e,w);
}
napi_value coldTraceEndV1(napi_env e,napi_callback_info info){
 size_t n=1;napi_value arg;uint64_t id=0;if(napi_get_cb_info(e,info,&n,&arg,nullptr,nullptr)!=napi_ok||n!=1||!PageU64(e,arg,id))return text(e,"{\"state\":\"INVALID_COLD_ARGUMENT\"}");
 auto& a=app();const char* result;{std::lock_guard<std::mutex> lock(a.mutex);result=a.coldTrace.End(id,a.Stamp(),a.ColdScopeLocked());}
 std::array<char,256> raw{};cold503::Writer w(raw.data(),raw.size());cold503::Envelope(w,result);return ColdText(e,w);
}
napi_value coldTracePageV1(napi_env e,napi_callback_info info){
 size_t n=2;napi_value args[2];uint64_t id=0;double page=0;if(napi_get_cb_info(e,info,&n,args,nullptr,nullptr)!=napi_ok||n!=2||!PageU64(e,args[0],id)||napi_get_value_double(e,args[1],&page)!=napi_ok||!std::isfinite(page)||page<0||page>UINT32_MAX||std::floor(page)!=page)return text(e,"{\"state\":\"TRACE_BAD_PAGE\"}");
 auto& a=app();cold503::TraceManager::Read read;const char* result;
 {std::lock_guard<std::mutex> lock(a.mutex);result=a.coldTrace.Acquire(id,uint32_t(page),a.Stamp(),a.ColdScopeLocked(),read);}
 std::array<char,cold503::PageBytes> raw{};cold503::Writer w(raw.data(),raw.size());
 if(std::strcmp(result,"OK")){cold503::Envelope(w,result);return ColdText(e,w);}
 struct Release{App& a;cold503::TraceManager::Read& read;bool active=true;~Release(){if(active){std::lock_guard<std::mutex> lock(a.mutex);a.coldTrace.Release(read,a.Stamp(),a.ColdScopeLocked());}}} release{a,read};
 cold503::TracePage(w,read,uint32_t(page));
 {std::lock_guard<std::mutex> lock(a.mutex);result=a.coldTrace.Release(read,a.Stamp(),a.ColdScopeLocked());release.active=false;}
 if(std::strcmp(result,"TRACE_READY")){w.Reset();cold503::Envelope(w,result);}return ColdText(e,w);
}
napi_value executionDiagnosticsV1(napi_env e,napi_callback_info info){
 size_t n=2;napi_value args[2];double remaining=0;cold503::Scope expected;
 if(napi_get_cb_info(e,info,&n,args,nullptr,nullptr)!=napi_ok||n!=2||!ColdScopeArg(e,args[0],expected)||napi_get_value_double(e,args[1],&remaining)!=napi_ok||!std::isfinite(remaining)||remaining<0||std::floor(remaining)!=remaining)return text(e,"{\"state\":\"INVALID_COLD_ARGUMENT\"}");
 const size_t limit=size_t(std::min<double>(cold503::DiagnosticBytes,remaining));auto& a=app();cold503::SmallDiagnostic summary;const char* result;
 {std::lock_guard<std::mutex> lock(a.mutex);const auto actual=a.ColdScopeLocked();result=a.diagnosticGate.Begin(expected,actual,limit);if(!std::strcmp(result,"OK"))summary.Copy(a.diagnostics,actual);}
 std::array<char,cold503::PageBytes> fallback{};
 if(std::strcmp(result,"OK")){cold503::Writer w(fallback.data(),fallback.size());cold503::Envelope(w,result);return ColdText(e,w);}
 cold503::GateLease release{a.diagnosticGate,a.mutex};
 auto copy=std::unique_ptr<diagnostic492::Capture>(new(std::nothrow) diagnostic492::Capture);
 auto buffer=std::unique_ptr<char[]>(new(std::nothrow) char[limit]);
 if(!copy||!buffer){cold503::Writer w(fallback.data(),fallback.size());cold503::DiagnosticFallback(w,summary,"ALLOCATION_FAILED",limit);return ColdText(e,w);}
 resource501::HostScope copyOwnership(a.resources.get(),"BOUNDED_DIAGNOSTIC_COPY",copy.get(),sizeof(*copy),sizeof(*copy));
 resource501::HostScope writerOwnership(a.resources.get(),"BOUNDED_UTF8_WRITER",buffer.get(),0,limit);
 bool stale=false;
 {std::lock_guard<std::mutex> lock(a.mutex);*copy=a.diagnostics;++a.diagnosticGate.copied;summary.Copy(*copy,a.ColdScopeLocked());stale=summary.scope!=expected;}
 cold503::Writer w(buffer.get(),limit);cold503::DiagnosticJson(w,*copy,summary,limit,stale);
 {std::lock_guard<std::mutex> lock(a.mutex);if(summary.scope!=a.ColdScopeLocked())stale=true;}
 if(stale){cold503::DiagnosticFallback(w,summary,"INCOMPLETE_SCOPE_CHANGED",limit,true);}
 return ColdText(e,w);
}
