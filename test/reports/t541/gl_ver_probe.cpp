#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cstdio>
int main(){EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);if(!eglInitialize(d,0,0)){printf("EGL_INIT_FAIL\n");return 1;}
eglBindAPI(EGL_OPENGL_ES_API);EGLint attr[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig c;EGLint n=0;eglChooseConfig(d,attr,&c,1,&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},sa[]={EGL_WIDTH,1,EGL_HEIGHT,1,EGL_NONE};EGLContext ctx=eglCreateContext(d,c,EGL_NO_CONTEXT,ca);EGLSurface s=eglCreatePbufferSurface(d,c,sa);eglMakeCurrent(d,s,s,ctx);
printf("VERSION=%s\n",(const char*)glGetString(GL_VERSION));
GLint maj=0,minr=0;glGetIntegerv(GL_MAJOR_VERSION,&maj);glGetIntegerv(GL_MINOR_VERSION,&minr);
printf("MAJOR=%d MINOR=%d\n",maj,minr);
GLint numext=0;glGetIntegerv(GL_NUM_EXTENSIONS,&numext);
printf("NUM_EXTENSIONS=%d\n",numext);
int ssbo=0;for(int i=0;i<numext;i++){const char* e=(const char*)glGetStringi(GL_EXTENSIONS,i);if(e&&(__builtin_strstr(e,"shader_storage")||__builtin_strstr(e,"compute")))printf("EXT[%d]=%s\n",i,e);}
return 0;}