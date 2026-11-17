#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cstdio>
int main(){EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);if(!eglInitialize(d,0,0))return 1;
eglBindAPI(EGL_OPENGL_ES_API);EGLint attr[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig c;EGLint n=0;eglChooseConfig(d,attr,&c,1,&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},sa[]={EGL_WIDTH,1,EGL_HEIGHT,1,EGL_NONE};EGLContext ctx=eglCreateContext(d,c,EGL_NO_CONTEXT,ca);EGLSurface s=eglCreatePbufferSurface(d,c,sa);
printf("pre-makeCurrent: eglErr=%04x\n",eglGetError());
if(!eglMakeCurrent(d,s,s,ctx))printf("MAKE_CURRENT_FAIL egl=%04x\n",eglGetError());
printf("post-makeCurrent: gl=%04x egl=%04x\n",glGetError(),eglGetError());
GLint x=-1;glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,&x);
printf("shared=%d gl=%04x\n",x,glGetError());
return 0;}