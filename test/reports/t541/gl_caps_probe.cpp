#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GLES3/gl3.h>
#ifndef GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS
#define GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS 0x9031
#endif
#ifndef GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS
#define GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS 0x90BE
#endif
#ifndef GL_MAX_COMPUTE_SHARED_MEMORY_SIZE
#define GL_MAX_COMPUTE_SHARED_MEMORY_SIZE 0x90C0
#endif
#ifndef GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
#define GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS 0x90EB
#endif
#ifndef GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS
#define GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS 0x90D1
#endif
#ifndef GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS
#define GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS 0x90D2
#endif
#ifndef GL_MAX_SHADER_STORAGE_BLOCK_SIZE
#define GL_MAX_SHADER_STORAGE_BLOCK_SIZE 0x90DE
#endif
#include <cstdio>
int main(){EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);if(d==EGL_NO_DISPLAY||!eglInitialize(d,0,0)){printf("EGL_INIT_FAIL\n");return 1;}
eglBindAPI(EGL_OPENGL_ES_API);EGLint attr[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig c;EGLint n=0;eglChooseConfig(d,attr,&c,1,&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},sa[]={EGL_WIDTH,1,EGL_HEIGHT,1,EGL_NONE};EGLContext ctx=eglCreateContext(d,c,EGL_NO_CONTEXT,ca);EGLSurface s=eglCreatePbufferSurface(d,c,sa);eglMakeCurrent(d,s,s,ctx);
GLint bindings=0,blocks=0,shared=0,lanes=0,vertex=0,fragment=0;GLint64 maximum=0;
glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,&bindings);glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,&blocks);glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,&shared);glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,&lanes);glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,&vertex);glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,&fragment);glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&maximum);
printf("bindings=%d blocks=%d shared=%d lanes=%d vertexSSBO=%d fragmentSSBO=%d maxSSBOSize=%lld\n",bindings,blocks,shared,lanes,vertex,fragment,(long long)maximum);
printf("need: bindings>=4 blocks>=4 shared>=4096 lanes>=64 maxSSBO>=11718656\n");
printf("renderer=%s\n",(const char*)glGetString(GL_RENDERER));
return 0;}