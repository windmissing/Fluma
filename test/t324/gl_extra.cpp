// T324 host build: same forwarding stub as t317/gl_extra.cpp with the
// platform macros and integer typedefs defined first.
#define EGL_NO_PLATFORM_SPECIFIC_TYPES 1
#define KHRONOS_STATIC 1
#include <stdint.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cassert>
extern "C" void GL_APIENTRY glGetActiveUniform(GLuint p,GLuint i,GLsizei n,GLsizei* l,GLint* size,GLenum* type,GLchar* name){auto f=reinterpret_cast<PFNGLGETACTIVEUNIFORMPROC>(eglGetProcAddress("glGetActiveUniform"));assert(f);f(p,i,n,l,size,type,name);}
