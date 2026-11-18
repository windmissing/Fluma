// T576 (problem T567): G12 scene implementation. J uses the new
// waterPacket32 integer instance attributes (mass4/xQ20/y4Q22) from the
// CURRENT state buffer; the P pass reuses the parent integer attribute ABI.
#include "scene.h"
#include <string>
namespace grs::g12water {
namespace {
const char* kSceneVertex=R"G12S(#version 310 es
precision highp int;
uniform vec2 uResolution;uniform vec4 uBox;
void main(){vec2 c=vec2(gl_VertexID==1?1.0:0.0,gl_VertexID==2?1.0:0.0);gl_Position=vec4(c*2.0-1.0,0.0,1.0);}
)G12S";
const char* kSceneFragment=R"G12F(#version 310 es
precision highp int;
layout(std430,binding=0)readonly buffer State{uint S[];};
uniform vec2 uResolution;uniform vec4 uBox;uniform int uN;
layout(location=0) out vec4 color;
void main(){
 vec2 pixel=gl_FragCoord.xy/uResolution;
 uint column=uint(clamp(pixel.x,0.0,0.999)*128.0);
 uint colAt=512u+column*32u;
 uint bed=S[colAt+0u],flow=S[colAt+1u];
 // Water tail: waterColumn16 at the profile-dependent tail offset.
 uint wcol=uint(uN);
 uint h=S[wcol+column*16u+0u],eta=S[wcol+column*16u+4u];
 vec3 sand=vec3(0.72,0.56,0.32);vec3 water=vec3(0.35,0.55,0.80);
 float sandFill=bed==0u&&flow==0u?0.0:0.6;
 float waterFill=eta==0u?0.0:clamp(float(h)/float(eta+1u),0.0,1.0)*0.4;
 color=vec4(mix(sand*max(sandFill,0.35),water,waterFill),1.0);
}
)G12F";
const char* kPacketVertex=R"G12P(#version 310 es
precision highp int;
layout(location=0) in uint aMass;layout(location=1) in uint aXQ20;layout(location=2) in uint aYQ21;
uniform vec2 uResolution;uniform vec4 uBox;out float vShade;
void main(){
 vec2 center=vec2(float(aXQ20)/1048576.0,float(aYQ21)/2097152.0);
 const vec2 kC[6]=vec2[6](vec2(0,0),vec2(1,0),vec2(1,1),vec2(0,0),vec2(1,1),vec2(0,1));
 vShade=clamp(float(aMass)/268435456.0,0.25,1.0);
 gl_Position=vec4((center+kC[gl_VertexID]*0.01)*2.0-1.0,0.0,1.0);
}
)G12P";
const char* kPacketFragment=R"G12PF(#version 310 es
precision highp int;in float vShade;layout(location=0) out vec4 color;
void main(){color=vec4(vec3(0.35,0.50,0.75)*vShade,1.0);}
)G12PF";
const char* kWaterJVertex=R"G12J(#version 310 es
precision highp int;
layout(location=0) in uint aMass4;layout(location=1) in uint aXQ20;layout(location=2) in uint aY4Q22;
uniform vec2 uResolution;uniform vec4 uBox;out float vDrop;
void main(){
 vec2 center=vec2(float(aXQ20)/1048576.0,float(aY4Q22)/4194304.0);
 const vec2 kC[6]=vec2[6](vec2(0,0),vec2(1,0),vec2(1,1),vec2(0,0),vec2(1,1),vec2(0,1));
 vDrop=clamp(float(aMass4)/268435456.0,0.2,1.0);
 gl_Position=vec4((center+kC[gl_VertexID]*0.008)*2.0-1.0,0.0,1.0);
}
)G12J";
const char* kWaterJFragment=R"G12JF(#version 310 es
precision highp int;in float vDrop;layout(location=0) out vec4 color;
void main(){color=vec4(vec3(0.45,0.65,0.92)*vDrop,0.9);}
)G12JF";
}
bool Scene::Reflect(GLuint program,size_t index,uint64_t generation,const programcache::GraphicsProgramDesc& desc){
 auto* record=programcache::GraphicsCurrent(program,generation);if(!record||record->abi!=desc.bindingAbi||!uniforms_[index].Prepare(program))return false;
 record->reflectBegin=programcache::Stamp();
 bool valid=uniforms_[index].Find("uResolution")>=0&&uniforms_[index].Find("uBox")>=0;
 if(index==0){valid=valid&&uniforms_[index].Find("uN")>=0;}
 else{
  const GLenum properties[]={GL_TYPE,GL_ARRAY_SIZE,GL_LOCATION};
  for(GLuint location=0;location<3&&valid;++location){
   const char* name=index==1?(location==0?"aMass":location==1?"aXQ20":"aYQ21"):(location==0?"aMass4":location==1?"aXQ20":"aY4Q22");
   const auto input=glGetProgramResourceIndex(program,GL_PROGRAM_INPUT,name);
   GLint fields[3]={};if(input!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_PROGRAM_INPUT,input,3,properties,3,nullptr,fields);
   valid=valid&&input!=GL_INVALID_INDEX&&fields[0]==GL_UNSIGNED_INT&&fields[1]==1&&fields[2]==GLint(location);
  }
 }
 const auto colorOutput=glGetProgramResourceIndex(program,GL_PROGRAM_OUTPUT,"color");const GLenum properties[]={GL_TYPE,GL_ARRAY_SIZE,GL_LOCATION};GLint fields[3]={};
 if(colorOutput!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_PROGRAM_OUTPUT,colorOutput,3,properties,3,nullptr,fields);
 valid=valid&&fields[0]==GL_FLOAT_VEC4&&fields[1]==1&&fields[2]==0;
 const auto error=glGetError();record->reflectEnd=programcache::Stamp();
 programcache::GraphicsReceipt(*record,valid&&!error?"G12_REFLECT_V1_OK":"G12_REFLECT_V1_FAILED");
 return valid&&!error;
}
bool Scene::PreparePrograms(uint64_t generation){
 const auto context=eglGetCurrentContext();if(context==EGL_NO_CONTEXT||!generation)return false;
 if(context_!=context||generation_!=generation)Reset(false);
 if(programs_[0]&&programs_[1]&&programs_[2]&&vao_&&pVao_&&jVao_)return true;
 context_=context;generation_=generation;
 const std::array<programcache::GraphicsProgramDesc,3> descriptions={{
  {"G12_SCENE",{{{GL_VERTEX_SHADER,kSceneVertex},{GL_FRAGMENT_SHADER,kSceneFragment}}},"G12_POROUS_SCENE_V1|State:std430:uint[]@0|uResolution:vec2|uBox:vec4|uN:int|color:vec4@0"},
  {"G12_PACKETS",{{{GL_VERTEX_SHADER,kPacketVertex},{GL_FRAGMENT_SHADER,kPacketFragment}}},"G12_POROUS_PACKETS_V1|no-vertex-SSBO|aMass:u32@0|aXQ20:u32@1|aYQ21:u32@2|stride256:divisor1|color:vec4@0"},
  {"G12_WATER_J",{{{GL_VERTEX_SHADER,kWaterJVertex},{GL_FRAGMENT_SHADER,kWaterJFragment}}},"G12_POROUS_WATER_J_V1|no-vertex-SSBO|aMass4:u32@0|aXQ20:u32@1|aY4Q22:u32@2|stride128:divisor1|color:vec4@0"}
 }};
 glGenVertexArrays(1,&vao_);glGenVertexArrays(1,&pVao_);glGenVertexArrays(1,&jVao_);
 if(!vao_||!pVao_||!jVao_||glGetError()!=GL_NO_ERROR){Reset(true);return false;}
 for(size_t i=0;i<3;++i){GLenum error=0;if(!programcache::BuildGraphics(descriptions[i],programs_[i],error,generation)||!Reflect(programs_[i],i,generation,descriptions[i])||programcache::GraphicsPrepared(programs_[i],generation)!=GL_NO_ERROR){Reset(true);return false;}}
 return true;
}
void Scene::Reset(bool current){
 current=current&&context_==eglGetCurrentContext();targets_.Reset(current);
 for(auto p:programs_)if(p){programcache::ForgetGraphics(p,generation_,context_);if(current)glDeleteProgram(p);}
 if(current){if(vao_)glDeleteVertexArrays(1,&vao_);if(pVao_)glDeleteVertexArrays(1,&pVao_);if(jVao_)glDeleteVertexArrays(1,&jVao_);}
 programs_={};uniforms_={};vao_=pVao_=jVao_=0;context_=EGL_NO_CONTEXT;generation_=last_=0;
}
bool Scene::Draw(Transport& transport,uint32_t width,uint32_t height,bool(*swap)(void*),void* user,std::string& token){
 auto buffer=transport.StateBuffer(transport.CurrentBank());
 if(!buffer)return false;
 auto profile=transport.SelectedProfile();
 bool success=width>=128&&height>0&&PreparePrograms(profile?profile->stateBytes:0)&&targets_.Ensure(int(width),int(height),0)==p013::Status::OK;
 auto geometry=[&](size_t index){glUseProgram(programs_[index]);glUniform2f(uniforms_[index].Find("uResolution"),float(width),float(height));glUniform4f(uniforms_[index].Find("uBox"),0,0,float(width),float(height));};
 if(success){
  glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);
  geometry(0);glBindVertexArray(vao_);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,buffer);
  glUniform1i(uniforms_[0].Find("uN"),int((592128+(profile&&profile->k==512?2560u:0u))/4));
  glDrawArrays(GL_TRIANGLES,0,3);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
  // P packets: parent ABI stride 256 at the packet segment.
  if(profile){
   geometry(1);glBindVertexArray(pVao_);glBindBuffer(GL_ARRAY_BUFFER,buffer);
   const uint32_t packetBytes=280576u,offsets[3]={0u,4u,8u};
   for(uint32_t location=0;location<3;++location){glVertexAttribIPointer(location,1,GL_UNSIGNED_INT,256,reinterpret_cast<const void*>(uintptr_t(packetBytes+offsets[location])));glEnableVertexAttribArray(location);glVertexAttribDivisor(location,1);}
   glDrawArraysInstanced(GL_TRIANGLES,0,6,profile->k);
   // J water packets: waterPacket32 tail segment, stride 128.
   geometry(2);glBindVertexArray(jVao_);glBindBuffer(GL_ARRAY_BUFFER,buffer);
   const uint32_t jBytes=uint32_t(993536u+(profile->k==512?2560u:0u));const uint32_t jOffsets[3]={0u,4u,8u};
   for(uint32_t location=0;location<3;++location){glVertexAttribIPointer(location,1,GL_UNSIGNED_INT,128,reinterpret_cast<const void*>(uintptr_t(jBytes+jOffsets[location])));glEnableVertexAttribArray(location);glVertexAttribDivisor(location,1);}
   glDrawArraysInstanced(GL_TRIANGLES,0,6,profile->kw);
   glBindBuffer(GL_ARRAY_BUFFER,0);
  }
  glBindVertexArray(vao_);
  success=glGetError()==GL_NO_ERROR;
 }else success=false;
 if(success&&swap&&swap(user)){token="G12_POROUS_SCENE_V1|"+std::to_string(width)+"|"+std::to_string(height);return true;}
 return false;
}
}
