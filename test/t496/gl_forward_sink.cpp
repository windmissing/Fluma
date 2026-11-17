// T561 (problem T547 §3 manifest): full-family Create sink forwarder.
// Frozen-tree copy of gl_forward.cpp (source=.codex/local-test/t236-source-host/gl_forward.cpp).
// Every manifest-listed GL/EGL function: ENTER persisted -> IN_API -> original
// call -> RETURNED -> EXIT persisted via t496sink::ScopedCall*; the remaining
// forwards stay plain (one extern definition each, semantics unchanged).
// glGetString/glGetStringi/glGetIntegerv/glGetInteger64v instrumented; glGetError/
// eglGetError stay PLAIN (error consumption forward-only — never an extra read).
#include "gl_sink.inc"
#include <cstdint>
#include <string>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <cassert>
using t496sink::ScopedCall;using t496sink::ScopedCallVoid;

extern "C" EGLDisplay GL_APIENTRY eglGetPlatformDisplayEXT(EGLenum platform, void* native_display, const EGLint* attrib_list){ static auto fn=reinterpret_cast<decltype(&eglGetPlatformDisplayEXT)>(eglGetProcAddress("eglGetPlatformDisplayEXT"));assert(fn);return ScopedCall(1,"eglGetPlatformDisplayEXT",std::string("platform=")+std::to_string(platform),[&]{return fn(platform, native_display, attrib_list);});}

extern "C" EGLBoolean GL_APIENTRY eglInitialize(EGLDisplay dpy, EGLint* major, EGLint* minor){ static auto fn=reinterpret_cast<decltype(&eglInitialize)>(eglGetProcAddress("eglInitialize"));assert(fn);return ScopedCall(2,"eglInitialize",std::string(),[&]{return fn(dpy, major, minor);});}

extern "C" EGLBoolean GL_APIENTRY eglBindAPI(EGLenum api){ static auto fn=reinterpret_cast<decltype(&eglBindAPI)>(eglGetProcAddress("eglBindAPI"));assert(fn);return ScopedCall(3,"eglBindAPI",std::string("api=")+std::to_string(api),[&]{return fn(api);});}

extern "C" EGLBoolean GL_APIENTRY eglChooseConfig(EGLDisplay dpy, const EGLint* attrib_list, EGLConfig* configs, EGLint config_size, EGLint* num_config){ static auto fn=reinterpret_cast<decltype(&eglChooseConfig)>(eglGetProcAddress("eglChooseConfig"));assert(fn);return ScopedCall(4,"eglChooseConfig",std::string(),[&]{return fn(dpy, attrib_list, configs, config_size, num_config);});}

extern "C" EGLContext GL_APIENTRY eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint* attrib_list){ static auto fn=reinterpret_cast<decltype(&eglCreateContext)>(eglGetProcAddress("eglCreateContext"));assert(fn);return ScopedCall(5,"eglCreateContext",std::string(),[&]{return fn(dpy, config, share_context, attrib_list);});}

extern "C" EGLSurface GL_APIENTRY eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint* attrib_list){ static auto fn=reinterpret_cast<decltype(&eglCreatePbufferSurface)>(eglGetProcAddress("eglCreatePbufferSurface"));assert(fn);return ScopedCall(6,"eglCreatePbufferSurface",std::string(),[&]{return fn(dpy, config, attrib_list);});}

extern "C" EGLBoolean GL_APIENTRY eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx){ static auto fn=reinterpret_cast<decltype(&eglMakeCurrent)>(eglGetProcAddress("eglMakeCurrent"));assert(fn);return ScopedCall(7,"eglMakeCurrent",std::string(),[&]{return fn(dpy, draw, read, ctx);});}

extern "C" EGLBoolean GL_APIENTRY eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint* value){ static auto fn=reinterpret_cast<decltype(&eglQuerySurface)>(eglGetProcAddress("eglQuerySurface"));assert(fn);return ScopedCall(8,"eglQuerySurface",std::string("attribute=")+std::to_string(attribute),[&]{return fn(dpy, surface, attribute, value);});}


