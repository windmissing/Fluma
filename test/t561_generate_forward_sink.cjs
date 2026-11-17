'use strict';
// T561: regenerate test/t496/gl_forward_sink.cpp with full-family sink
// instrumentation (problem T547 §3 manifest apiGroups). Every listed function
// gets ENTER persisted -> IN_API -> original call -> RETURNED -> EXIT persisted
// via t496sink::ScopedCall/ScopedCallVoid; unlisted functions keep the plain
// forward (one extern definition, unchanged semantics).
const fs = require('fs');

// [cppName, fid, returnType|'', paramDecls, argList, enterDetailExpr]
const FNS = [
  // EGL/host mapping (18)
  ['eglGetPlatformDisplayEXT', 1, 'EGLDisplay', 'EGLenum platform, void* native_display, const EGLint* attrib_list', 'platform, native_display, attrib_list', 'platform=' + '+std::to_string(platform)'],
  ['eglInitialize', 2, 'EGLBoolean', 'EGLDisplay dpy, EGLint* major, EGLint* minor', 'dpy, major, minor', ''],
  ['eglBindAPI', 3, 'EGLBoolean', 'EGLenum api', 'api', 'api=' + '+std::to_string(api)'],
  ['eglChooseConfig', 4, 'EGLBoolean', 'EGLDisplay dpy, const EGLint* attrib_list, EGLConfig* configs, EGLint config_size, EGLint* num_config', 'dpy, attrib_list, configs, config_size, num_config', ''],
  ['eglCreateContext', 5, 'EGLContext', 'EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint* attrib_list', 'dpy, config, share_context, attrib_list', ''],
  ['eglCreatePbufferSurface', 6, 'EGLSurface', 'EGLDisplay dpy, EGLConfig config, const EGLint* attrib_list', 'dpy, config, attrib_list', ''],
  ['eglMakeCurrent', 7, 'EGLBoolean', 'EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx', 'dpy, draw, read, ctx', ''],
  ['eglQuerySurface', 8, 'EGLBoolean', 'EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint* value', 'dpy, surface, attribute, value', 'attribute=' + '+std::to_string(attribute)'],
  ['eglQueryContext', 9, 'EGLBoolean', 'EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint* value', 'dpy, ctx, attribute, value', 'attribute=' + '+std::to_string(attribute)'],
  ['eglSwapBuffers', 10, 'EGLBoolean', 'EGLDisplay dpy, EGLSurface surface', 'dpy, surface', ''],
  ['eglDestroySurface', 11, 'EGLBoolean', 'EGLDisplay dpy, EGLSurface surface', 'dpy, surface', ''],
  ['eglDestroyContext', 12, 'EGLBoolean', 'EGLDisplay dpy, EGLContext ctx', 'dpy, ctx', ''],
  ['eglTerminate', 13, 'EGLBoolean', 'EGLDisplay dpy', 'dpy', ''],
  ['eglGetProcAddress', 14, 'void*', 'const char* procname', 'procname', 'std::string("proc=") + (procname ? procname : "")'],
  ['eglGetCurrentContext', 15, 'EGLContext', '', '', ''],
  ['eglGetCurrentDisplay', 16, 'EGLDisplay', '', '', ''],
  ['eglGetCurrentSurface', 17, 'EGLSurface', 'EGLint readdraw', 'readdraw', ''],
  ['eglGetError', 18, 'EGLint', '', '', ''],
  // compile/binary (15)
  ['glCreateShader', 100, 'GLuint', 'GLenum type', 'type', 'type=' + '+std::to_string(type)'],
  ['glShaderSource', 101, '', 'GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length', 'shader, count, string, length', 'shader=' + '+std::to_string(shader) + " count=" + std::to_string(count)'],
  ['glCompileShader', 102, '', 'GLuint shader', 'shader', 'shader=' + '+std::to_string(shader)'],
  ['glCreateProgram', 103, 'GLuint', '', '', ''],
  ['glAttachShader', 104, '', 'GLuint program, GLuint shader', 'program, shader', 'program=' + '+std::to_string(program) + " shader=" + std::to_string(shader)'],
  ['glLinkProgram', 105, '', 'GLuint program', 'program', 'program=' + '+std::to_string(program)'],
  ['glProgramParameteri', 106, '', 'GLuint program, GLenum pname, GLint value', 'program, pname, value', 'program=' + '+std::to_string(program) + " pname=" + std::to_string(pname)'],
  ['glProgramBinary', 107, '', 'GLuint program, GLenum binaryFormat, const void* binary, GLsizei length', 'program, binaryFormat, binary, length', 'program=' + '+std::to_string(program) + " format=" + std::to_string(binaryFormat) + " bytes=" + std::to_string(length)'],
  ['glGetProgramBinary', 108, '', 'GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, void* binary', 'program, bufSize, length, binaryFormat, binary', 'program=' + '+std::to_string(program) + " bufSize=" + std::to_string(bufSize)'],
  ['glGetProgramiv', 109, '', 'GLuint program, GLenum pname, GLint* params', 'program, pname, params', 'program=' + '+std::to_string(program) + " pname=" + std::to_string(pname)'],
  ['glGetShaderiv', 110, '', 'GLuint shader, GLenum pname, GLint* params', 'shader, pname, params', 'shader=' + '+std::to_string(shader) + " pname=" + std::to_string(pname)'],
  ['glGetShaderInfoLog', 111, '', 'GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog', 'shader, bufSize, length, infoLog', 'shader=' + '+std::to_string(shader)'],
  ['glGetProgramInfoLog', 112, '', 'GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog', 'program, bufSize, length, infoLog', 'program=' + '+std::to_string(program)'],
  ['glDeleteProgram', 113, '', 'GLuint program', 'program', 'program=' + '+std::to_string(program)'],
  ['glDeleteShader', 114, '', 'GLuint shader', 'shader', 'shader=' + '+std::to_string(shader)'],
  // environment (4; error forward-only stays plain to not double-read)
  ['glGetString', 200, 'const GLubyte*', 'GLenum name', 'name', 'name=' + '+std::to_string(name)'],
  ['glGetStringi', 201, 'const GLubyte*', 'GLenum name, GLuint index', 'name, index', 'name=' + '+std::to_string(name) + " index=" + std::to_string(index)'],
  ['glGetIntegerv', 202, '', 'GLenum pname, GLint* data', 'pname, data', 'pname=' + '+std::to_string(pname)'],
  ['glGetInteger64v', 203, '', 'GLenum pname, GLint64* data', 'pname, data', 'pname=' + '+std::to_string(pname)'],
  // reflection (10)
  ['glGetActiveUniform', 300, '', 'GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name', 'program, index, bufSize, length, size, type, name', 'program=' + '+std::to_string(program) + " index=" + std::to_string(index)'],
  ['glGetUniformLocation', 301, 'GLint', 'GLuint program, const GLchar* name', 'program, name', 'program=' + '+std::to_string(program) + " name=" + (name ? name : "")'],
  ['glGetProgramInterfaceiv', 302, '', 'GLuint program, GLenum programInterface, GLenum pname, GLint* params', 'program, programInterface, pname, params', 'program=' + '+std::to_string(program) + " iface=" + std::to_string(programInterface)'],
  ['glGetProgramResourceIndex', 303, 'GLuint', 'GLuint program, GLenum programInterface, const GLchar* name', 'program, programInterface, name', 'program=' + '+std::to_string(program) + " iface=" + std::to_string(programInterface) + " name=" + (name ? name : "")'],
  ['glGetProgramResourceiv', 304, '', 'GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum* props, GLsizei bufSize, GLsizei* length, GLint* params', 'program, programInterface, index, propCount, props, bufSize, length, params', 'program=' + '+std::to_string(program) + " index=" + std::to_string(index)'],
  ['glGetActiveAttrib', 305, '', 'GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name', 'program, index, bufSize, length, size, type, name', 'program=' + '+std::to_string(program) + " index=" + std::to_string(index)'],
  ['glGetAttribLocation', 306, 'GLint', 'GLuint program, const GLchar* name', 'program, name', 'program=' + '+std::to_string(program) + " name=" + (name ? name : "")'],
  ['glGetFragDataLocation', 307, 'GLint', 'GLuint program, const GLchar* name', 'program, name', 'program=' + '+std::to_string(program) + " name=" + (name ? name : "")'],
  ['glIsProgram', 308, 'GLboolean', 'GLuint program', 'program', 'program=' + '+std::to_string(program)'],
  ['glProgramUniform1ui', 309, '', 'GLuint program, GLint location, GLuint v0', 'program, location, v0', ''],
  // resource/prepare (21)
  ['glGenBuffers', 400, '', 'GLsizei n, GLuint* buffers', 'n, buffers', 'n=' + '+std::to_string(n)'],
  ['glBindBuffer', 401, '', 'GLenum target, GLuint buffer', 'target, buffer', 'target=' + '+std::to_string(target) + " buffer=" + std::to_string(buffer)'],
  ['glBufferData', 402, '', 'GLenum target, GLsizeiptr size, const void* data, GLenum usage', 'target, size, data, usage', 'target=' + '+std::to_string(target) + " size=" + std::to_string(size)'],
  ['glBufferSubData', 403, '', 'GLenum target, GLintptr offset, GLsizeiptr size, const void* data', 'target, offset, size, data', 'target=' + '+std::to_string(target) + " offset=" + std::to_string(offset) + " size=" + std::to_string(size)'],
  ['glBindBufferBase', 404, '', 'GLenum target, GLuint index, GLuint buffer', 'target, index, buffer', 'target=' + '+std::to_string(target) + " index=" + std::to_string(index) + " buffer=" + std::to_string(buffer)'],
  ['glCopyBufferSubData', 405, '', 'GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size', 'readTarget, writeTarget, readOffset, writeOffset, size', 'size=' + '+std::to_string(size)'],
  ['glDeleteBuffers', 406, '', 'GLsizei n, const GLuint* buffers', 'n, buffers', 'n=' + '+std::to_string(n)'],
  ['glGenVertexArrays', 407, '', 'GLsizei n, GLuint* arrays', 'n, arrays', 'n=' + '+std::to_string(n)'],
  ['glBindVertexArray', 408, '', 'GLuint array', 'array', 'array=' + '+std::to_string(array)'],
  ['glDeleteVertexArrays', 409, '', 'GLsizei n, const GLuint* arrays', 'n, arrays', 'n=' + '+std::to_string(n)'],
  ['glUseProgram', 410, '', 'GLuint program', 'program', 'program=' + '+std::to_string(program)'],
  ['glDispatchCompute', 411, '', 'GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z', 'num_groups_x, num_groups_y, num_groups_z', 'x=' + '+std::to_string(num_groups_x)'],
  ['glMemoryBarrier', 412, '', 'GLbitfield barriers', 'barriers', 'barriers=' + '+std::to_string(barriers)'],
  ['glFlush', 413, '', '', '', ''],
  ['glFinish', 414, '', '', '', ''],
  ['glFenceSync', 415, 'GLsync', 'GLenum condition, GLbitfield flags', 'condition, flags', 'condition=' + '+std::to_string(condition)'],
  ['glClientWaitSync', 416, 'GLenum', 'GLsync sync, GLbitfield flags, GLuint64 timeout', 'sync, flags, timeout', 'timeout=' + '+std::to_string(timeout)'],
  ['glDeleteSync', 417, '', 'GLsync sync', 'sync', ''],
  ['glMapBufferRange', 418, 'void*', 'GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access', 'target, offset, length, access', 'target=' + '+std::to_string(target) + " length=" + std::to_string(length)'],
  ['glUnmapBuffer', 419, 'GLboolean', 'GLenum target', 'target', 'target=' + '+std::to_string(target)'],
  ['glGetProgramResourceLocation', 420, 'GLint', 'GLuint program, GLenum programInterface, const GLchar* name', 'program, programInterface, name', ''],
  // scene/FBO (4 — only those this harness TU can define; the rest stay plain)
  ['glGenFramebuffers', 500, '', 'GLsizei n, GLuint* framebuffers', 'n, framebuffers', 'n=' + '+std::to_string(n)'],
  ['glBindFramebuffer', 501, '', 'GLenum target, GLuint framebuffer', 'target, framebuffer', 'target=' + '+std::to_string(target) + " fb=" + std::to_string(framebuffer)'],
  ['glDeleteFramebuffers', 502, '', 'GLsizei n, const GLuint* framebuffers', 'n, framebuffers', 'n=' + '+std::to_string(n)'],
  ['glDrawArrays', 503, '', 'GLenum mode, GLint first, GLsizei count', 'mode, first, count', 'mode=' + '+std::to_string(mode) + " count=" + std::to_string(count)'],
];

