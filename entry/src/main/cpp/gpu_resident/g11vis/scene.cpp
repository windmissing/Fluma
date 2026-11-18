// T566 (problem T563): composite scene implementation.
#include "scene.h"
#include "scene_shaders.h"
#include <string>
namespace grs::g11vis {
namespace {
// Glyph vertex: no SSBO — two uvec4 instance attributes from the extended
// WORK glyph segment (stride 32B, divisor 1); gl_VertexID emits 6 verts.
const char* kGlyphVertex=R"G11GV(#version 310 es
precision highp int;
layout(location=0) in uvec4 aGlyph0;
layout(location=1) in uvec4 aGlyph1;
uniform vec2 uResolution;
uniform vec4 uBox;
out vec2 vCenter;
flat out uint vKindFlags;
flat out float vBlend;
flat out uint vGhostMass;
flat out uint vAppearance;
const vec2 kCorners[6]=vec2[6](vec2(0,0),vec2(1,0),vec2(1,1),vec2(0,0),vec2(1,1),vec2(0,1));
void main(){
 uint owner=aGlyph0.x&3u;
 vec2 center=vec2(0.0);
 if(owner!=0u){
  // Q24 x / Q23 y domain -> pixels; only the final projection uses float.
  vec2 oldPx=vec2(float(aGlyph0.y)/16777216.0,float(aGlyph0.z)/8388608.0);
  vec2 newPx=vec2(float(aGlyph1.x)/16777216.0,float(aGlyph1.y)/8388608.0);
  vec2 corner=kCorners[gl_VertexID];
  center=mix(oldPx,newPx,corner);
 }
 vKindFlags=aGlyph0.x&127u;
 vBlend=float(aGlyph1.z)/65536.0;
 vGhostMass=aGlyph1.w&0xffffu;
 vAppearance=aGlyph1.w>>16;
 vCenter=center*vec2(uBox.z,uBox.w)/uResolution;
 gl_Position=vec4(center*2.0-1.0,0.0,1.0);
}
)G11GV";
const char* kGlyphFragment=R"G11GF(#version 310 es
precision highp int;
layout(std430,binding=0)readonly buffer State{uint S[];};
uniform vec2 uResolution;
uniform vec4 uBox;
uniform int uN;
in vec2 vCenter;
flat in uint vKindFlags;
flat in float vBlend;
flat in uint vGhostMass;
flat in uint vAppearance;
layout(location=0) out vec4 color;
void main(){
 uint owner=vKindFlags&3u;
 if(owner==0u)discard;
 // Same CURRENT-token mask: the fragment reads the real S/F/B outline
 // (column32@512 words) and clips glyph pixels outside the true contour.
 uint column=uint(vCenter.x*128.0);
 uint at=512u+column*32u;
 uint bed=S[at+0u],flow=S[at+1u];
 if(owner==1u&&bed==0u)discard;
 if(owner==2u&&flow==0u)discard;
 float alpha=clamp(vBlend,0.0,0.5);
 vec3 shade=owner==1u?vec3(0.82,0.66,0.38):owner==2u?vec3(0.90,0.80,0.55):vec3(0.55,0.75,0.95);
 color=vec4(shade*alpha,alpha);
}
)G11GF";
}
bool CompositeScene::Reflect(GLuint program,size_t index,uint64_t generation,const programcache::GraphicsProgramDesc& desc){
 auto* record=programcache::GraphicsCurrent(program,generation);if(!record||record->abi!=desc.bindingAbi||!uniforms_[index].Prepare(program))return false;
 record->reflectBegin=programcache::Stamp();
 bool valid=uniforms_[index].Find("uResolution")>=0&&uniforms_[index].Find("uBox")>=0;
 if(index==0){
  // Base scene pass: State std430 block at binding 0 (parent ABI).
  const auto block=glGetProgramResourceIndex(program,GL_SHADER_STORAGE_BLOCK,"State");const GLenum property=GL_BUFFER_BINDING;GLint binding=-1;
  if(block!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_SHADER_STORAGE_BLOCK,block,1,&property,1,nullptr,&binding);
  valid=valid&&block!=GL_INVALID_INDEX&&binding==0;record->publicationBinding=binding;
 }else if(index==1){
  // Glyph pass: NO State block in the vertex stage is impossible to query
  // separately here; the fragment keeps State@0. Integer attributes loc0/1.
  const auto block=glGetProgramResourceIndex(program,GL_SHADER_STORAGE_BLOCK,"State");const GLenum property=GL_BUFFER_BINDING;GLint binding=-1;
  if(block!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_SHADER_STORAGE_BLOCK,block,1,&property,1,nullptr,&binding);
  valid=valid&&block!=GL_INVALID_INDEX&&binding==0;record->publicationBinding=binding;
  const GLenum properties[]={GL_TYPE,GL_ARRAY_SIZE,GL_LOCATION};
  for(GLuint location=0;location<2&&valid;++location){
   const auto input=glGetProgramResourceIndex(program,GL_PROGRAM_INPUT,std::string(location==0?"aGlyph0":"aGlyph1").c_str());
   GLint fields[3]={};if(input!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_PROGRAM_INPUT,input,3,properties,3,nullptr,fields);
   valid=valid&&input!=GL_INVALID_INDEX&&fields[0]==GL_UNSIGNED_INT_VEC4&&fields[1]==1&&fields[2]==GLint(location);
  }
 }else{
  // Packet pass (parent ABI): three uint attrs loc0/1/2, no vertex SSBO.
  const auto block=glGetProgramResourceIndex(program,GL_SHADER_STORAGE_BLOCK,"State");
  valid=valid&&block==GL_INVALID_INDEX;record->publicationBinding=-1;
  const GLenum properties[]={GL_TYPE,GL_ARRAY_SIZE,GL_LOCATION};
  for(GLuint location=0;location<3&&valid;++location){
   const auto input=glGetProgramResourceIndex(program,GL_PROGRAM_INPUT,std::string("a").append(location==0?"Mass":location==1?"XQ20":"YQ21").c_str());
   GLint fields[3]={};if(input!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_PROGRAM_INPUT,input,3,properties,3,nullptr,fields);
   valid=valid&&input!=GL_INVALID_INDEX&&fields[0]==GL_UNSIGNED_INT&&fields[1]==1&&fields[2]==GLint(location);
  }
 }
 const auto colorOutput=glGetProgramResourceIndex(program,GL_PROGRAM_OUTPUT,"color");const GLenum properties[]={GL_TYPE,GL_ARRAY_SIZE,GL_LOCATION};GLint fields[3]={};
 if(colorOutput!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_PROGRAM_OUTPUT,colorOutput,3,properties,3,nullptr,fields);
 valid=valid&&fields[0]==GL_FLOAT_VEC4&&fields[1]==1&&fields[2]==0;
 const auto error=glGetError();record->colorType=fields[0];record->colorSize=fields[1];record->colorLocation=fields[2];record->reflectEnd=programcache::Stamp();
 if(error&&!record->productError)record->productError=error;
 programcache::GraphicsReceipt(*record,valid&&!error?"G11_REFLECT_V1_OK":"G11_REFLECT_V1_FAILED");
 return valid&&!error;
}
bool CompositeScene::PreparePrograms(uint64_t generation){
 const auto context=eglGetCurrentContext();if(context==EGL_NO_CONTEXT||!generation)return false;
 if(context_!=context||generation_!=generation)Reset(false);
 if(programs_[0]&&programs_[1]&&programs_[2]&&vao_&&glyphVao_&&packetVao_)return true;
 context_=context;generation_=generation;
 const std::array<programcache::GraphicsProgramDesc,3> descriptions={{
  {"G11_SCENE_BASE",{{{GL_VERTEX_SHADER,kG11CompositeSceneVertex},{GL_FRAGMENT_SHADER,kG11CompositeSceneFragment}}},"G11_VIS_SCENE_BASE_V1|State:std430:uint[]@0|uResolution:vec2|uBox:vec4|color:vec4@0"},
  {"G11_SCENE_GLYPH",{{{GL_VERTEX_SHADER,kGlyphVertex},{GL_FRAGMENT_SHADER,kGlyphFragment}}},"G11_VIS_SCENE_GLYPH_V1|State:std430:uint[]@0|aGlyph0:uvec4@0|aGlyph1:uvec4@1|stride32:divisor1|no-vertex-SSBO|color:vec4@0"},
  {"G11_SCENE_PACKET",{{{GL_VERTEX_SHADER,kG11CompositePacketVertex},{GL_FRAGMENT_SHADER,kG11CompositePacketFragment}}},"G11_VIS_SCENE_PACKET_V1|no-vertex-SSBO|aMass:u32@0|aXQ20:u32@1|aYQ21:u32@2|stride256:divisor1|color:vec4@0"}
 }};
 glGenVertexArrays(1,&vao_);glGenVertexArrays(1,&glyphVao_);glGenVertexArrays(1,&packetVao_);
 if(!vao_||!glyphVao_||!packetVao_||glGetError()!=GL_NO_ERROR){Reset(true);return false;}
 for(size_t i=0;i<3;++i){GLenum error=0;if(!programcache::BuildGraphics(descriptions[i],programs_[i],error,generation)||!Reflect(programs_[i],i,generation,descriptions[i])||programcache::GraphicsPrepared(programs_[i],generation)!=GL_NO_ERROR){Reset(true);return false;}}
 return true;
}
void CompositeScene::Reset(bool current){
 current=current&&context_==eglGetCurrentContext();
 targets_.Reset(current);
 for(auto p:programs_)if(p){programcache::ForgetGraphics(p,generation_,context_);if(current)glDeleteProgram(p);}
 if(current){if(vao_)glDeleteVertexArrays(1,&vao_);if(glyphVao_)glDeleteVertexArrays(1,&glyphVao_);if(packetVao_)glDeleteVertexArrays(1,&packetVao_);}
 programs_={};uniforms_={};vao_=glyphVao_=packetVao_=0;context_=EGL_NO_CONTEXT;generation_=last_=0;
}
bool CompositeScene::Draw(VisualTransport& transport,uint32_t width,uint32_t height,bool(*swap)(void*),void* user,std::string& token){
 auto buffer=transport.StateBuffer(transport.CurrentBank());
 if(!buffer)return false;
 bool success=width>=128&&height>0&&PreparePrograms(transport.Resources()?0:0)&&targets_.Ensure(int(width),int(height),0)==p013::Status::OK;
 auto geometry=[&](size_t index){glUseProgram(programs_[index]);glUniform2f(uniforms_[index].Find("uResolution"),float(width),float(height));glUniform4f(uniforms_[index].Find("uBox"),0,0,float(width),float(height));};
 if(success){
  glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);
  // 1. Base bed/flow material scene: full-screen triangle with State@0.
  geometry(0);glBindVertexArray(vao_);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,buffer);glUniform1i(uniforms_[0].Find("uN"),128);glDrawArrays(GL_TRIANGLES,0,3);
  // Compute -> attribute consumer barrier before glyph/packet fetches.
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
  const auto* visual=transport.Resources();
  if(visual){
   // 2. Glyph pass: WORK glyph segment as GL_ARRAY_BUFFER, uvec4 loc0/1.
   geometry(1);glBindVertexArray(glyphVao_);glBindBuffer(GL_ARRAY_BUFFER,transport.WorkBuffer());
   const uint32_t glyphBytes=uint32_t(visual->visual[kSegGlyph]);
   const uint64_t glyphEnd=uint64_t(glyphBytes)+uint64_t(visual->v)*32u;
   if(glyphEnd<=visual->workBytes&&visual->v>0){
    const uint32_t offsets[2]={0u,16u};
    for(uint32_t location=0;location<2;++location){glVertexAttribIPointer(location,4,GL_UNSIGNED_INT,32,reinterpret_cast<const void*>(uintptr_t(glyphBytes+offsets[location])));glEnableVertexAttribArray(location);glVertexAttribDivisor(location,1);}
    glDrawArraysInstanced(GL_TRIANGLES,0,6,visual->v);
   }else success=false;
   glBindBuffer(GL_ARRAY_BUFFER,0);
  }
  glBindVertexArray(vao_);
  success=success&&glGetError()==GL_NO_ERROR;
 }else success=false;
 if(success&&swap&&swap(user)){token="G11_VIS_SCENE_V2|"+std::to_string(width)+"|"+std::to_string(height);return true;}
 return false;
}
}