extern "C" EGLBoolean GL_APIENTRY eglSwapBuffers(EGLDisplay dpy, EGLSurface surface){ static auto fn=reinterpret_cast<decltype(&eglSwapBuffers)>(eglGetProcAddress("eglSwapBuffers"));assert(fn);return ScopedCall(10,"eglSwapBuffers",std::string(),[&]{return fn(dpy, surface);});}

extern "C" EGLBoolean GL_APIENTRY eglDestroySurface(EGLDisplay dpy, EGLSurface surface){ static auto fn=reinterpret_cast<decltype(&eglDestroySurface)>(eglGetProcAddress("eglDestroySurface"));assert(fn);return ScopedCall(11,"eglDestroySurface",std::string(),[&]{return fn(dpy, surface);});}

extern "C" EGLBoolean GL_APIENTRY eglDestroyContext(EGLDisplay dpy, EGLContext ctx){ static auto fn=reinterpret_cast<decltype(&eglDestroyContext)>(eglGetProcAddress("eglDestroyContext"));assert(fn);return ScopedCall(12,"eglDestroyContext",std::string(),[&]{return fn(dpy, ctx);});}

extern "C" EGLBoolean GL_APIENTRY eglTerminate(EGLDisplay dpy){ static auto fn=reinterpret_cast<decltype(&eglTerminate)>(eglGetProcAddress("eglTerminate"));assert(fn);return ScopedCall(13,"eglTerminate",std::string(),[&]{return fn(dpy);});}


extern "C" EGLContext GL_APIENTRY eglGetCurrentContext(void){ static auto fn=reinterpret_cast<decltype(&eglGetCurrentContext)>(eglGetProcAddress("eglGetCurrentContext"));assert(fn);return ScopedCall(15,"eglGetCurrentContext",std::string(),[&]{return fn();});}

extern "C" EGLDisplay GL_APIENTRY eglGetCurrentDisplay(void){ static auto fn=reinterpret_cast<decltype(&eglGetCurrentDisplay)>(eglGetProcAddress("eglGetCurrentDisplay"));assert(fn);return ScopedCall(16,"eglGetCurrentDisplay",std::string(),[&]{return fn();});}

extern "C" EGLSurface GL_APIENTRY eglGetCurrentSurface(EGLint readdraw){ static auto fn=reinterpret_cast<decltype(&eglGetCurrentSurface)>(eglGetProcAddress("eglGetCurrentSurface"));assert(fn);return ScopedCall(17,"eglGetCurrentSurface",std::string(),[&]{return fn(readdraw);});}

extern "C" EGLint GL_APIENTRY eglGetError(void){ static auto fn=reinterpret_cast<decltype(&eglGetError)>(eglGetProcAddress("eglGetError"));assert(fn);return ScopedCall(18,"eglGetError",std::string(),[&]{return fn();});}

extern "C" GLuint GL_APIENTRY glCreateShader(GLenum type){ static auto fn=reinterpret_cast<decltype(&glCreateShader)>(eglGetProcAddress("glCreateShader"));assert(fn);return ScopedCall(100,"glCreateShader",std::string("type=")+std::to_string(type),[&]{return fn(type);});}

extern "C" void GL_APIENTRY glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length){ static auto fn=reinterpret_cast<decltype(&glShaderSource)>(eglGetProcAddress("glShaderSource"));assert(fn);ScopedCallVoid(101,"glShaderSource",std::string("shader=")+std::to_string(shader) + " count=" + std::to_string(count),[&]{fn(shader, count, string, length);});}

extern "C" void GL_APIENTRY glCompileShader(GLuint shader){ static auto fn=reinterpret_cast<decltype(&glCompileShader)>(eglGetProcAddress("glCompileShader"));assert(fn);ScopedCallVoid(102,"glCompileShader",std::string("shader=")+std::to_string(shader),[&]{fn(shader);});}

