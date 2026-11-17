#include "cold_contract.h"
#include <cstring>
namespace grs::g9dm {
bool Transport::CaptureSpan(uint32_t* out,size_t words,bool full,const uint32_t* quota,uint64_t request,uint64_t trace,uint64_t created){
 coldGl_=coldEgl_=0;auto coldGood=[&](){const auto g=glGetError();const auto e=eglGetError();if(g&&!coldGl_)coldGl_=g;if(e!=EGL_SUCCESS&&!coldEgl_)coldEgl_=e;return !coldGl_&&!coldEgl_;};
 cold::Layout layout;if(!ready_||busy_||!Current()||!profile_||!out||!quota||!cold::Layout::Make(*profile_,4,layout,full)||words<layout.words||full&&!config_.fullTrialTrace||tail_[0]!=kCompletionMagic||tail_[6]==5)return false;
 // Prefix has no descriptor for itself. Its hash belongs to the external
 // page archive manifest, avoiding any recursive digest input.
 std::memset(out,0,layout.words*4);out[0]=0x31433947;out[1]=1;out[2]=8192;out[4]=kProfileCode;out[5]=full?1:0;out[6]=uint32_t(layout.count);out[7]=profile_->k;Put(out,8,layout.words*4);Put(out,10,request);Put(out,12,trace);Put(out,14,created);Put(out,16,permit_.identity.run);Put(out,18,permit_.identity.context);Put(out,20,permit_.identity.surface);Put(out,22,permit_.identity.pageSession);Put(out,24,permit_.identity.inputEpoch);Put(out,26,permit_.nonce);out[28]=tail_[6];out[29]=tail_[37];out[30]=tail_[44];out[31]=tail_[46];
 std::copy(tail_.begin(),tail_.end(),out+256);std::copy(lastInput_.begin(),lastInput_.end(),out+512);std::copy(lastConfig_.begin(),lastConfig_.end(),out+640);std::copy(quota,quota+256,out+768);
 auto copy=[&](GLuint buffer,size_t offset,size_t count,size_t destination){
  if(!count)return true;if(destination>layout.words||count>layout.words-destination||offset>SIZE_MAX/4||count>SIZE_MAX/4)return false;
  glBindBuffer(GL_COPY_READ_BUFFER,buffer);auto* mapped=glMapBufferRange(GL_COPY_READ_BUFFER,GLintptr(offset*4),GLsizeiptr(count*4),GL_MAP_READ_BIT);if(!mapped){coldGood();return false;}
  std::memcpy(out+destination,mapped,count*4);const auto unmapped=glUnmapBuffer(GL_COPY_READ_BUFFER);const bool good=coldGood();return unmapped==GL_TRUE&&good;
 };
 glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);
 if(!coldGood()||!copy(buffers_[5],profile_->work[6].begin/4,384,1024))return false;
 for(size_t i=0;i<layout.count;++i){const auto& segment=layout.segments[i];GLuint buffer=i==0?buffers_[1]:i==1?buffers_[current_]:i==2?buffers_[7]:i==3?buffers_[8+permit_.arena]:full_[i-4];
  const uint32_t* summary=i>=4?out+1024+(i-4)*64:nullptr;bool executed=i<4||summary[0]!=0;
  if(executed&&!copy(buffer,0,segment.words,segment.begin))return false;
  auto* descriptor=out+2048+i*32;descriptor[0]=uint32_t(i+1);descriptor[1]=i>=4?uint32_t(i-4):0;descriptor[2]=i==2?1:i==3?2:i>=4?1:0;descriptor[3]=executed?2:1;descriptor[4]=i>=4?summary[0]:tail_[37];descriptor[5]=i>=4?summary[7]:tail_[44];descriptor[6]=i>=4?summary[8]:tail_[46];Put(descriptor,8,segment.begin);Put(descriptor,10,segment.words);descriptor[12]=1;
  cold::SegmentDigest(out+segment.begin,segment.words,descriptor+23);descriptor[31]=kSeal;
 }
 out[3]=kSeal;return cold::Validate(out,words,layout);
}
}
