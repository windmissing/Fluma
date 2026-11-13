// Snapshot page serialization and JS string creation are outside App::mutex.
napi_value newRunAction(napi_env e,napi_callback_info info){size_t n=1;napi_value v;uint64_t id=0;if(napi_get_cb_info(e,info,&n,&v,nullptr,nullptr)!=napi_ok||n!=1||!PageU64(e,v,id))return text(e,"INVALID_ACTION_SERIAL");app().invalidate(nullptr,false,"UI_RESET",0,0,id);return text(e,"NEW_RUN_REQUESTED");}
napi_value timelineSnapshotBegin(napi_env e,napi_callback_info){auto& a=app();TimelineSnapshot::Page p;const char* result;{std::lock_guard<std::mutex> lock(a.mutex);result=a.SnapshotBeginLocked(p);}return text(e,std::strcmp(result,"OK")==0?p.Json():result);}
napi_value timelineSnapshotPage(napi_env e,napi_callback_info info){
 size_t n=2;napi_value args[2];uint64_t id=0;double cursor=0;
 if(napi_get_cb_info(e,info,&n,args,nullptr,nullptr)!=napi_ok||n!=2||!PageU64(e,args[0],id)||napi_get_value_double(e,args[1],&cursor)!=napi_ok||!std::isfinite(cursor)||cursor<0||cursor>4096||std::floor(cursor)!=cursor)return text(e,"INVALID_CURSOR");
 auto& a=app();TimelineSnapshot::Page p;const char* result;{std::lock_guard<std::mutex> lock(a.mutex);result=a.stableTimeline->Read(id,uint32_t(cursor),a.Stamp(),p);}return text(e,std::strcmp(result,"OK")==0?p.Json():result);
}
napi_value timelineSnapshotEnd(napi_env e,napi_callback_info info){size_t n=1;napi_value v;uint64_t id=0;if(napi_get_cb_info(e,info,&n,&v,nullptr,nullptr)!=napi_ok||n!=1||!PageU64(e,v,id))return text(e,"STALE_SNAPSHOT");auto& a=app();const char* result;{std::lock_guard<std::mutex> lock(a.mutex);result=a.stableTimeline->End(id,a.Stamp());}return text(e,result);}
napi_value executionDiagnostics(napi_env e,napi_callback_info){auto copy=std::make_unique<diagnostic492::Capture>();auto& a=app();{std::lock_guard<std::mutex> lock(a.mutex);*copy=a.diagnostics;}std::ostringstream out;copy->Json(out);return text(e,out.str());}
napi_value recordDiagnosticAction(napi_env e,napi_callback_info info){
 size_t n=1;napi_value v;size_t bytes=0;std::array<char,1025> raw{};
 if(napi_get_cb_info(e,info,&n,&v,nullptr,nullptr)!=napi_ok||n!=1||napi_get_value_string_utf8(e,v,nullptr,0,&bytes)!=napi_ok||!bytes||bytes>1024||napi_get_value_string_utf8(e,v,raw.data(),raw.size(),&bytes)!=napi_ok)return text(e,"INVALID_ACTION");
 std::string value(raw.data(),bytes);auto& a=app();{std::lock_guard<std::mutex> lock(a.mutex);a.DiagnosticActionLocked(value);}return text(e,"ACTION_RECORDED");
}