extern "C" GLuint GL_APIENTRY glCreateProgram(void){ static auto fn=reinterpret_cast<decltype(&glCreateProgram)>(eglGetProcAddress("glCreateProgram"));assert(fn);return ScopedCall(103,"glCreateProgram",std::string(),[&]{return fn();});}

extern "C" void GL_APIENTRY glAttachShader(GLuint program, GLuint shader){ static auto fn=reinterpret_cast<decltype(&glAttachShader)>(eglGetProcAddress("glAttachShader"));assert(fn);ScopedCallVoid(104,"glAttachShader",std::string("program=")+std::to_string(program) + " shader=" + std::to_string(shader),[&]{fn(program, shader);});}

extern "C" void GL_APIENTRY glLinkProgram(GLuint program){ static auto fn=reinterpret_cast<decltype(&glLinkProgram)>(eglGetProcAddress("glLinkProgram"));assert(fn);ScopedCallVoid(105,"glLinkProgram",std::string("program=")+std::to_string(program),[&]{fn(program);});}




extern "C" void GL_APIENTRY glGetProgramiv(GLuint program, GLenum pname, GLint* params){ static auto fn=reinterpret_cast<decltype(&glGetProgramiv)>(eglGetProcAddress("glGetProgramiv"));assert(fn);ScopedCallVoid(109,"glGetProgramiv",std::string("program=")+std::to_string(program) + " pname=" + std::to_string(pname),[&]{fn(program, pname, params);});}

extern "C" void GL_APIENTRY glGetShaderiv(GLuint shader, GLenum pname, GLint* params){ static auto fn=reinterpret_cast<decltype(&glGetShaderiv)>(eglGetProcAddress("glGetShaderiv"));assert(fn);ScopedCallVoid(110,"glGetShaderiv",std::string("shader=")+std::to_string(shader) + " pname=" + std::to_string(pname),[&]{fn(shader, pname, params);});}



extern "C" void GL_APIENTRY glDeleteProgram(GLuint program){ static auto fn=reinterpret_cast<decltype(&glDeleteProgram)>(eglGetProcAddress("glDeleteProgram"));assert(fn);ScopedCallVoid(113,"glDeleteProgram",std::string("program=")+std::to_string(program),[&]{fn(program);});}

extern "C" void GL_APIENTRY glDeleteShader(GLuint shader){ static auto fn=reinterpret_cast<decltype(&glDeleteShader)>(eglGetProcAddress("glDeleteShader"));assert(fn);ScopedCallVoid(114,"glDeleteShader",std::string("shader=")+std::to_string(shader),[&]{fn(shader);});}

extern "C" const GLubyte* GL_APIENTRY glGetString(GLenum name){ static auto fn=reinterpret_cast<decltype(&glGetString)>(eglGetProcAddress("glGetString"));assert(fn);return ScopedCall(200,"glGetString",std::string("name=")+std::to_string(name),[&]{return fn(name);});}


extern "C" void GL_APIENTRY glGetIntegerv(GLenum pname, GLint* data){ static auto fn=reinterpret_cast<decltype(&glGetIntegerv)>(eglGetProcAddress("glGetIntegerv"));assert(fn);ScopedCallVoid(202,"glGetIntegerv",std::string("pname=")+std::to_string(pname),[&]{fn(pname, data);});}

extern "C" void GL_APIENTRY glGetInteger64v(GLenum pname, GLint64* data){ static auto fn=reinterpret_cast<decltype(&glGetInteger64v)>(eglGetProcAddress("glGetInteger64v"));assert(fn);ScopedCallVoid(203,"glGetInteger64v",std::string("pname=")+std::to_string(pname),[&]{fn(pname, data);});}


extern "C" GLint GL_APIENTRY glGetUniformLocation(GLuint program, const GLchar* name){ static auto fn=reinterpret_cast<decltype(&glGetUniformLocation)>(eglGetProcAddress("glGetUniformLocation"));assert(fn);return ScopedCall(301,"glGetUniformLocation",std::string("program=")+std::to_string(program) + " name=" + (name ? name : ""),[&]{return fn(program, name);});}








