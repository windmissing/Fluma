#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cstdio>
int main(){EGLDisplay d=eglGetDisplay(EGL_DEFAULT_DISPLAY);if(!eglInitialize(d,0,0))return 1;
eglBindAPI(EGL_OPENGL_ES_API);EGLint attr[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_NONE};EGLConfig c;EGLint n=0;eglChooseConfig(d,attr,&c,1,&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE},sa[]={EGL_WIDTH,1,EGL_HEIGHT,1,EGL_NONE};EGLContext ctx=eglCreateContext(d,c,EGL_NO_CONTEXT,ca);EGLSurface s=eglCreatePbufferSurface(d,c,sa);eglMakeCurrent(d,s,s,ctx);
GLint num=0;glGetIntegerv(GL_NUM_EXTENSIONS,&num);
for(int i=0;i<num;i++){const unsigned char* e=glGetStringi(GL_EXTENSIONS,(GLuint)i);if(e)printf("EXT[%d]=%s\n",i,(const char*)e);}
GLint bindings=-7,blocks=-7,shared=-7;GLint64 maximum=-7;
glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS,&bindings);
glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,&blocks);
glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE,&shared);
glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,&maximum);
GLenum e1=glGetError(),e2=glGetError(),e3=glGetError(),e4=glGetError();
printf("bindings=%d(blocks_q_err=%04x) blocks=%d shared=%d maximum=%lld(err=%04x)\n",bindings,e1,blocks,shared,(long long)maximum,e4);
return 0;}