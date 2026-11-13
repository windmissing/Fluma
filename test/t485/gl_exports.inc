#define FORWARD_VOID(name,type,args,values) extern "C" void GL_APIENTRY name args {auto procedure=reinterpret_cast<type>(eglGetProcAddress(#name));assert(procedure);procedure values;}
FORWARD_VOID(glFinish,PFNGLFINISHPROC,(),())
FORWARD_VOID(glCopyBufferSubData,PFNGLCOPYBUFFERSUBDATAPROC,(GLenum a,GLenum b,GLintptr c,GLintptr d,GLsizeiptr n),(a,b,c,d,n))
FORWARD_VOID(glReadPixels,PFNGLREADPIXELSPROC,(GLint x,GLint y,GLsizei w,GLsizei h,GLenum f,GLenum t,void* p),(x,y,w,h,f,t,p))
FORWARD_VOID(glUniform1f,PFNGLUNIFORM1FPROC,(GLint p,GLfloat v),(p,v))
FORWARD_VOID(glUniform4fv,PFNGLUNIFORM4FVPROC,(GLint p,GLsizei n,const GLfloat* v),(p,n,v))
FORWARD_VOID(glGetActiveAttrib,PFNGLGETACTIVEATTRIBPROC,(GLuint p,GLuint i,GLsizei n,GLsizei* l,GLint* s,GLenum* t,GLchar* v),(p,i,n,l,s,t,v))
FORWARD_VOID(glGetProgramInterfaceiv,PFNGLGETPROGRAMINTERFACEIVPROC,(GLuint p,GLenum i,GLenum q,GLint* v),(p,i,q,v))
FORWARD_VOID(glGetProgramResourceiv,PFNGLGETPROGRAMRESOURCEIVPROC,(GLuint p,GLenum i,GLuint x,GLsizei n,const GLenum* q,GLsizei b,GLsizei* l,GLint* v),(p,i,x,n,q,b,l,v))
extern "C" GLint GL_APIENTRY glGetAttribLocation(GLuint p,const GLchar* n){auto f=reinterpret_cast<PFNGLGETATTRIBLOCATIONPROC>(eglGetProcAddress("glGetAttribLocation"));assert(f);return f(p,n);}
extern "C" GLint GL_APIENTRY glGetFragDataLocation(GLuint p,const GLchar* n){auto f=reinterpret_cast<PFNGLGETFRAGDATALOCATIONPROC>(eglGetProcAddress("glGetFragDataLocation"));assert(f);return f(p,n);}
extern "C" GLuint GL_APIENTRY glGetProgramResourceIndex(GLuint p,GLenum i,const GLchar* n){auto f=reinterpret_cast<PFNGLGETPROGRAMRESOURCEINDEXPROC>(eglGetProcAddress("glGetProgramResourceIndex"));assert(f);return f(p,i,n);}
extern "C" GLboolean GL_APIENTRY glIsProgram(GLuint p){auto f=reinterpret_cast<PFNGLISPROGRAMPROC>(eglGetProcAddress("glIsProgram"));assert(f);return f(p);}