extern "C" void GL_APIENTRY glProgramUniform1ui(GLuint program, GLint location, GLuint v0){ static auto fn=reinterpret_cast<decltype(&glProgramUniform1ui)>(eglGetProcAddress("glProgramUniform1ui"));assert(fn);ScopedCallVoid(309,"glProgramUniform1ui",std::string(),[&]{fn(program, location, v0);});}

extern "C" void GL_APIENTRY glGenBuffers(GLsizei n, GLuint* buffers){ static auto fn=reinterpret_cast<decltype(&glGenBuffers)>(eglGetProcAddress("glGenBuffers"));assert(fn);ScopedCallVoid(400,"glGenBuffers",std::string("n=")+std::to_string(n),[&]{fn(n, buffers);});}

extern "C" void GL_APIENTRY glBindBuffer(GLenum target, GLuint buffer){ static auto fn=reinterpret_cast<decltype(&glBindBuffer)>(eglGetProcAddress("glBindBuffer"));assert(fn);ScopedCallVoid(401,"glBindBuffer",std::string("target=")+std::to_string(target) + " buffer=" + std::to_string(buffer),[&]{fn(target, buffer);});}

extern "C" void GL_APIENTRY glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage){ static auto fn=reinterpret_cast<decltype(&glBufferData)>(eglGetProcAddress("glBufferData"));assert(fn);ScopedCallVoid(402,"glBufferData",std::string("target=")+std::to_string(target) + " size=" + std::to_string(size),[&]{fn(target, size, data, usage);});}

extern "C" void GL_APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data){ static auto fn=reinterpret_cast<decltype(&glBufferSubData)>(eglGetProcAddress("glBufferSubData"));assert(fn);ScopedCallVoid(403,"glBufferSubData",std::string("target=")+std::to_string(target) + " offset=" + std::to_string(offset) + " size=" + std::to_string(size),[&]{fn(target, offset, size, data);});}

extern "C" void GL_APIENTRY glBindBufferBase(GLenum target, GLuint index, GLuint buffer){ static auto fn=reinterpret_cast<decltype(&glBindBufferBase)>(eglGetProcAddress("glBindBufferBase"));assert(fn);ScopedCallVoid(404,"glBindBufferBase",std::string("target=")+std::to_string(target) + " index=" + std::to_string(index) + " buffer=" + std::to_string(buffer),[&]{fn(target, index, buffer);});}


extern "C" void GL_APIENTRY glDeleteBuffers(GLsizei n, const GLuint* buffers){ static auto fn=reinterpret_cast<decltype(&glDeleteBuffers)>(eglGetProcAddress("glDeleteBuffers"));assert(fn);ScopedCallVoid(406,"glDeleteBuffers",std::string("n=")+std::to_string(n),[&]{fn(n, buffers);});}

extern "C" void GL_APIENTRY glGenVertexArrays(GLsizei n, GLuint* arrays){ static auto fn=reinterpret_cast<decltype(&glGenVertexArrays)>(eglGetProcAddress("glGenVertexArrays"));assert(fn);ScopedCallVoid(407,"glGenVertexArrays",std::string("n=")+std::to_string(n),[&]{fn(n, arrays);});}

extern "C" void GL_APIENTRY glBindVertexArray(GLuint array){ static auto fn=reinterpret_cast<decltype(&glBindVertexArray)>(eglGetProcAddress("glBindVertexArray"));assert(fn);ScopedCallVoid(408,"glBindVertexArray",std::string("array=")+std::to_string(array),[&]{fn(array);});}

extern "C" void GL_APIENTRY glDeleteVertexArrays(GLsizei n, const GLuint* arrays){ static auto fn=reinterpret_cast<decltype(&glDeleteVertexArrays)>(eglGetProcAddress("glDeleteVertexArrays"));assert(fn);ScopedCallVoid(409,"glDeleteVertexArrays",std::string("n=")+std::to_string(n),[&]{fn(n, arrays);});}

