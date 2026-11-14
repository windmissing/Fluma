#pragma once
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include "resource_ledger.h"
namespace grs {namespace resource501 {
// Allocation-only observation. Preserve the original GL error flags until the
// application's next glGetError; never query errors on dispatch/bind/draw paths.
inline thread_local std::array<GLenum,16> errors{};
inline thread_local size_t errorCount=0;
inline thread_local EGLContext errorContext=EGL_NO_CONTEXT;
inline uint64_t Context(){return reinterpret_cast<uintptr_t>(eglGetCurrentContext());}
inline void Remember(GLenum error){if(error==GL_NO_ERROR)return;for(size_t i=0;i<errorCount;++i)if(errors[i]==error)return;if(errorCount<errors.size())errors[errorCount++]=error;}
inline GLenum Drain(){if(errorContext!=eglGetCurrentContext()){errorCount=0;errorContext=eglGetCurrentContext();}GLenum first=0;for(unsigned i=0;i<16;++i){const auto error=::glGetError();if(!error)break;if(!first)first=error;Remember(error);}return first;}
inline GLenum Error(){if(!current)return ::glGetError();if(errorContext!=eglGetCurrentContext()){errorCount=0;errorContext=eglGetCurrentContext();}if(errorCount){const auto error=errors[0];for(size_t i=1;i<errorCount;++i)errors[i-1]=errors[i];--errorCount;return error;}return ::glGetError();}
inline void Bind(Ledger* ledger,uint64_t scope){current=ledger;currentScope=scope;errorCount=0;errorContext=eglGetCurrentContext();if(ledger)ledger->Scope(Context(),scope,"CURRENT");}
inline GLenum BufferBinding(GLenum target){switch(target){case GL_ARRAY_BUFFER:return GL_ARRAY_BUFFER_BINDING;case GL_ELEMENT_ARRAY_BUFFER:return GL_ELEMENT_ARRAY_BUFFER_BINDING;case GL_SHADER_STORAGE_BUFFER:return GL_SHADER_STORAGE_BUFFER_BINDING;case GL_COPY_READ_BUFFER:return GL_COPY_READ_BUFFER_BINDING;case GL_COPY_WRITE_BUFFER:return GL_COPY_WRITE_BUFFER_BINDING;case GL_UNIFORM_BUFFER:return GL_UNIFORM_BUFFER_BINDING;case GL_PIXEL_PACK_BUFFER:return GL_PIXEL_PACK_BUFFER_BINDING;case GL_PIXEL_UNPACK_BUFFER:return GL_PIXEL_UNPACK_BUFFER_BINDING;default:return 0;}}
inline void BufferData(GLenum target,GLsizeiptr size,const void* data,GLenum usage){if(!current){::glBufferData(target,size,data,usage);return;}Drain();GLint name=0;const auto query=BufferBinding(target);if(query)::glGetIntegerv(query,&name);Drain();::glBufferData(target,size,data,usage);const auto error=Drain();current->Allocate(Context(),currentScope,Kind::Buffer,uint32_t(name),size<0?0:uint64_t(size),error);}
inline void TexStorage2D(GLenum target,GLsizei levels,GLenum format,GLsizei w,GLsizei h){if(!current){::glTexStorage2D(target,levels,format,w,h);return;}Drain();GLint name=0;if(target==GL_TEXTURE_2D)::glGetIntegerv(GL_TEXTURE_BINDING_2D,&name);Drain();::glTexStorage2D(target,levels,format,w,h);const auto error=Drain();uint64_t bytes=0;if(w>0&&h>0&&levels>0&&(format==GL_SRGB8_ALPHA8||format==GL_RGBA8||format==GL_R32F))for(GLsizei l=0;l<levels&&l<32;++l)bytes+=4ull*uint64_t(std::max(1,w>>l))*uint64_t(std::max(1,h>>l));current->Allocate(Context(),currentScope,Kind::Texture,uint32_t(name),bytes,error);}
#define T501_NAMES(Suffix,KindValue) \
 inline void Gen##Suffix(GLsizei n,GLuint* names){if(!current){::glGen##Suffix(n,names);return;}Drain();::glGen##Suffix(n,names);auto error=Drain();for(GLsizei i=0;i<n;++i)current->Create(Context(),currentScope,Kind::KindValue,names[i],error);} \
 inline void Delete##Suffix(GLsizei n,const GLuint* names){if(!current){::glDelete##Suffix(n,names);return;}Drain();::glDelete##Suffix(n,names);auto error=Drain();for(GLsizei i=0;i<n;++i)current->Delete(Context(),currentScope,Kind::KindValue,names[i],error);}
T501_NAMES(Buffers,Buffer)
T501_NAMES(Textures,Texture)
T501_NAMES(VertexArrays,VertexArray)
T501_NAMES(Framebuffers,Framebuffer)
#undef T501_NAMES
inline GLuint CreateProgram(){if(!current)return ::glCreateProgram();Drain();const auto name=::glCreateProgram();const auto error=Drain();current->Create(Context(),currentScope,Kind::Program,name,error);return name;}
inline GLuint CreateShader(GLenum type){if(!current)return ::glCreateShader(type);Drain();const auto name=::glCreateShader(type);const auto error=Drain();current->Create(Context(),currentScope,Kind::Shader,name,error);return name;}
inline void DeleteProgram(GLuint name){if(!current){::glDeleteProgram(name);return;}Drain();::glDeleteProgram(name);auto error=Drain();current->Delete(Context(),currentScope,Kind::Program,name,error);}
inline void DeleteShader(GLuint name){if(!current){::glDeleteShader(name);return;}Drain();::glDeleteShader(name);auto error=Drain();current->Delete(Context(),currentScope,Kind::Shader,name,error);}
inline GLsync FenceSync(GLenum condition,GLbitfield flags){if(!current)return ::glFenceSync(condition,flags);Drain();auto name=::glFenceSync(condition,flags);auto error=Drain();current->Create(Context(),currentScope,Kind::Sync,reinterpret_cast<uintptr_t>(name),error);return name;}
inline void DeleteSync(GLsync name){if(!current){::glDeleteSync(name);return;}Drain();::glDeleteSync(name);auto error=Drain();current->Delete(Context(),currentScope,Kind::Sync,reinterpret_cast<uintptr_t>(name),error);}
}}
#define glGetError grs::resource501::Error
#define glBufferData grs::resource501::BufferData
#define glTexStorage2D grs::resource501::TexStorage2D
#define glGenBuffers grs::resource501::GenBuffers
#define glDeleteBuffers grs::resource501::DeleteBuffers
#define glGenTextures grs::resource501::GenTextures
#define glDeleteTextures grs::resource501::DeleteTextures
#define glGenVertexArrays grs::resource501::GenVertexArrays
#define glDeleteVertexArrays grs::resource501::DeleteVertexArrays
#define glGenFramebuffers grs::resource501::GenFramebuffers
#define glDeleteFramebuffers grs::resource501::DeleteFramebuffers
#define glCreateProgram grs::resource501::CreateProgram
#define glDeleteProgram grs::resource501::DeleteProgram
#define glCreateShader grs::resource501::CreateShader
#define glDeleteShader grs::resource501::DeleteShader
#define glFenceSync grs::resource501::FenceSync
#define glDeleteSync grs::resource501::DeleteSync
