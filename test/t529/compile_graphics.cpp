#include <cstdint>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <string>
#include "../t522/gl_api.inc"
static std::string Read(const std::string& path){std::ifstream in(path,std::ios::binary);assert(in);return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};}
int main(int argc,char** argv){
 if(argc!=3){std::cerr<<"vertex and fragment source paths required\n";return 2;}
 using F=EGLDisplay(EGLAPIENTRYP)(EGLenum,void*,const EGLint*);PROC(F,eglGetPlatformDisplayEXT);
 const EGLint displayArgs[]={0x3203,0x3450,0x3209,0x3487,EGL_NONE};auto display=f(0x3202,nullptr,displayArgs);assert(eglInitialize(display,nullptr,nullptr));
 const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig config;EGLint count;assert(eglChooseConfig(display,attributes,&config,1,&count)&&count);
 const EGLint contextArgs[]={EGL_CONTEXT_CLIENT_VERSION,3,0x30FB,1,EGL_NONE},surfaceArgs[]={EGL_WIDTH,8,EGL_HEIGHT,8,EGL_NONE};auto context=eglCreateContext(display,config,EGL_NO_CONTEXT,contextArgs);auto surface=eglCreatePbufferSurface(display,config,surfaceArgs);assert(eglMakeCurrent(display,surface,surface,context));
 GLuint program=glCreateProgram();
 for(int i=1;i<3;++i){auto source=Read(argv[i]);const char* text=source.c_str();auto shader=glCreateShader(i==1?GL_VERTEX_SHADER:GL_FRAGMENT_SHADER);glShaderSource(shader,1,&text,nullptr);glCompileShader(shader);GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);if(!ok){char log[8192];glGetShaderInfoLog(shader,sizeof(log),nullptr,log);std::cerr<<log;return 1;}glAttachShader(program,shader);glDeleteShader(shader);}
 glLinkProgram(program);GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok){char log[8192];glGetProgramInfoLog(program,sizeof(log),nullptr,log);std::cerr<<log;return 1;}
 glDeleteProgram(program);eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
 std::cout<<"T529 graphics compile/link PASS: "<<argv[1]<<" + "<<argv[2]<<"\n";
}