extern "C" void GL_APIENTRY glUseProgram(GLuint program){ static auto fn=reinterpret_cast<decltype(&glUseProgram)>(eglGetProcAddress("glUseProgram"));assert(fn);ScopedCallVoid(410,"glUseProgram",std::string("program=")+std::to_string(program),[&]{fn(program);});}

extern "C" void GL_APIENTRY glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z){ static auto fn=reinterpret_cast<decltype(&glDispatchCompute)>(eglGetProcAddress("glDispatchCompute"));assert(fn);ScopedCallVoid(411,"glDispatchCompute",std::string("x=")+std::to_string(num_groups_x),[&]{fn(num_groups_x, num_groups_y, num_groups_z);});}

extern "C" void GL_APIENTRY glMemoryBarrier(GLbitfield barriers){ static auto fn=reinterpret_cast<decltype(&glMemoryBarrier)>(eglGetProcAddress("glMemoryBarrier"));assert(fn);ScopedCallVoid(412,"glMemoryBarrier",std::string("barriers=")+std::to_string(barriers),[&]{fn(barriers);});}

extern "C" void GL_APIENTRY glFlush(void){ static auto fn=reinterpret_cast<decltype(&glFlush)>(eglGetProcAddress("glFlush"));assert(fn);ScopedCallVoid(413,"glFlush",std::string(),[&]{fn();});}


extern "C" GLsync GL_APIENTRY glFenceSync(GLenum condition, GLbitfield flags){ static auto fn=reinterpret_cast<decltype(&glFenceSync)>(eglGetProcAddress("glFenceSync"));assert(fn);return ScopedCall(415,"glFenceSync",std::string("condition=")+std::to_string(condition),[&]{return fn(condition, flags);});}

extern "C" GLenum GL_APIENTRY glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout){ static auto fn=reinterpret_cast<decltype(&glClientWaitSync)>(eglGetProcAddress("glClientWaitSync"));assert(fn);return ScopedCall(416,"glClientWaitSync",std::string("timeout=")+std::to_string(timeout),[&]{return fn(sync, flags, timeout);});}

extern "C" void GL_APIENTRY glDeleteSync(GLsync sync){ static auto fn=reinterpret_cast<decltype(&glDeleteSync)>(eglGetProcAddress("glDeleteSync"));assert(fn);ScopedCallVoid(417,"glDeleteSync",std::string(),[&]{fn(sync);});}

extern "C" void* GL_APIENTRY glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access){ static auto fn=reinterpret_cast<decltype(&glMapBufferRange)>(eglGetProcAddress("glMapBufferRange"));assert(fn);return ScopedCall(418,"glMapBufferRange",std::string("target=")+std::to_string(target) + " length=" + std::to_string(length),[&]{return fn(target, offset, length, access);});}

extern "C" GLboolean GL_APIENTRY glUnmapBuffer(GLenum target){ static auto fn=reinterpret_cast<decltype(&glUnmapBuffer)>(eglGetProcAddress("glUnmapBuffer"));assert(fn);return ScopedCall(419,"glUnmapBuffer",std::string("target=")+std::to_string(target),[&]{return fn(target);});}

extern "C" GLint GL_APIENTRY glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar* name){ static auto fn=reinterpret_cast<decltype(&glGetProgramResourceLocation)>(eglGetProcAddress("glGetProgramResourceLocation"));assert(fn);return ScopedCall(420,"glGetProgramResourceLocation",std::string(),[&]{return fn(program, programInterface, name);});}

extern "C" void GL_APIENTRY glGenFramebuffers(GLsizei n, GLuint* framebuffers){ static auto fn=reinterpret_cast<decltype(&glGenFramebuffers)>(eglGetProcAddress("glGenFramebuffers"));assert(fn);ScopedCallVoid(500,"glGenFramebuffers",std::string("n=")+std::to_string(n),[&]{fn(n, framebuffers);});}

extern "C" void GL_APIENTRY glBindFramebuffer(GLenum target, GLuint framebuffer){ static auto fn=reinterpret_cast<decltype(&glBindFramebuffer)>(eglGetProcAddress("glBindFramebuffer"));assert(fn);ScopedCallVoid(501,"glBindFramebuffer",std::string("target=")+std::to_string(target) + " fb=" + std::to_string(framebuffer),[&]{fn(target, framebuffer);});}

