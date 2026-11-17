#include "gpu_resident/g9spf/schema_generated.h"
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cstdio>
using namespace grs::g9spf;
int main(){
 const auto* p=SelectProfile(192);
 printf("SelectProfile(192)=%s\n",p?"FOUND":"NULL");
 if(!p)return 2;
 EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);if(!eglInitialize(d,0,0)){printf("EGL_INIT_FAIL\n");return 1;}
 eglBindAPI(EGL_OPENGL_ES_API);EGLint attr[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig c;EGLint n=0;eglChooseConfig(d,attr,&c,1,&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},sa[]={EGL_WIDTH,1,EGL_HEIGHT,1,EGL_NONE};EGLContext ctx=eglCreateContext(d,c,EGL_NO_CONTEXT,ca);EGLSurface s=eglCreatePbufferSurface(d,c,sa);
 if(!eglMakeCurrent(d,s,s,ctx)){printf("MAKE_CURRENT_FAIL\n");return 3;}
 printf("pre-query eglErr=%04x glErr=%04x\n",eglGetError(),glGetError());
 GLint bindings=0,blocks=0,shared=0,lanes=0,vertex=0,fragment=0;GLint64 maximum=0;
 glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,&bindings);glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,&blocks);glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,&shared);glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,&lanes);glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,&vertex);glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,&fragment);glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&maximum);
 GLenum qe=glGetError();
 printf("bindings=%d blocks=%d shared=%d lanes=%d vertex=%d fragment=%d max=%lld qErr=%04x\n",bindings,blocks,shared,lanes,vertex,fragment,(long long)maximum,qe);
 uint64_t mx=std::max((uint64_t)p->arenaBytes,std::max((uint64_t)p->stateBytes,(uint64_t)p->workBytes));
 printf("needMax=%llu arena=%llu state=%llu work=%llu\n",(unsigned long long)mx,(unsigned long long)p->arenaBytes,(unsigned long long)p->stateBytes,(unsigned long long)p->workBytes);
 bool pass=bindings>=4&&blocks>=4&&shared>=4096&&lanes>=64&&vertex>=1&&fragment>=1&&maximum>0&&maximum>=(GLint64)mx;
 printf("PREDICATE=%s\n",pass?"PASS":"FAIL");
 return pass?0:4;}
