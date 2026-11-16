#define PROC(type, name)                                                       \
  auto f = reinterpret_cast<type>(eglGetProcAddress(#name));                   \
  assert(f)
extern "C" void GL_APIENTRY glCopyBufferSubData(GLenum readTarget,GLenum writeTarget,GLintptr readOffset,GLintptr writeOffset,GLsizeiptr size){
  PROC(PFNGLCOPYBUFFERSUBDATAPROC,glCopyBufferSubData);f(readTarget,writeTarget,readOffset,writeOffset,size);
}
extern "C" void GL_APIENTRY glFinish(){PROC(PFNGLFINISHPROC,glFinish);f();}
extern "C" void GL_APIENTRY glGetActiveUniform(GLuint program,GLuint index,GLsizei capacity,GLsizei* length,GLint* size,GLenum* type,GLchar* name){PROC(PFNGLGETACTIVEUNIFORMPROC,glGetActiveUniform);f(program,index,capacity,length,size,type,name);}
extern "C" void GL_APIENTRY glGetBufferParameteri64v(GLenum target,GLenum name,GLint64* result){PROC(PFNGLGETBUFFERPARAMETERI64VPROC,glGetBufferParameteri64v);f(target,name,result);}
extern "C" GLboolean GL_APIENTRY glIsBuffer(GLuint buffer){PROC(PFNGLISBUFFERPROC,glIsBuffer);return f(buffer);}
extern "C" void GL_APIENTRY glProgramParameteri(GLuint p, GLenum k, GLint v) {
  PROC(PFNGLPROGRAMPARAMETERIPROC, glProgramParameteri);
  f(p, k, v);
}
extern "C" void GL_APIENTRY glProgramBinary(GLuint p, GLenum format,
                                            const void *bytes, GLsizei n) {
  PROC(PFNGLPROGRAMBINARYPROC, glProgramBinary);
  f(p, format, bytes, n);
}
extern "C" void GL_APIENTRY glGetProgramBinary(GLuint p, GLsizei n,
                                               GLsizei *len, GLenum *format,
                                               void *bytes) {
  PROC(PFNGLGETPROGRAMBINARYPROC, glGetProgramBinary);
  f(p, n, len, format, bytes);
}
extern "C" const GLubyte *GL_APIENTRY glGetStringi(GLenum p, GLuint i) {
  PROC(PFNGLGETSTRINGIPROC, glGetStringi);
  return f(p, i);
}
extern "C" void GL_APIENTRY glGetShaderInfoLog(GLuint p, GLsizei n,
                                               GLsizei *len, GLchar *bytes) {
  PROC(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
  f(p, n, len, bytes);
}
extern "C" void GL_APIENTRY glGetProgramInfoLog(GLuint p, GLsizei n,
                                                GLsizei *len, GLchar *bytes) {
  PROC(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
  f(p, n, len, bytes);
}
extern "C" EGLBoolean EGLAPIENTRY eglQueryContext(EGLDisplay d, EGLContext c,
                                                  EGLint k, EGLint *v) {
  using F = EGLBoolean(EGLAPIENTRYP)(EGLDisplay, EGLContext, EGLint, EGLint *);
  PROC(F, eglQueryContext);
  return f(d, c, k, v);
}