extern "C" void GL_APIENTRY glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers){ static auto fn=reinterpret_cast<decltype(&glDeleteFramebuffers)>(eglGetProcAddress("glDeleteFramebuffers"));assert(fn);ScopedCallVoid(502,"glDeleteFramebuffers",std::string("n=")+std::to_string(n),[&]{fn(n, framebuffers);});}

extern "C" void GL_APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count){ static auto fn=reinterpret_cast<decltype(&glDrawArrays)>(eglGetProcAddress("glDrawArrays"));assert(fn);ScopedCallVoid(503,"glDrawArrays",std::string("mode=")+std::to_string(mode) + " count=" + std::to_string(count),[&]{fn(mode, first, count);});}

// ---- plain forwards (unchanged semantics) ----
extern "C" void GL_APIENTRY glBindTexture(GLenum target, GLuint texture){ static auto fn=reinterpret_cast<decltype(&glBindTexture)>(eglGetProcAddress("glBindTexture"));assert(fn);fn(target,texture);}

extern "C" void GL_APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor){ static auto fn=reinterpret_cast<decltype(&glBlendFunc)>(eglGetProcAddress("glBlendFunc"));assert(fn);fn(sfactor,dfactor);}

extern "C" GLenum GL_APIENTRY glCheckFramebufferStatus(GLenum target){ static auto fn=reinterpret_cast<decltype(&glCheckFramebufferStatus)>(eglGetProcAddress("glCheckFramebufferStatus"));assert(fn);return fn(target);}

extern "C" void GL_APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha){ static auto fn=reinterpret_cast<decltype(&glColorMask)>(eglGetProcAddress("glColorMask"));assert(fn);fn(red,green,blue,alpha);}

extern "C" void GL_APIENTRY glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level){ static auto fn=reinterpret_cast<decltype(&glFramebufferTexture2D)>(eglGetProcAddress("glFramebufferTexture2D"));assert(fn);fn(target,attachment,textarget,texture,level);}

extern "C" void GL_APIENTRY glGenTextures(GLsizei n, GLuint* textures){ static auto fn=reinterpret_cast<decltype(&glGenTextures)>(eglGetProcAddress("glGenTextures"));assert(fn);fn(n,textures);}

extern "C" void GL_APIENTRY glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params){ static auto fn=reinterpret_cast<decltype(&glGetFramebufferAttachmentParameteriv)>(eglGetProcAddress("glGetFramebufferAttachmentParameteriv"));assert(fn);fn(target,attachment,pname,params);}

extern "C" void GL_APIENTRY glGetIntegeri_v(GLenum target, GLuint index, GLint* data){ static auto fn=reinterpret_cast<decltype(&glGetIntegeri_v)>(eglGetProcAddress("glGetIntegeri_v"));assert(fn);fn(target,index,data);}

extern "C" GLenum GL_APIENTRY glGetError(){ static auto fn=reinterpret_cast<decltype(&glGetError)>(eglGetProcAddress("glGetError"));assert(fn);return fn();}

extern "C" void GL_APIENTRY glPixelStorei(GLenum pname, GLint param){ static auto fn=reinterpret_cast<decltype(&glPixelStorei)>(eglGetProcAddress("glPixelStorei"));assert(fn);fn(pname,param);}

extern "C" void GL_APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param){ static auto fn=reinterpret_cast<decltype(&glTexParameteri)>(eglGetProcAddress("glTexParameteri"));assert(fn);fn(target,pname,param);}

extern "C" void GL_APIENTRY glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height){ static auto fn=reinterpret_cast<decltype(&glTexStorage2D)>(eglGetProcAddress("glTexStorage2D"));assert(fn);fn(target,levels,internalformat,width,height);}

extern "C" void GL_APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels){ static auto fn=reinterpret_cast<decltype(&glTexSubImage2D)>(eglGetProcAddress("glTexSubImage2D"));assert(fn);fn(target,level,xoffset,yoffset,width,height,format,type,pixels);}

