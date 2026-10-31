// T336 build supplement: two GL forwarding stubs the partial_host link unit
// needs (scene.cpp's uOptics/uRadius/uMassReference uniforms). Same pattern
// as t324/gl_extra.cpp and t236-source-host/gl_forward.cpp — forward via
// eglGetProcAddress to the real ANGLE entry point. Kept separate so the
// shared t324/gl_extra.cpp is untouched for other suites.
#define EGL_NO_PLATFORM_SPECIFIC_TYPES 1
#define KHRONOS_STATIC 1
#include <stdint.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cassert>
extern "C" void GL_APIENTRY glUniform1f(GLint location, GLfloat v0){auto fn=reinterpret_cast<PFNGLUNIFORM1FPROC>(eglGetProcAddress("glUniform1f"));assert(fn);fn(location,v0);}
extern "C" void GL_APIENTRY glUniform4fv(GLint location, GLsizei count, const GLfloat* value){auto fn=reinterpret_cast<PFNGLUNIFORM4FVPROC>(eglGetProcAddress("glUniform4fv"));assert(fn);fn(location,count,value);}
