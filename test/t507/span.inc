TransportCode FullM1Transport::CaptureSpan(uint32_t* out,size_t capacity,size_t& stateWords,size_t& eventWords,GLenum& raw,const char*& stage){
 stateWords=eventWords=0;raw=0;stage="CAPTURE_GUARD";
 if(!frozen||active||submitPhase!=SubmitPhase::IDLE||!source.Current()||!source.ready||source.fence)return TransportCode::BUSY;
 const auto& c=LastCompletion();if(c[1]!=3||(c[27]!=1&&c[27]!=2))return TransportCode::PROTOCOL_REJECTED;
 const uint32_t arena=acceptedArena,rows=acceptedQuantum?tail[53]:tail[169],stride=acceptedQuantum?64u:128u;
 const uint64_t maxRows=acceptedQuantum?uint64_t(quantum.eventCapacity):uint64_t(2*source.K+1);
 if(arena>1||rows>maxRows||source.words>capacity||uint64_t(rows)*stride>capacity-source.words)return TransportCode::PROTOCOL_REJECTED;
 const auto read=[&](GLuint buffer,size_t words,uint32_t* target,const char* mapStage,const char* unmapStage){
  stage=mapStage;if(!words)return true;
  glBindBuffer(GL_COPY_READ_BUFFER,buffer);const auto bytes=GLsizeiptr(words*4);void* data=glMapBufferRange(GL_COPY_READ_BUFFER,0,bytes,GL_MAP_READ_BIT);
  if(!data){raw=glGetError();return false;}std::memcpy(target,data,words*4);
  stage=unmapStage;const bool good=glUnmapBuffer(GL_COPY_READ_BUFFER)==GL_TRUE;const auto error=glGetError();if(error&&!raw)raw=error;return good&&!error;
 };
 stage="STATE_BARRIER";glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);source.stats.barriers++;
 if(!read(source.banks[source.owner.Bank()],source.words,out,"STATE_MAP","STATE_UNMAP"))return TransportCode::GL_FAILED;
 stateWords=source.words;source.stats.coldMappedBytes+=stateWords*4;
 if(!read(acceptedQuantum?eventArenas[arena]:source.source128[arena],size_t(rows)*stride,out+stateWords,"EVENT_MAP","EVENT_UNMAP"))return TransportCode::GL_FAILED;
 eventWords=size_t(rows)*stride;stats.coldMappedBytes+=eventWords*4;stage="CAPTURE_COMPLETE";return TransportCode::OK;
}
