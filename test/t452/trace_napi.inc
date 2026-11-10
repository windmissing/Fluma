#ifdef GRS_APP_M1
napi_value requestM2Trace(napi_env e,napi_callback_info){
 auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);
 if(a.m2TraceRequested||a.m2TraceSerial==UINT64_MAX)return text(e,"TRACE_BUSY");
 if(!a.active||!a.window)return text(e,"TRACE_SCOPE_UNAVAILABLE");
 a.m2TraceEpoch=a.epoch;a.m2TraceRequested=true;a.m2TraceResult="TRACE_REQUESTED";a.wake.notify_all();
 return text(e,a.m2TraceResult);
}
napi_value readM2Trace(napi_env e,napi_callback_info info){
 size_t n=2;napi_value args[2];uint64_t id=0;uint32_t page=0;double pageNumber=0;
 if(napi_get_cb_info(e,info,&n,args,nullptr,nullptr)!=napi_ok||n!=2||!PageU64(e,args[0],id)||
    napi_get_value_double(e,args[1],&pageNumber)!=napi_ok||!std::isfinite(pageNumber)||pageNumber<0||pageNumber>UINT32_MAX||std::floor(pageNumber)!=pageNumber)return text(e,"{\"state\":\"TRACE_BAD_ARGUMENT\"}");
 page=uint32_t(pageNumber);
 auto& a=app();std::shared_ptr<const App::M2ColdTrace> trace;const char* result;
 {std::lock_guard<std::mutex> lock(a.mutex);trace=a.m2Trace;result=a.m2TraceResult;}
 if(!trace||(!id&&std::strcmp(result,"TRACE_READY")!=0))return text(e,std::string("{\"state\":\"")+result+"\"}");
 if(id&&id!=trace->id)return text(e,"{\"state\":\"STALE_TRACE_ID\"}");
 const auto& c=trace->completion;constexpr size_t perPage=1024;
 const size_t count=256+trace->state.size()+trace->events.size(),pages=(count+perPage-1)/perPage;
 if(page>pages)return text(e,"{\"state\":\"TRACE_BAD_PAGE\"}");
 std::ostringstream out;
 out<<"{\"state\":\"TRACE_READY\",\"revision\":\"T445_M2_TRACE_V3\",\"traceId\":\""<<trace->id<<"\",\"epoch\":\""<<trace->epoch
    <<"\",\"page\":"<<page<<",\"pages\":"<<pages<<",\"rootKind\":"<<c[27]<<",\"stride\":"<<(c[27]==1?128:64)
    <<",\"rootToken\":\""<<m2::RootTokenV3(c)<<"\",\"rows\":"<<(c[27]==1?c[169]:c[53])<<",\"arena\":"<<(c[27]==1?c[172]:c[51])
    <<",\"generationLo\":"<<c[36]<<",\"generationHi\":"<<c[37]<<",\"stateWords\":"<<trace->state.size()<<",\"eventWords\":"<<trace->events.size()<<",\"words\":[";
 if(page!=0){
  const size_t begin=(page-1)*perPage,end=std::min(count,begin+perPage);
  for(size_t i=begin;i<end;++i){if(i!=begin)out<<',';out<<(i<256?c[i]:i-256<trace->state.size()?trace->state[i-256]:trace->events[i-256-trace->state.size()]);}
 }
 out<<"]}";return text(e,out.str());
}
#endif