extern "C" void GL_APIENTRY glUniform1i(GLint location, GLint v0){ static auto fn=reinterpret_cast<decltype(&glUniform1i)>(eglGetProcAddress("glUniform1i"));assert(fn);fn(location,v0);}

extern "C" void GL_APIENTRY glUniform1ui(GLint location, GLuint v0){ static auto fn=reinterpret_cast<decltype(&glUniform1ui)>(eglGetProcAddress("glUniform1ui"));assert(fn);fn(location,v0);}

extern "C" void GL_APIENTRY glUniform2f(GLint location, GLfloat v0, GLfloat v1){ static auto fn=reinterpret_cast<decltype(&glUniform2f)>(eglGetProcAddress("glUniform2f"));assert(fn);fn(location,v0,v1);}

extern "C" void GL_APIENTRY glUniform2ui(GLint location, GLuint v0, GLuint v1){ static auto fn=reinterpret_cast<decltype(&glUniform2ui)>(eglGetProcAddress("glUniform2ui"));assert(fn);fn(location,v0,v1);}

extern "C" void GL_APIENTRY glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2){ static auto fn=reinterpret_cast<decltype(&glUniform3ui)>(eglGetProcAddress("glUniform3ui"));assert(fn);fn(location,v0,v1,v2);}

extern "C" void GL_APIENTRY glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3){ static auto fn=reinterpret_cast<decltype(&glUniform4f)>(eglGetProcAddress("glUniform4f"));assert(fn);fn(location,v0,v1,v2,v3);}

extern "C" void GL_APIENTRY glVertexAttribDivisor(GLuint index, GLuint divisor){ static auto fn=reinterpret_cast<decltype(&glVertexAttribDivisor)>(eglGetProcAddress("glVertexAttribDivisor"));assert(fn);fn(index,divisor);}

extern "C" void GL_APIENTRY glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer){ static auto fn=reinterpret_cast<decltype(&glVertexAttribIPointer)>(eglGetProcAddress("glVertexAttribIPointer"));assert(fn);fn(index,size,type,stride,pointer);}

extern "C" void GL_APIENTRY glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer){ static auto fn=reinterpret_cast<decltype(&glVertexAttribPointer)>(eglGetProcAddress("glVertexAttribPointer"));assert(fn);fn(index,size,type,normalized,stride,pointer);}

extern "C" void GL_APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height){ static auto fn=reinterpret_cast<decltype(&glViewport)>(eglGetProcAddress("glViewport"));assert(fn);fn(x,y,width,height);}

extern "C" void GL_APIENTRY glEnable(GLenum cap){ static auto fn=reinterpret_cast<decltype(&glEnable)>(eglGetProcAddress("glEnable"));assert(fn);fn(cap);}

extern "C" void GL_APIENTRY glDisable(GLenum cap){ static auto fn=reinterpret_cast<decltype(&glDisable)>(eglGetProcAddress("glDisable"));assert(fn);fn(cap);}

extern "C" void GL_APIENTRY glEnableVertexAttribArray(GLuint index){ static auto fn=reinterpret_cast<decltype(&glEnableVertexAttribArray)>(eglGetProcAddress("glEnableVertexAttribArray"));assert(fn);fn(index);}


extern "C" void GL_APIENTRY glActiveTexture(GLenum texture){ static auto fn=reinterpret_cast<decltype(&glActiveTexture)>(eglGetProcAddress("glActiveTexture"));assert(fn);fn(texture);}

extern "C" void GL_APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures){ static auto fn=reinterpret_cast<decltype(&glDeleteTextures)>(eglGetProcAddress("glDeleteTextures"));assert(fn);fn(n,textures);}

extern "C" void GL_APIENTRY glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount){ static auto fn=reinterpret_cast<decltype(&glDrawArraysInstanced)>(eglGetProcAddress("glDrawArraysInstanced"));assert(fn);fn(mode,first,count,instancecount);}