const plain = [ // keep original plain forwarding (not in manifest sink list)
  'glBindTexture', 'glBlendFunc', 'glCheckFramebufferStatus', 'glClientWaitSyncPlain', 'glColorMask',
  'glFramebufferTexture2D', 'glGenTextures', 'glGetFramebufferAttachmentParameteriv',
  'glGetIntegeri_v', 'glGetError', 'glPixelStorei', 'glTexParameteri', 'glTexStorage2D',
  'glTexSubImage2D', 'glUniform1i', 'glUniform1ui', 'glUniform2f', 'glUniform2ui', 'glUniform3ui',
  'glUniform4f', 'glVertexAttribDivisor', 'glVertexAttribIPointer', 'glVertexAttribPointer',
  'glViewport', 'glEnable', 'glDisable', 'glEnableVertexAttribArray',
];

let out = [];
out.push('// T561 (problem T547 §3 manifest): full-family Create sink forwarder.');
out.push('// Frozen-tree copy of gl_forward.cpp (source=.codex/local-test/t236-source-host/gl_forward.cpp).');
out.push('// Every manifest-listed GL/EGL function: ENTER persisted -> IN_API -> original');
out.push('// call -> RETURNED -> EXIT persisted via t496sink::ScopedCall*; the remaining');
out.push('// forwards stay plain (one extern definition each, semantics unchanged).');
out.push('// glGetString/glGetStringi/glGetIntegerv/glGetInteger64v instrumented; glGetError/');
out.push('// eglGetError stay PLAIN (error consumption forward-only — never an extra read).');
out.push('#include "gl_sink.inc"');
out.push('#include <cstdint>');
out.push('#include <string>');
out.push('#include <EGL/egl.h>');
out.push('#include <GLES3/gl31.h>');
out.push('#include <cassert>');
out.push('using t496sink::ScopedCall;using t496sink::ScopedCallVoid;');
out.push('');

