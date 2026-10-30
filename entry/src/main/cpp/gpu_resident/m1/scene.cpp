#include "scene.h"
#include "m1_scene_sources.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstring>
namespace grs {namespace m1 {
namespace {
GLuint Shader(GLenum kind,const char* text){GLuint s=glCreateShader(kind);glShaderSource(s,1,&text,nullptr);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){glDeleteShader(s);return 0;}return s;}
GLuint Program(const char* vs,const char* fs){GLuint v=Shader(GL_VERTEX_SHADER,vs),f=Shader(GL_FRAGMENT_SHADER,fs);if(!v||!f){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return 0;}GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glDeleteShader(v);glDeleteShader(f);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){glDeleteProgram(p);return 0;}return p;}
bool equal(U64 a,U64 b){return a.lo==b.lo&&a.hi==b.hi;}uint64_t value(U64 x){return (uint64_t(x.hi)<<32)|x.lo;}
}
void Scene::Reset(bool current){targets.Reset(current);if(current){if(full)glDeleteProgram(full);if(particles)glDeleteProgram(particles);if(vao)glDeleteVertexArrays(1,&vao);if(particleVao)glDeleteVertexArrays(1,&particleVao);if(water)glDeleteProgram(water);}full=particles=water=vao=particleVao=0;context=EGL_NO_CONTEXT;lastGood.clear();lastRenderFrame=0;opticsBound=false;}
bool Scene::Ensure(){EGLContext now=eglGetCurrentContext();if(now==EGL_NO_CONTEXT)return false;if(context!=now){Reset(false);context=now;}if(full&&particles&&water&&vao&&particleVao)return true;
 GLint fragments=0;glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,&fragments);if(fragments<1)return false;
 full=Program(kM1FullVertex,kM1SceneFragment);particles=Program(kM1ParticleVertex,kM1ParticleFragment);water=Program(kM1WaterVertex,kM1WaterFragment);if(!full||!particles||!water)return false;glGenVertexArrays(1,&vao);glGenVertexArrays(1,&particleVao);return glGetError()==GL_NO_ERROR;
}
void Scene::Geometry(GLuint p,const ResidentSceneFrame& f){glUseProgram(p);glUniform2f(glGetUniformLocation(p,"uResolution"),float(f.width),float(f.height));glUniform4f(glGetUniformLocation(p,"uBox"),f.boxX,f.boxY,f.boxWidth,f.boxHeight);glUniform1i(glGetUniformLocation(p,"uMirror"),f.mirror);}
std::string Scene::Token(const PublicationLease& p,const ResidentSceneFrame& f,const SceneOptics& optics){std::ostringstream s;s<<std::setprecision(9)<<"GRS_GPU_SCENE_Q20_M1_V2|"<<value(p.identity.run)<<'|'<<value(p.identity.owner)<<'|'<<value(p.identity.context)<<'|'<<value(p.identity.surface)<<'|'<<value(p.identity.geometry)<<'|'<<value(p.identity.config)<<'|'<<value(p.source)<<'|'<<value(p.physics)<<'|'<<value(p.control)<<'|'<<value(p.frame)<<'|'<<value(p.nonce)<<'|'<<p.bank<<'|'<<value(p.generation)<<'|'<<p.angle<<'|'<<f.renderFrame<<'|'<<f.width<<'|'<<f.height<<'|'<<f.boxX<<'|'<<f.boxY<<'|'<<f.boxWidth<<'|'<<f.boxHeight<<'|'<<f.dpi<<'|'<<f.mirror;for(const auto* a:{&p.identity.shaderSha,&p.identity.angleSha,&p.identity.layoutSha})for(auto word:*a)s<<'|'<<word;s<<'|'<<kM1SceneSha;for(float v:optics.values){uint32_t bits;std::memcpy(&bits,&v,4);s<<'|'<<bits;}s<<'|'<<optics.waterRadiusPx<<'|'<<optics.massReference;return s.str();}
ResidentSceneReceipt Scene::Draw(FullM1Transport& source,uint32_t slot,U64 generation,const ResidentSceneFrame& f,ResidentScene::Swap swap,void* user,const SceneOptics& optics,Capture capture){
 ResidentSceneReceipt r;r.stage="GRS_SCENE_GEOMETRY";
 if(f.renderFrame<=lastRenderFrame||f.width<=0||f.height<=0||!std::isfinite(f.boxX)||!std::isfinite(f.boxY)||!std::isfinite(f.boxWidth)||!std::isfinite(f.boxHeight)||!std::isfinite(f.dpi)||f.dpi<=0||f.boxX<0||f.boxY<0||f.boxWidth<128||f.boxHeight<=0||f.boxX+f.boxWidth>float(f.width)||f.boxY+f.boxHeight>float(f.height))return r;
 for(float v:optics.values)if(!std::isfinite(v)){r.stage="M1_OPTICS_FINITE";return r;}const auto& o=optics.values;if(o[0]<0||o[5]!=1||o[6]!=1||o[7]!=0||o[8]<=0||o[8]>8||o[13]<1||o[13]>64||std::abs(std::hypot(o[14],o[15])-1)>1e-6||!std::isfinite(optics.waterRadiusPx)||optics.waterRadiusPx<=0||!std::isfinite(optics.massReference)||optics.massReference<=0){r.stage="M1_OPTICS_DOMAIN";return r;}for(int i:{1,2,3,4,9,10,11,12})if(o[i]<0||o[i]>1){r.stage="M1_OPTICS_COLOR";return r;}
 const auto& header=source.Owner().Header();if(header[1]!=2||header[31]!=3||header[32]!=0||header[122]!=1||header[124]!=1||header[125]!=1){r.stage="M1_SCENE_REVISION_FEATURE";return r;}if(generation.lo!=header[72]||generation.hi!=header[73]){r.stage="GRS_SCENE_STALE_PUBLICATION";return r;}
 PublicationLease lease;GLuint buffer=0;r.stage="GRS_SCENE_LEASE";if(source.Acquire(slot,generation,lease,buffer)!=ControlCode::OK)return r;
 r.requestedToken=Token(lease,f,optics);
 bool valid=equal(lease.identity.run,f.run)&&equal(lease.identity.context,f.context)&&equal(lease.identity.surface,f.surface)&&equal(lease.identity.geometry,f.geometry);
 if(opticsBound&&(std::memcmp(boundOptics.values.data(),optics.values.data(),64)!=0||boundOptics.waterRadiusPx!=optics.waterRadiusPx||boundOptics.massReference!=optics.massReference)){GLsync released=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();source.ReleasePublication(slot,generation,released);r.stage="M1_OPTICS_COLD_CHANGE";return r;}
 GLint fragmentBlocks=0;glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,&fragmentBlocks);if(fragmentBlocks<1){GLsync released=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();source.ReleasePublication(slot,generation,released);r.stage="M1_FRAGMENT_SSBO_UNSUPPORTED";return r;}
 bool success=valid;const char* stage=valid?"GRS_SCENE_SHADER":"GRS_SCENE_TOKEN";
 if(success)success=Ensure();if(success&&!opticsBound){boundOptics=optics;opticsBound=true;}
 lastRenderFrame=f.renderFrame;
 auto checked=[&](){GLenum e=glGetError();if(e!=GL_NO_ERROR&&r.rawGl==0)r.rawGl=e;return e==GL_NO_ERROR;};
 if(success){stage="GRS_SCENE_TARGET";success=targets.Ensure(f.width,f.height,value(f.context))==p013::Status::OK;r.frameTargetBytes=targets.Bytes();}
 if(success){
  stage="M1_SCENE_GPU_PUBLICATION";glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,buffer);Geometry(full,f);glUniform1ui(glGetUniformLocation(full,"uColumn"),16*(header[41]+header[43])+20*(header[42]+header[44]));glUniform4fv(glGetUniformLocation(full,"uOptics"),4,optics.values.data());success=checked();
 }
 if(success){
  glBindFramebuffer(GL_FRAMEBUFFER,targets.fbos[targets.candidate]);glViewport(0,0,f.width,f.height);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_SAMPLE_COVERAGE);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDisable(GL_BLEND);
  stage="GRS_SCENE_BACKGROUND";Geometry(full,f);glBindVertexArray(vao);glUniform1i(glGetUniformLocation(full,"uLayer"),0);glDrawArrays(GL_TRIANGLES,0,3);success=checked();if(capture)capture(0,targets.fbos[targets.candidate],user);
 }
 if(success){stage="GRS_SCENE_BED";glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);glUniform1i(glGetUniformLocation(full,"uLayer"),1);glDrawArrays(GL_TRIANGLES,0,3);success=checked();if(capture)capture(1,targets.fbos[targets.candidate],user);}
 if(success){
  stage="GRS_SCENE_PARTICLES";Geometry(particles,f);glBindVertexArray(particleVao);glBindBuffer(GL_ARRAY_BUFFER,buffer);uint32_t capacity=header[41]+header[43];
  const uint32_t fields[5]={8,9,3,12,10};for(uint32_t i=0;i<5;i++){size_t offset=(fields[i]*capacity)*4;if(i==2||i==3)glVertexAttribIPointer(i,1,GL_UNSIGNED_INT,4,reinterpret_cast<void*>(offset));else glVertexAttribPointer(i,1,GL_FLOAT,GL_FALSE,4,reinterpret_cast<void*>(offset));glEnableVertexAttribArray(i);glVertexAttribDivisor(i,1);}
  glDrawArraysInstanced(GL_TRIANGLES,0,6,capacity);glBindBuffer(GL_ARRAY_BUFFER,0);success=checked();if(capture)capture(2,targets.fbos[targets.candidate],user);
 }
 if(success){stage="M1_WATER_BODY";Geometry(full,f);glBindVertexArray(vao);glUniform1i(glGetUniformLocation(full,"uLayer"),3);glDrawArrays(GL_TRIANGLES,0,3);success=checked();if(capture)capture(3,targets.fbos[targets.candidate],user);}
 if(success){stage="M1_WATER_SURFACE";glUniform1i(glGetUniformLocation(full,"uLayer"),4);glDrawArrays(GL_TRIANGLES,0,3);success=checked();if(capture)capture(4,targets.fbos[targets.candidate],user);}
 if(success){stage="M1_FLIGHT_J";Geometry(water,f);glUniform4fv(glGetUniformLocation(water,"uOptics"),4,optics.values.data());glUniform1f(glGetUniformLocation(water,"uRadius"),optics.waterRadiusPx);glUniform1f(glGetUniformLocation(water,"uMassReference"),optics.massReference);glBindVertexArray(particleVao);glBindBuffer(GL_ARRAY_BUFFER,buffer);uint32_t capacity=header[42]+header[44],base=16*(header[41]+header[43]),fields[3]={16,17,0};for(uint32_t i=0;i<3;i++){size_t offset=(base+fields[i]*capacity)*4;if(i==2)glVertexAttribIPointer(i,1,GL_UNSIGNED_INT,4,reinterpret_cast<void*>(offset));else glVertexAttribPointer(i,1,GL_FLOAT,GL_FALSE,4,reinterpret_cast<void*>(offset));glEnableVertexAttribArray(i);glVertexAttribDivisor(i,1);}glDrawArraysInstanced(GL_TRIANGLES,0,6,capacity);glBindBuffer(GL_ARRAY_BUFFER,0);success=checked();if(capture)capture(6,targets.fbos[targets.candidate],user);}
 if(success){stage="GRS_SCENE_FOREGROUND";Geometry(full,f);glBindVertexArray(vao);glUniform1i(glGetUniformLocation(full,"uLayer"),5);glDrawArrays(GL_TRIANGLES,0,3);success=checked();if(capture)capture(5,targets.fbos[targets.candidate],user);}
 if(success){stage="GRS_SCENE_PRESENT";Geometry(full,f);glBindVertexArray(vao);glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,f.width,f.height);glDisable(GL_BLEND);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,targets.textures[targets.candidate]);glUniform1i(glGetUniformLocation(full,"uFrame"),1);glUniform1i(glGetUniformLocation(full,"uLayer"),6);glDrawArrays(GL_TRIANGLES,0,3);success=checked();}
 if(success){stage="GRS_SCENE_SWAP";success=swap&&swap(user);r.swapSucceeded=success;if(!success)r.rawEgl=eglGetError();}
 GLenum raw=glGetError();if(raw!=GL_NO_ERROR){success=false;r.rawGl=raw;}
 GLsync release=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();if(!release||source.ReleasePublication(slot,generation,release)!=ControlCode::OK){success=false;stage="GRS_SCENE_RELEASE";}
 if(success){targets.Commit();lastGood=r.requestedToken;r.renderedToken=lastGood;r.current=true;r.stage="NONE";}else r.stage=stage;
 return r;
}
}

}
