#ifdef GRS_APP_M1
 struct M2ColdTrace {
  uint64_t id=0,epoch=0;
  Words256 completion{};
  std::vector<uint32_t> state,events;
 };
 std::shared_ptr<const M2ColdTrace> m2Trace;
 bool m2TraceRequested=false;
 uint64_t m2TraceSerial=0,m2TraceEpoch=0;
 const char* m2TraceResult="NO_TRACE";
 void ServiceM2Trace(AppTransport& gpu,uint64_t seen,void* held){
  uint64_t serial=0;
  {std::lock_guard<std::mutex> lock(mutex);
   if(!m2TraceRequested)return;
   if(m2TraceEpoch!=seen||!CurrentLocked(seen,held)){
    m2TraceRequested=false;m2TraceResult="STALE_TRACE_SCOPE";return;
   }
   if(!gpu.CaptureRootIdle())return;
   m2TraceRequested=false;serial=++m2TraceSerial;
  }
  auto trace=std::make_shared<M2ColdTrace>();trace->id=serial;trace->epoch=seen;
  auto code=gpu.Freeze(true);
  if(code==TransportCode::OK){
   trace->completion=gpu.LastCompletion();code=gpu.Capture(trace->state,trace->events);
   if(gpu.Freeze(false)!=TransportCode::OK)code=TransportCode::GL_FAILED;
  }
  std::lock_guard<std::mutex> lock(mutex);
  if(!CurrentLocked(seen,held)){m2TraceResult="STALE_TRACE_SCOPE";return;}
  const auto& c=trace->completion;
  const uint32_t stride=c[27]==1?128:64,rows=c[27]==1?c[169]:c[53];
  if(code!=TransportCode::OK||c[1]!=3||(c[27]!=1&&c[27]!=2)||trace->events.size()!=size_t(rows)*stride){
   m2TraceResult="TRACE_UNAVAILABLE";return;
  }
  m2Trace=std::move(trace);m2TraceResult="TRACE_READY";
 }
#endif
