// Independent T522 V5 renderer. See generate_scene.py for versioned reuse.
#include "scene.h"
#include "m3b_scene_sources.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstring>
namespace grs {namespace m3b {
namespace {
bool equal(U64 a,U64 b){return a.lo==b.lo&&a.hi==b.hi;}uint64_t value(U64 x){return (uint64_t(x.hi)<<32)|x.lo;}
}
std::array<programcache::GraphicsProgramDesc,3> ScenePrograms() {
 return{{
  {"SCENE_FULL",{{{GL_VERTEX_SHADER,kM3FullVertex},{GL_FRAGMENT_SHADER,kM3SceneFragment}}},
   "M3_SCENE_ABI_V5_MEDIUM3|color:vec4@0|Publication:std430:uint[]@0|uResolution:vec2|uBox:vec4|uMirror:bool|uColumn:uint|uLayer:int|uFrame:sampler2D|uOptics:vec4[4]"},
  {"SCENE_PARTICLES",{{{GL_VERTEX_SHADER,kM3ParticleVertex},{GL_FRAGMENT_SHADER,kM3ParticleFragment}}},
   "M3_SCENE_ABI_V5_MEDIUM3|color:vec4@0|AcceptedState:std430:uint[]@1|uK:uint|aX:float@0|aY:float@1|aMass:uint@2|aState:uint@3:optional|aRadius:float@4|uResolution:vec2|uBox:vec4|uMirror:bool"},
  {"SCENE_WATER",{{{GL_VERTEX_SHADER,kM3WaterVertex},{GL_FRAGMENT_SHADER,kM3WaterFragment}}},
   "M3_SCENE_ABI_V5_MEDIUM3|color:vec4@0|aX:float@0|aY:float@1|aMass:uint@2|uResolution:vec2|uBox:vec4|uMirror:bool|uRadius:float|uMassReference:float|uOptics:vec4[3+]"}
 }};
}
void Scene::Reset(bool current) {
 current=current&&context!=EGL_NO_CONTEXT&&eglGetCurrentContext()==context;
 targets.Reset(current);
 for(GLuint program:{full,particles,water})if(program) {
  programcache::ForgetGraphics(program,contextGeneration,context);
  if(current)glDeleteProgram(program);
 }
 if(current){if(vao)glDeleteVertexArrays(1,&vao);if(particleVao)glDeleteVertexArrays(1,&particleVao);}
 full=particles=water=vao=particleVao=0;context=EGL_NO_CONTEXT;contextGeneration=0;
 uniforms={};lastGood.clear();lastRenderFrame=0;opticsBound=false;
}
bool Scene::PreparePrograms(uint64_t generation,CandidateCheck check,void* user,bool cacheEnabled) {
 const auto now=eglGetCurrentContext();
 if(now==EGL_NO_CONTEXT||!generation){prepareFailure="SCENE_CONTEXT_REQUIRED";return false;}
 if(context!=now||contextGeneration!=generation)Reset(false);
 if(full&&particles&&water&&vao&&particleVao)return !check||check(user);
 const auto start=programcache::Stamp();
 prepareFailure.clear();
 std::array<GLuint,3> programs{};GLuint vertices=0,instances=0;
 std::array<programcache::GraphicsUniformTable,3> tables{};
 auto fail=[&](const std::string& reason) {
  prepareFailure=reason;prepareWallNs=programcache::Stamp()-start;
  const bool current=eglGetCurrentContext()==now;
  for(auto program:programs)if(program){programcache::ForgetGraphics(program,generation,now);if(current)glDeleteProgram(program);}
  if(current){if(vertices)glDeleteVertexArrays(1,&vertices);if(instances)glDeleteVertexArrays(1,&instances);}
  return false;
 };
 const auto descriptors=ScenePrograms();
 GLint fragments=0;glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,&fragments);
 if(fragments<1)return fail("SCENE_FRAGMENT_SSBO_UNSUPPORTED");
 glGenVertexArrays(1,&vertices);glGenVertexArrays(1,&instances);
 glBindVertexArray(vertices);glBindVertexArray(instances);glBindVertexArray(0);
 const auto vaoError=glGetError();
 if(!vertices||!instances||vaoError!=GL_NO_ERROR)return fail("SCENE_VAO_CREATE/"+std::to_string(vaoError));
 for(size_t i=0;i<programs.size();++i) {
  if(check&&!check(user))return fail("SCENE_CANDIDATE_INVALIDATED");
  GLenum error=0;
  if(!programcache::BuildGraphics(descriptors[i],programs[i],error,generation,cacheEnabled))
   return fail(std::string(descriptors[i].role)+"/COMPILE_OR_LOAD/"+std::to_string(error));
  if(eglGetCurrentContext()!=now||(check&&!check(user)))return fail("SCENE_CANDIDATE_INVALIDATED");
  if(!programcache::ReflectGraphics(programs[i],tables[i],descriptors[i],error,generation))
   return fail(std::string(descriptors[i].role)+"/REFLECT/"+std::to_string(error));
  if(programcache::GraphicsPrepared(programs[i],generation)!=GL_NO_ERROR)
   return fail(std::string(descriptors[i].role)+"/VAO_PREPARE");
 }
 if(eglGetCurrentContext()!=now||(check&&!check(user)))return fail("SCENE_CANDIDATE_INVALIDATED");
 full=programs[0];particles=programs[1];water=programs[2];vao=vertices;particleVao=instances;
 uniforms=std::move(tables);context=now;contextGeneration=generation;
 prepareWallNs=programcache::Stamp()-start;return true;
}
bool Scene::Ensure(uint64_t generation){return PreparePrograms(generation);}
void Scene::Geometry(GLuint program,const ResidentSceneFrame& f) {
 const auto& table=uniforms[program==full?0:program==particles?1:2];
 glUseProgram(program);glUniform2f(table.Find("uResolution"),float(f.width),float(f.height));
 glUniform4f(table.Find("uBox"),f.boxX,f.boxY,f.boxWidth,f.boxHeight);
 glUniform1i(table.Find("uMirror"),f.mirror);
}
std::string Scene::Token(const PublicationLease& p,const ResidentSceneFrame& f,const SceneOptics& optics){std::ostringstream s;s<<std::setprecision(9)<<"GRS_GPU_SCENE_Q20_M3_V5|"<<value(p.identity.run)<<'|'<<value(p.identity.owner)<<'|'<<value(p.identity.context)<<'|'<<value(p.identity.surface)<<'|'<<value(p.identity.geometry)<<'|'<<value(p.identity.config)<<'|'<<value(p.source)<<'|'<<value(p.physics)<<'|'<<value(p.control)<<'|'<<value(p.frame)<<'|'<<value(p.nonce)<<'|'<<p.bank<<'|'<<value(p.generation)<<'|'<<p.angle<<'|'<<f.renderFrame<<'|'<<f.width<<'|'<<f.height<<'|'<<f.boxX<<'|'<<f.boxY<<'|'<<f.boxWidth<<'|'<<f.boxHeight<<'|'<<f.dpi<<'|'<<f.mirror;for(const auto* a:{&p.identity.shaderSha,&p.identity.angleSha,&p.identity.layoutSha})for(auto word:*a)s<<'|'<<word;s<<'|'<<kM3SceneSha;for(float v:optics.values){uint32_t bits;std::memcpy(&bits,&v,4);s<<'|'<<bits;}s<<'|'<<optics.waterRadiusPx<<'|'<<optics.massReference;return s.str();}
ResidentSceneReceipt Scene::Draw(SceneSourceV5& source,uint32_t slot,U64 generation,const ResidentSceneFrame& f,ResidentScene::Swap swap,void* user,const SceneOptics& optics,Capture capture,TokenReady tokenReady){
 ResidentSceneReceipt r;r.stage="GRS_SCENE_GEOMETRY";
 if(f.renderFrame<=lastRenderFrame||f.width<=0||f.height<=0||!std::isfinite(f.boxX)||!std::isfinite(f.boxY)||!std::isfinite(f.boxWidth)||!std::isfinite(f.boxHeight)||!std::isfinite(f.dpi)||f.dpi<=0||f.boxX<0||f.boxY<0||f.boxWidth<128||f.boxHeight<=0||f.boxX+f.boxWidth>float(f.width)||f.boxY+f.boxHeight>float(f.height))return r;
 for(float v:optics.values)if(!std::isfinite(v)){r.stage="M1_OPTICS_FINITE";return r;}const auto& o=optics.values;if(o[0]<0||o[5]!=1||o[6]!=1||o[7]!=0||o[8]<=0||o[8]>8||o[13]<1||o[13]>64||std::abs(std::hypot(o[14],o[15])-1)>1e-6||!std::isfinite(optics.waterRadiusPx)||optics.waterRadiusPx<=0||!std::isfinite(optics.massReference)||optics.massReference<=0){r.stage="M1_OPTICS_DOMAIN";return r;}for(int i:{1,2,3,4,9,10,11,12})if(o[i]<0||o[i]>1){r.stage="M1_OPTICS_COLOR";return r;}
 const auto header=source.SceneHeaderV5();if(header[1]!=5||header[31]!=31||header[32]!=0||header[27]!=3||header[122]!=1||header[124]!=2||header[125]!=2){r.stage="M1_SCENE_REVISION_FEATURE";return r;}if(generation.lo!=header[72]||generation.hi!=header[73]){r.stage="GRS_SCENE_STALE_PUBLICATION";return r;}
 PublicationLease lease;GLuint buffer=0;r.stage="GRS_SCENE_LEASE";if(source.AcquireV5(slot,generation,lease,buffer)!=ControlCode::OK)return r;
 r.requestedToken=Token(lease,f,optics);
 if(tokenReady)tokenReady(r.requestedToken,user);
 bool valid=equal(lease.identity.run,f.run)&&equal(lease.identity.context,f.context)&&equal(lease.identity.surface,f.surface)&&equal(lease.identity.geometry,f.geometry);
 if(opticsBound&&(std::memcmp(boundOptics.values.data(),optics.values.data(),64)!=0||boundOptics.waterRadiusPx!=optics.waterRadiusPx||boundOptics.massReference!=optics.massReference)){GLsync released=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();source.ReleaseV5(slot,generation,released);r.stage="M1_OPTICS_COLD_CHANGE";return r;}
 GLint fragmentBlocks=0;glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,&fragmentBlocks);if(fragmentBlocks<1){GLsync released=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();source.ReleaseV5(slot,generation,released);r.stage="M1_FRAGMENT_SSBO_UNSUPPORTED";return r;}
 bool success=valid;const char* stage=valid?"GRS_SCENE_SHADER":"GRS_SCENE_TOKEN";
 if(success)success=Ensure(value(f.context));if(success&&!opticsBound){boundOptics=optics;opticsBound=true;}
 lastRenderFrame=f.renderFrame;
 auto checked=[&](){GLenum e=glGetError();if(e!=GL_NO_ERROR&&r.rawGl==0)r.rawGl=e;return e==GL_NO_ERROR;};
 if(success){stage="GRS_SCENE_TARGET";success=targets.Ensure(f.width,f.height,value(f.context))==p013::Status::OK;r.frameTargetBytes=targets.Bytes();}
 if(success){
  stage="M1_SCENE_GPU_PUBLICATION";glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,buffer);Geometry(full,f);glUniform1ui(uniforms[0].Find("uColumn"),16*(header[41]+header[43])+20*(header[42]+header[44]));glUniform4fv(uniforms[0].Find("uOptics"),4,optics.values.data());success=checked();
 }
 if(success){
  glBindFramebuffer(GL_FRAMEBUFFER,targets.fbos[targets.candidate]);glViewport(0,0,f.width,f.height);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);glDisable(GL_SAMPLE_COVERAGE);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDisable(GL_BLEND);
  stage="GRS_SCENE_BACKGROUND";Geometry(full,f);glBindVertexArray(vao);glUniform1i(uniforms[0].Find("uLayer"),0);glDrawArrays(GL_TRIANGLES,0,3);success=checked();if(capture)capture(0,targets.fbos[targets.candidate],user);
 }
 if(success){stage="GRS_SCENE_BED";glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);glUniform1i(uniforms[0].Find("uLayer"),1);glDrawArrays(GL_TRIANGLES,0,3);success=checked();if(capture)capture(1,targets.fbos[targets.candidate],user);}
 if(success){
  stage="GRS_SCENE_PARTICLES";Geometry(particles,f);const auto stateBuffer=source.StateBufferV5(lease);if(!stateBuffer){GLsync released=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();source.ReleaseV5(slot,generation,released);r.stage="V5_STATE_LEASE_REJECTED";return r;}glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,stateBuffer);glUniform1ui(uniforms[1].Find("uK"),header[41]+header[43]);glBindVertexArray(particleVao);glBindBuffer(GL_ARRAY_BUFFER,buffer);uint32_t capacity=header[41]+header[43];
  const uint32_t fields[5]={8,9,3,12,10};for(uint32_t i=0;i<5;i++){size_t offset=(fields[i]*capacity)*4;if(i==2||i==3)glVertexAttribIPointer(i,1,GL_UNSIGNED_INT,4,reinterpret_cast<void*>(offset));else glVertexAttribPointer(i,1,GL_FLOAT,GL_FALSE,4,reinterpret_cast<void*>(offset));glEnableVertexAttribArray(i);glVertexAttribDivisor(i,1);}
  glDrawArraysInstanced(GL_TRIANGLES,0,6,capacity);glBindBuffer(GL_ARRAY_BUFFER,0);success=checked();if(capture)capture(2,targets.fbos[targets.candidate],user);
 }
 if(success){stage="M1_WATER_BODY";Geometry(full,f);glBindVertexArray(vao);glUniform1i(uniforms[0].Find("uLayer"),3);glDrawArrays(GL_TRIANGLES,0,3);success=checked();if(capture)capture(3,targets.fbos[targets.candidate],user);}
 if(success){stage="M1_WATER_SURFACE";glUniform1i(uniforms[0].Find("uLayer"),4);glDrawArrays(GL_TRIANGLES,0,3);success=checked();if(capture)capture(4,targets.fbos[targets.candidate],user);}
 if(success){stage="M1_FLIGHT_J";Geometry(water,f);glUniform4fv(uniforms[2].Find("uOptics"),4,optics.values.data());glUniform1f(uniforms[2].Find("uRadius"),optics.waterRadiusPx);glUniform1f(uniforms[2].Find("uMassReference"),optics.massReference);glBindVertexArray(particleVao);glBindBuffer(GL_ARRAY_BUFFER,buffer);uint32_t capacity=header[42]+header[44],base=16*(header[41]+header[43]),fields[3]={16,17,0};for(uint32_t i=0;i<3;i++){size_t offset=(base+fields[i]*capacity)*4;if(i==2)glVertexAttribIPointer(i,1,GL_UNSIGNED_INT,4,reinterpret_cast<void*>(offset));else glVertexAttribPointer(i,1,GL_FLOAT,GL_FALSE,4,reinterpret_cast<void*>(offset));glEnableVertexAttribArray(i);glVertexAttribDivisor(i,1);}glDrawArraysInstanced(GL_TRIANGLES,0,6,capacity);glBindBuffer(GL_ARRAY_BUFFER,0);success=checked();if(capture)capture(6,targets.fbos[targets.candidate],user);}
 if(success){stage="GRS_SCENE_FOREGROUND";Geometry(full,f);glBindVertexArray(vao);glUniform1i(uniforms[0].Find("uLayer"),5);glDrawArrays(GL_TRIANGLES,0,3);success=checked();if(capture)capture(5,targets.fbos[targets.candidate],user);}
 if(success){stage="GRS_SCENE_PRESENT";Geometry(full,f);glBindVertexArray(vao);glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,f.width,f.height);glDisable(GL_BLEND);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,targets.textures[targets.candidate]);glUniform1i(uniforms[0].Find("uFrame"),1);glUniform1i(uniforms[0].Find("uLayer"),6);glDrawArrays(GL_TRIANGLES,0,3);success=checked();}
 if(success){stage="GRS_SCENE_SWAP";success=swap&&swap(user);r.swapSucceeded=success;if(!success)r.rawEgl=eglGetError();}
 GLenum raw=glGetError();if(raw!=GL_NO_ERROR){success=false;r.rawGl=raw;}
 GLsync release=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();if(!release||source.ReleaseV5(slot,generation,release)!=ControlCode::OK){success=false;stage="GRS_SCENE_RELEASE";}
 if(success){targets.Commit();lastGood=r.requestedToken;r.renderedToken=lastGood;r.current=true;r.stage="NONE";}else r.stage=stage;
 return r;
}
}

}