for (const [name, fid, ret, params, args, detail] of FNS) {
  const plist = params ? (' ' + params) : '(void)';
  const call = `reinterpret_cast<decltype(&${name})>(eglGetProcAddress("${name}"))`;
  if (ret === '') {
    out.push(`extern "C" void GL_APIENTRY ${name}(${params || 'void'}){ static auto fn=${call};assert(fn);ScopedCallVoid(${fid},"${name}",${detail ? `std::string(${detail})` : 'std::string()'},[&]{fn(${args});});}`);
  } else {
    out.push(`extern "C" ${ret} GL_APIENTRY ${name}(${params || 'void'}){ static auto fn=${call};assert(fn);return ScopedCall(${fid},"${name}",${detail ? `std::string(${detail})` : 'std::string()'},[&]()->{return fn(${args});});}`);
  }
  out.push('');
}
out.push('// ---- plain forwards (unchanged semantics) ----');
const plainMap = {
  'glBindTexture': ['void', 'GLenum target, GLuint texture', 'target,texture'],
  'glBlendFunc': ['void', 'GLenum sfactor, GLenum dfactor', 'sfactor,dfactor'],
  'glCheckFramebufferStatus': ['GLenum', 'GLenum target', 'target'],
  'glColorMask': ['void', 'GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha', 'red,green,blue,alpha'],
  'glFramebufferTexture2D': ['void', 'GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level', 'target,attachment,textarget,texture,level'],
  'glGenTextures': ['void', 'GLsizei n, GLuint* textures', 'n,textures'],
  'glGetFramebufferAttachmentParameteriv': ['void', 'GLenum target, GLenum attachment, GLenum pname, GLint* params', 'target,attachment,pname,params'],
  'glGetIntegeri_v': ['void', 'GLenum target, GLuint index, GLint* data', 'target,index,data'],
  'glGetError': ['GLenum', '', ''],
  'glPixelStorei': ['void', 'GLenum pname, GLint param', 'pname,param'],
  'glTexParameteri': ['void', 'GLenum target, GLenum pname, GLint param', 'target,pname,param'],
  'glTexStorage2D': ['void', 'GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height', 'target,levels,internalformat,width,height'],
  'glTexSubImage2D': ['void', 'GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels', 'target,level,xoffset,yoffset,width,height,format,type,pixels'],
  'glUniform1i': ['void', 'GLint location, GLint v0', 'location,v0'],
  'glUniform1ui': ['void', 'GLint location, GLuint v0', 'location,v0'],
  'glUniform2f': ['void', 'GLint location, GLfloat v0, GLfloat v1', 'location,v0,v1'],
  'glUniform2ui': ['void', 'GLint location, GLuint v0, GLuint v1', 'location,v0,v1'],
  'glUniform3ui': ['void', 'GLint location, GLuint v0, GLuint v1, GLuint v2', 'location,v0,v1,v2'],
  'glUniform4f': ['void', 'GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3', 'location,v0,v1,v2,v3'],
  'glVertexAttribDivisor': ['void', 'GLuint index, GLuint divisor', 'index,divisor'],
  'glVertexAttribIPointer': ['void', 'GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer', 'index,size,type,stride,pointer'],
  'glVertexAttribPointer': ['void', 'GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer', 'index,size,type,normalized,stride,pointer'],
  'glViewport': ['void', 'GLint x, GLint y, GLsizei width, GLsizei height', 'x,y,width,height'],
  'glEnable': ['void', 'GLenum cap', 'cap'],
  'glDisable': ['void', 'GLenum cap', 'cap'],
  'glEnableVertexAttribArray': ['void', 'GLuint index', 'index'],
};
for (const [name, [ret, params, args]] of Object.entries(plainMap)) {
  const call = `reinterpret_cast<decltype(&${name})>(eglGetProcAddress("${name}"))`;
  if (ret === 'void') out.push(`extern "C" void GL_APIENTRY ${name}(${params}){ static auto fn=${call};assert(fn);fn(${args});}`);
  else out.push(`extern "C" ${ret} GL_APIENTRY ${name}(${params}){ static auto fn=${call};assert(fn);return fn(${args});}`);
  out.push('');
}
fs.writeFileSync('test/t496/gl_forward_sink.cpp', out.join('\n') + '\n');
console.log('written', FNS.length, 'sink functions +', Object.keys(plainMap).length, 'plain forwards');
