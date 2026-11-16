 void ServiceColdTrace(AppTransport& gpu,uint64_t seen,void* held){
  int slot=-1;std::unique_ptr<uint32_t[]> old;size_t words=0;
  {
   std::lock_guard<std::mutex> lock(mutex);ColdMaintainLocked();
   if(!CurrentLocked(seen,held)||!gpu.CaptureRootIdle())return;
   slot=coldTrace.Start();if(slot<0)return;
   words=coldTrace.slots[slot].reserved;old=coldTrace.TakeOld(slot);
  }
  if(words){
   // Freed FREE-slot backing is removed before reserving the new allocation.
   old.reset();{std::lock_guard<std::mutex> lock(mutex);coldTrace.OldFreed(slot);}
   auto storage=std::unique_ptr<uint32_t[]>(new(std::nothrow) uint32_t[words]);
   {std::lock_guard<std::mutex> lock(mutex);coldTrace.Adopt(slot,std::move(storage));}
  }
  uint32_t* block=nullptr;size_t capacity=0;
  {std::lock_guard<std::mutex> lock(mutex);ColdMaintainLocked();auto& s=coldTrace.slots[slot];block=s.block.get();capacity=s.capacity;if(s.cancel){coldTrace.Finish(slot,ColdScopeLocked(),Stamp(),"CANCELLED",0,0);resources->HostStorage("COLD_TRACE_SLOT",reinterpret_cast<uintptr_t>(&s),0,s.capacity*4,s.capacity!=0);return;}}
  size_t stateWords=0,eventWords=0;GLenum gl=0;uint32_t transportCode=0;const char* stage="ALLOCATE";const char* result="TRACE_ALLOCATION_FAILED";
  if(block){
   stage="FREEZE";auto code=gpu.Freeze(true);
   if(code==TransportCode::OK){
    const auto& completion=gpu.LastCompletion();std::copy(completion.begin(),completion.end(),block);
    stage="CAPTURE_SPAN";code=gpu.CaptureSpan(block+256,capacity-256,stateWords,eventWords,gl,stage);
    const auto restore=gpu.Freeze(false);
    if(restore!=TransportCode::OK&&code==TransportCode::OK){stage="RESTORE";code=restore;}
    // The restore call is always attempted before a cancelled builder retires.
   }
   transportCode=uint32_t(code);result=code==TransportCode::OK?"OK":code==TransportCode::PROTOCOL_REJECTED?"TRACE_FORMAT_REJECTED":"TRACE_CAPTURE_FAILED";
  }
  {
   std::lock_guard<std::mutex> lock(mutex);coldTrace.Finish(slot,ColdScopeLocked(),Stamp(),result,stateWords,eventWords,gl,0,stage,transportCode);
   auto& s=coldTrace.slots[slot];resources->HostStorage("COLD_TRACE_SLOT",reinterpret_cast<uintptr_t>(&s),s.stateWords*4+s.eventWords*4+1024,s.capacity*4,s.capacity!=0);
  }
 }
