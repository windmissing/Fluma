#include "scene.h"
#include "grs_scene_sources.h"
#include <sstream>
#include <iomanip>
#include <cmath>
namespace grs {
namespace {
GLuint Shader(GLenum kind,const char* text){GLuint s=glCreateShader(kind);glShaderSource(s,1,&text,nullptr);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){glDeleteShader(s);return 0;}return s;}
GLuint Program(const char* vs,const char* fs){GLuint v=Shader(GL_VERTEX_SHADER,vs),f=Shader(GL_FRAGMENT_SHADER,fs);if(!v||!f){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return 0;}GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glDeleteShader(v);glDeleteShader(f);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){glDeleteProgram(p);return 0;}return p;}
bool equal(U64 a,U64 b){return a.lo==b.lo&&a.hi==b.hi;}uint64_t value(U64 x){return (uint64_t(x.hi)<<32)|x.lo;}
}
void ResidentScene::Reset(bool current){targets.Reset(current);if(current){if(full)glDeleteProgram(full);if(particles)glDeleteProgram(particles);if(vao)glDeleteVertexArrays(1,&vao);if(particleVao)glDeleteVertexArrays(1,&particleVao);if(bed)glDeleteTextures(1,&bed);}full=particles=vao=particleVao=bed=0;context=EGL_NO_CONTEXT;lastGood.clear();lastRenderFrame=0;}
bool ResidentScene::Ensure(){EGLContext now=eglGetCurrentContext();if(now==EGL_NO_CONTEXT)return false;if(context!=now){Reset(false);context=now;}if(full&&particles&&vao&&bed&&particleVao)return true;
 full=Program(kGrsFullVertex,kGrsSceneFragment);particles=Program(kGrsParticleVertex,kGrsParticleFragment);if(!full||!particles)return false;glGenVertexArrays(1,&vao);glGenVertexArrays(1,&particleVao);glGenTextures(1,&bed);glBindTexture(GL_TEXTURE_2D,bed);glTexStorage2D(GL_TEXTURE_2D,1,GL_R32F,128,1);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);return glGetError()==GL_NO_ERROR;
}
void ResidentScene::Geometry(GLuint p,const ResidentSceneFrame& f){glUseProgram(p);glUniform2f(glGetUniformLocation(p,"uResolution"),float(f.width),float(f.height));glUniform4f(glGetUniformLocation(p,"uBox"),f.boxX,f.boxY,f.boxWidth,f.boxHeight);glUniform1i(glGetUniformLocation(p,"uMirror"),f.mirror);}
std::string ResidentScene::Token(const PublicationLease& p,const ResidentSceneFrame& f){std::ostringstream s;s<<std::setprecision(9)<<"GRS_GPU_SCENE_Q20_V1|"<<value(p.identity.run)<<'|'<<value(p.identity.owner)<<'|'<<value(p.identity.context)<<'|'<<value(p.identity.surface)<<'|'<<value(p.identity.geometry)<<'|'<<value(p.identity.config)<<'|'<<value(p.source)<<'|'<<value(p.physics)<<'|'<<value(p.control)<<'|'<<value(p.frame)<<'|'<<value(p.nonce)<<'|'<<p.bank<<'|'<<value(p.generation)<<'|'<<p.angle<<'|'<<f.renderFrame<<'|'<<f.width<<'|'<<f.height<<'|'<<f.boxX<<'|'<<f.boxY<<'|'<<f.boxWidth<<'|'<<f.boxHeight<<'|'<<f.dpi<<'|'<<f.mirror;for(const auto* a:{&p.identity.shaderSha,&p.identity.angleSha,&p.identity.layoutSha})for(auto word:*a)s<<'|'<<word;s<<'|'<<kGrsSceneShaderSha;return s.str();}
ResidentSceneReceipt ResidentScene::Draw(SourceGpuTransport& source,uint32_t slot,U64 generation,const ResidentSceneFrame& f,Swap swap,void* user){
 ResidentSceneReceipt r;r.stage="GRS_SCENE_GEOMETRY";
 if(f.renderFrame<=lastRenderFrame||f.width<=0||f.height<=0||!std::isfinite(f.boxX)||!std::isfinite(f.boxY)||!std::isfinite(f.boxWidth)||!std::isfinite(f.boxHeight)||!std::isfinite(f.dpi)||f.dpi<=0||f.boxX<0||f.boxY<0||f.boxWidth<128||f.boxHeight<=0||f.boxX+f.boxWidth>float(f.width)||f.boxY+f.boxHeight>float(f.height))return r;
 const auto& header=source.Owner().Header();if(generation.lo!=header[72]||generation.hi!=header[73]){r.stage="GRS_SCENE_STALE_PUBLICATION";return r;}
 PublicationLease lease;GLuint buffer=0;r.stage="GRS_SCENE_LEASE";if(source.Acquire(slot,generation,lease,buffer)!=ControlCode::OK)return r;
 r.requestedToken=Token(lease,f);
 bool valid=equal(lease.identity.run,f.run)&&equal(lease.identity.context,f.context)&&equal(lease.identity.surface,f.surface)&&equal(lease.identity.geometry,f.geometry);
 bool success=valid;const char* stage=valid?"GRS_SCENE_SHADER":"GRS_SCENE_TOKEN";
 if(success)success=Ensure();
 lastRenderFrame=f.renderFrame;
 auto checked=[&](){GLenum e=glGetError();if(e!=GL_NO_ERROR&&r.rawGl==0)r.rawGl=e;return e==GL_NO_ERROR;};
 if(success){stage="GRS_SCENE_TARGET";success=targets.Ensure(f.width,f.height,value(f.context))==p013::Status::OK;r.frameTargetBytes=targets.Bytes();}
 if(success){
  stage="GRS_SCENE_PUBLICATION_UPLOAD";const auto& layout=source.PublicationLayout();glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,bed);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,buffer);glPixelStorei(GL_UNPACK_ALIGNMENT,4);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,128,1,GL_RED,GL_FLOAT,reinterpret_cast<void*>((layout.column+3*128)*4));glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);success=checked();
 }
 if(success){
  glBindFramebuffer(GL_FRAMEBUFFER,targets.fbos[targets.candidate]);glViewport(0,0,f.width,f.height);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_SAMPLE_COVERAGE);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDisable(GL_BLEND);
  stage="GRS_SCENE_BACKGROUND";Geometry(full,f);glBindVertexArray(vao);glUniform1i(glGetUniformLocation(full,"uPass"),0);glDrawArrays(GL_TRIANGLES,0,3);success=checked();
 }
 if(success){stage="GRS_SCENE_BED";glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,bed);glUniform1i(glGetUniformLocation(full,"uBed"),0);glUniform1i(glGetUniformLocation(full,"uPass"),1);glDrawArrays(GL_TRIANGLES,0,3);success=checked();}
 if(success){
  stage="GRS_SCENE_PARTICLES";Geometry(particles,f);glBindVertexArray(particleVao);glBindBuffer(GL_ARRAY_BUFFER,buffer);uint32_t capacity=header[41]+header[43];const auto& layout=source.PublicationLayout();
  const uint32_t fields[5]={8,9,3,12,10};for(uint32_t i=0;i<5;i++){size_t offset=(layout.sand+fields[i]*capacity)*4;if(i==2||i==3)glVertexAttribIPointer(i,1,GL_UNSIGNED_INT,4,reinterpret_cast<void*>(offset));else glVertexAttribPointer(i,1,GL_FLOAT,GL_FALSE,4,reinterpret_cast<void*>(offset));glEnableVertexAttribArray(i);glVertexAttribDivisor(i,1);}
  glDrawArraysInstanced(GL_TRIANGLES,0,6,capacity);glBindBuffer(GL_ARRAY_BUFFER,0);success=checked();
 }
 if(success){stage="GRS_SCENE_FOREGROUND";Geometry(full,f);glBindVertexArray(vao);glUniform1i(glGetUniformLocation(full,"uPass"),2);glDrawArrays(GL_TRIANGLES,0,3);success=checked();}
 if(success){stage="GRS_SCENE_PRESENT";glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,f.width,f.height);glDisable(GL_BLEND);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,targets.textures[targets.candidate]);glUniform1i(glGetUniformLocation(full,"uFrame"),1);glUniform1i(glGetUniformLocation(full,"uPass"),3);glDrawArrays(GL_TRIANGLES,0,3);success=checked();}
 if(success){stage="GRS_SCENE_SWAP";success=swap&&swap(user);r.swapSucceeded=success;if(!success)r.rawEgl=eglGetError();}
 GLenum raw=glGetError();if(raw!=GL_NO_ERROR){success=false;r.rawGl=raw;}
 GLsync release=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();if(!release||source.ReleasePublication(slot,generation,release)!=ControlCode::OK){success=false;stage="GRS_SCENE_RELEASE";}
 if(success){targets.Commit();lastGood=r.requestedToken;r.renderedToken=lastGood;r.current=true;r.stage="NONE";}else r.stage=stage;
 return r;
}
}
