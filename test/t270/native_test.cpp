#include <cstdint>
#include "native_diag.h"
#include <cassert>
#include <cstring>
#include <iostream>
static int calls=0,reads=0;
extern "C" void GL_APIENTRY glGetIntegerv(GLenum name,GLint* value){++calls;*value=name==GL_FRAMEBUFFER_BINDING?7:name==GL_PACK_ALIGNMENT?4:0;}
extern "C" void GL_APIENTRY glReadPixels(GLint,GLint,GLsizei w,GLsizei h,GLenum,GLenum,void* p){++reads;std::memset(p,reads,w*h*4);}
extern "C" GLenum GL_APIENTRY glGetError(){return GL_NO_ERROR;}
int main(){p013::ClearWaterPacket p;p.frame.frameId=5;p.frame.surfaceWidthPx=p.frame.surfaceHeightPx=64;p.token="token|\"identity\"";cwdiag::sample(2,p,7);assert(calls==0&&reads==0);assert(!cwdiag::arm(5,4,0,0,4,4));assert(cwdiag::arm(5,3,0,0,4,4));for(int frame=5;frame<=8;frame++){p.frame.frameId=frame;for(int layer=0;layer<6;layer++)cwdiag::sample(layer,p,7);}assert(reads==9&&cwdiag::count==9);assert(cwdiag::samples[0].rgba[0]==1&&cwdiag::samples[8].rgba[0]==9);assert(cwdiag::samples[0].fbo==7);auto out=cwdiag::exportCold();assert(out.find("token|\\\"identity\\\"")!=std::string::npos);assert(out.find("SRGB8_ALPHA8")!=std::string::npos);assert(cwdiag::arm(20,1,0,0,4,4));p.frame.frameId=20;cwdiag::sample(3,p,8);assert(reads==9&&cwdiag::samples[0].raw==0xffffffffu);std::cout<<"{\"passed\":9,\"failed\":0,\"scope\":\"MOCK_GL_ROI_GATING_NO_STATE_MUTATION_API\",\"device\":false}\n";}
