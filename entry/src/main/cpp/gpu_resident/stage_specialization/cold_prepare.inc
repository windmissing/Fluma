TransportCode FullM1Transport::PrepareAllStages(const QuantumParameters& p,const ColdPrepareOptions& options,uint64_t start,uint32_t& rawGl){
 constexpr uint64_t limitNs=500000000;
 GLuint privateBuffers[4]{};GLsync complete=nullptr;
 uint64_t peakBytes=0,beforeHash=0,afterHash=0;uint32_t cut=0,issued=0;
 const auto check=[&](){return quantum.Current()&&(!options.current||options.current(options.context))&&TailObserver::Now()-start<limitNs;};
 const auto good=[&](){const auto e=glGetError();if(e&&!rawGl)rawGl=e;return !rawGl&&check();};
 const auto readBase=[&](uint64_t& hash){
  hash=14695981039346656037ull;glBindBuffer(GL_COPY_READ_BUFFER,source.banks[source.owner.Bank()]);
  for(size_t at=0;at<source.words;at+=1024){if(!check())return false;size_t n=std::min<size_t>(1024,source.words-at);
   const auto* mapped=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,GLintptr(at*4),GLsizeiptr(n*4),GL_MAP_READ_BIT));
   if(!mapped){good();return false;}for(size_t i=0;i<n;++i)for(unsigned shift=0;shift<32;shift+=8){hash^=(mapped[i]>>shift)&255u;hash*=1099511628211ull;}
   const bool unmapped=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;if(!unmapped||!good())return false;
  }return true;
 };
 const auto wait=[&](){
  complete=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();if(!complete||!good())return false;
  while(check()){const auto status=glClientWaitSync(complete,0,0);if(status==GL_ALREADY_SIGNALED||status==GL_CONDITION_SATISFIED){glDeleteSync(complete);complete=nullptr;return good();}if(status!=GL_TIMEOUT_EXPIRED)return false;std::this_thread::yield();}
  return false;
 };
 auto run=[&]()->TransportCode{
  cut=1;if(!check()||!readBase(beforeHash))return TransportCode::GL_FAILED;
  // These four cold objects cannot name any live bank/publication/completion.
  // Quantum's candidate/work are already private and reset on the next root.
  const size_t sizes[]={size_t(quantum.eventCapacity)*64,source.workWords,source.pubWords,256};
  glGenBuffers(4,privateBuffers);for(size_t i=0;i<4;++i){if(!check())return TransportCode::GL_FAILED;glBindBuffer(GL_SHADER_STORAGE_BUFFER,privateBuffers[i]);glBufferData(GL_SHADER_STORAGE_BUFFER,GLsizeiptr(sizes[i]*4),nullptr,GL_DYNAMIC_COPY);peakBytes+=sizes[i]*4;if(!good())return TransportCode::GL_FAILED;}
  // Full SOURCE access domain, without owner.Begin/Accept, Run quota or intent.
  cut=2;glBindBuffer(GL_COPY_READ_BUFFER,source.work);glBindBuffer(GL_COPY_WRITE_BUFFER,privateBuffers[1]);glCopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,0,0,GLsizeiptr(source.workWords*4));
  std::array<uint32_t,32> command{};const auto& h=source.owner.Header();command[0]=1129534023u;command[1]=2;command[4]=h[4];command[5]=h[5];command[6]=command[10]=command[14]=command[15]=command[24]=1;command[8]=h[84];command[9]=h[85];command[12]=h[20];command[13]=h[21];command[17]=0x40502000u;command[19]=0x40500000u;command[21]=0x3ff00000u;command[28]=h[22];command[29]=h[23];
  glBufferSubData(GL_COPY_WRITE_BUFFER,0,128,command.data());glUseProgram(source.program);
  const auto scalar=[&](const char* name,uint32_t value){const auto loc=source.uniformLocations.Find(name);if(loc>=0)glUniform1ui(loc,value);};
  const std::pair<const char*,uint32_t> scalars[]={{"uK",source.K},{"uKw",source.Kw},{"uWater",16*source.K},{"uColumn",16*source.K+16*source.Kw},{"uHeader",uint32_t(source.headerAt)},{"uWords",uint32_t(source.words)},{"uEmitQ",source.emitQ},{"uBaseBank",source.owner.Bank()},{"uPublicationSlot",0},{"uEventArena",0},{"uCarry",0},{"uUploaded",T536_SOURCE_UPLOADED}};
  for(auto value:scalars)scalar(value.first,value.second);
  for(const char* name:{"uNonce","uFrame","uControl"}){auto loc=source.uniformLocations.Find(name);if(loc>=0)glUniform2ui(loc,1,0);}
  if(auto loc=source.uniformLocations.Find("uBaseTangent");loc>=0)glUniform1i(loc,int32_t(kAngleWords[4u*uint32_t(int32_t(h[45])+7680)]));
  for(uint32_t pass=0;pass<T536_SOURCE_PASSES;++pass){if(!check())return TransportCode::GL_FAILED;
   const auto output=pass==T536_SOURCE_PUB||pass==T536_SOURCE_PUB+1?privateBuffers[2]:pass==T536_SOURCE_PASSES-1?privateBuffers[3]:privateBuffers[0];
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,source.banks[source.owner.Bank()]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,quantum.candidate);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,2,privateBuffers[1]);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,3,output);scalar("uPass",pass);glDispatchCompute(GLuint((source.words+63)/64),1,1);++issued;glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);if(!good())return TransportCode::GL_FAILED;
  }
  if(!wait())return TransportCode::GL_FAILED;
  glBindBuffer(GL_COPY_READ_BUFFER,privateBuffers[3]);const auto* sourceTail=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,1024,GL_MAP_READ_BIT));
  if(!sourceTail){good();return TransportCode::GL_FAILED;}
  const bool sourceAudited=sourceTail[0]==1381192263u&&sourceTail[2]==256u&&sourceTail[27]==1u&&sourceTail[28]==1u&&sourceTail[29]==0u&&sourceTail[30]==0u;
  const bool sourceUnmapped=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;
  if(!sourceUnmapped||!good())return TransportCode::GL_FAILED;if(!sourceAudited)return TransportCode::PROTOCOL_REJECTED;
  // Full fixed QUANTUM table, including maximum sorts, audit, replay and seal.
  // Stage28's other-bank output is the private SOURCE workspace (large enough),
  // never a live SourceRootOwner bank.
  cut=3;quantum.events=privateBuffers[0];quantum.coldPrefix=true;
  RootRequest fixture;fixture.kind=2;fixture.quantumOrdinal=1;fixture.nonce={1,UINT32_MAX};fixture.frame=fixture.control={1,0};
  auto code=quantum.Enqueue(source.banks[source.owner.Bank()],source.work,96+4096*64+source.words,0,0,p,fixture,coldObserver,true,&rawGl);if(code!=TransportCode::OK)return code;
  quantum.Set("uSpare",spareAt);quantum.Set("uReplayFrozen",replayFrozen);quantum.Set("uBaseBank",source.owner.Bank());quantum.Set("uPublicationSlot",0);quantum.Set("uEventArena",0);quantum.Set("uCarry",0);quantum.Set("uActualDispatches",quantum.PlannedPasses()+8);
  quantum.bindTargets[0]=privateBuffers[1];quantum.bindTargets[1]=privateBuffers[2];quantum.bindTargets[2]=privateBuffers[3];quantum.pubWords=uint32_t(source.pubWords);
  for(uint32_t pass=0;pass<quantum.PlannedPasses();++pass){if(!check())return TransportCode::GL_FAILED;quantum.RunPassRange(pass,pass+1);if(quantum.familyProtocolFault)return TransportCode::PROTOCOL_REJECTED;++issued;if(!good())return TransportCode::GL_FAILED;}
  cut=4;if(!wait())return TransportCode::GL_FAILED;
  glBindBuffer(GL_COPY_READ_BUFFER,quantum.workspace);const auto* state=static_cast<const uint32_t*>(glMapBufferRange(GL_COPY_READ_BUFFER,0,128,GL_MAP_READ_BIT));if(!state){good();return TransportCode::GL_FAILED;}const bool audited=state[0]==0;const bool unmapped=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;
  if(!unmapped||!good())return TransportCode::GL_FAILED;if(!audited)return TransportCode::PROTOCOL_REJECTED;
  cut=5;if(!readBase(afterHash)||beforeHash!=afterHash)return TransportCode::PROTOCOL_REJECTED;
  return check()?TransportCode::OK:TransportCode::GL_FAILED;
 };
 const auto result=run();if(quantum.Current()){if(complete)glDeleteSync(complete);glDeleteBuffers(4,privateBuffers);const auto error=glGetError();if(error&&!rawGl)rawGl=error;}
 quantum.events=eventArenas[0];quantum.coldPrefix=false;quantum.dispatches=quantum.firstGlError=0;quantum.bindTargets[0]=quantum.bindTargets[1]=quantum.bindTargets[2]=0;
 const auto outcome=result==TransportCode::OK&&rawGl?TransportCode::GL_FAILED:result;
 quantum.families.Warmup(start,TailObserver::Now(),rawGl);programcache::Warmup(source.program,start,TailObserver::Now(),rawGl);
 coldObserver.Emit({TailStage::ColdCleanup,0,start,TailObserver::Now(),peakBytes,beforeHash,afterHash,uint32_t(outcome),cut});
 std::fprintf(stderr,"{\"revision\":\"T536_COLD_PREPARE_V1\",\"state\":\"%s\",\"issued\":%u,\"cut\":%u,\"rawGl\":%u,\"code\":%u,\"privateGpuBytes\":%llu,\"uniformBytes\":%llu,\"programSwitches\":%llu}\n",outcome==TransportCode::OK?"COLD_PREPARE_COMPLETE":"COLD_PREPARE_FAILED",issued,cut,rawGl,uint32_t(outcome),(unsigned long long)peakBytes,(unsigned long long)quantum.families.PhysicalUniformBytes(),(unsigned long long)quantum.families.Switches());
 return outcome;
}
