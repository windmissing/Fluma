#include <cstdint>
#include "pixel.h"
#include <cassert>
#include <cstring>
#include <iostream>
int reads=0,calls=0;
extern "C" void GL_APIENTRY glGetIntegerv(GLenum n,GLint* v){calls++;*v=n==GL_FRAMEBUFFER_BINDING?7:n==GL_PACK_ALIGNMENT?4:0;}
extern "C" void GL_APIENTRY glReadPixels(GLint,GLint,GLsizei w,GLsizei h,GLenum,GLenum,void* p){reads++;std::memset(p,reads,w*h*4);}
extern "C" GLenum GL_APIENTRY glGetError(){return GL_NO_ERROR;}
int main(){using namespace t276pixel;sample("inactive",1,64,64);assert(reads==0&&calls==0);assert(!arm(0));assert(arm(8));assert(!arm(10));for(uint64_t i=7;i<14;i++)sample("full|same-token",i,64,64);assert(reads==4&&count==4);assert(rows[0].frame==8&&rows[3].frame==11);assert(rows[0].token=="full|same-token");assert(rows[0].rgba[0]==1&&rows[3].rgba[0]==4);assert(exportCold().find("66756c6c7c73616d652d746f6b656e")!=std::string::npos);std::cout<<"{\"passed\":9,\"failed\":0,\"scope\":\"MOCK_GL_BOUND_AND_TOKEN_ONLY\",\"device\":false}\n";}
