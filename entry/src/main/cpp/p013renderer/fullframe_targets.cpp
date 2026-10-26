#include "fullframe_targets.h"
#include <algorithm>
namespace p013 {
Status FullFrameTargets::Ensure(int w,int h,uint64_t epoch){
 if(context==epoch&&width==w&&height==h&&textures[0]&&textures[1])return Status::OK;
 Reset(context==epoch);GLint limit=0;glGetIntegerv(GL_MAX_TEXTURE_SIZE,&limit);if(w<=0||h<=0||w>limit||h>limit||size_t(w)>SIZE_MAX/size_t(h)/8)return Status::GL_RESOURCE_FAILED;
 context=epoch;width=w;height=h;glGenTextures(2,textures);glGenFramebuffers(2,fbos);
 for(int i=0;i<2;i++){
  glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,textures[i]);glTexStorage2D(GL_TEXTURE_2D,1,GL_SRGB8_ALPHA8,w,h);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glBindFramebuffer(GL_FRAMEBUFFER,fbos[i]);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,textures[i],0);
  GLint encoding=0;glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,&encoding);
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE||encoding!=GL_SRGB||glGetError()!=GL_NO_ERROR){Reset(true);return Status::COLORSPACE_UNSUPPORTED;}
 }
 allocations+=2;return Status::OK;
}
void FullFrameTargets::Reset(bool current){if(current){glDeleteTextures(2,textures);glDeleteFramebuffers(2,fbos);}textures[0]=textures[1]=fbos[0]=fbos[1]=0;context=0;width=height=0;candidate=0;lastGood=1;}
}
