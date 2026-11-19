// T566 (problem T563): composite scene implementation.
#include "scene.h"
#include "scene_shaders.h"
#include <string>
namespace grs::g11vis {
namespace {
// Glyph vertex: no SSBO — two uvec4 instance attributes from the extended
// WORK glyph segment (stride 32B, divisor 1); gl_VertexID emits 6 verts.
const char* kGlyphVertex=R"G11GV(#version 310 es
precision highp float;precision highp int;
layout(location=0) in uvec4 aGlyph0;
layout(location=1) in uvec4 aGlyph1;
uniform vec2 uResolution;
uniform vec4 uBox;
out vec2 vCenter;flat out vec2 vOld;flat out vec2 vNew;
flat out uint vKindFlags;
flat out float vBlend;
flat out uint vGhostMass;
flat out uint vAppearance;
const vec2 kCorners[6]=vec2[6](vec2(0,0),vec2(1,0),vec2(1,1),vec2(0,0),vec2(1,1),vec2(0,1));
void main(){
 vec2 oldPx=vec2(aGlyph0.xy)/vec2(16777216.0,8388608.0);
 vec2 newPx=vec2(aGlyph0.zw)/vec2(16777216.0,8388608.0);
 vec2 corner=kCorners[gl_VertexID];
 vec2 center=mix(min(oldPx,newPx)-vec2(0.015625),max(oldPx,newPx)+vec2(0.015625),corner);
 vKindFlags=aGlyph1.x&127u;vBlend=float(aGlyph1.y)/65536.0;vGhostMass=aGlyph1.z;vAppearance=aGlyph1.w;
 vOld=oldPx;vNew=newPx;vCenter=center;
 vec2 pixel=uBox.xy+center/vec2(128.0,256.0)*uBox.zw;
 gl_Position=vec4(pixel/uResolution*2.0-1.0,0.0,1.0);
}
)G11GV";
const char* kGlyphFragment=R"G11GF(#version 310 es
precision highp float;precision highp int;
layout(std430,binding=0)readonly buffer State{uint S[];};
uniform vec2 uResolution;
uniform vec4 uBox;
uniform int uN;
in vec2 vCenter;flat in vec2 vOld;flat in vec2 vNew;
flat in uint vKindFlags;
flat in float vBlend;
flat in uint vGhostMass;
flat in uint vAppearance;
layout(location=0) out vec4 color;
void main(){
 vec2 point=(gl_FragCoord.xy-uBox.xy)/uBox.zw*vec2(128.0,256.0);
 uint oldKind=vKindFlags&3u,newKind=(vKindFlags>>2u)&3u;
 bool oldDisk=(vKindFlags&16u)!=0u&&distance(point,vOld)<=0.015625;
 bool newDisk=(vKindFlags&32u)!=0u&&distance(point,vNew)<=0.015625;
 uint column=uint(clamp(floor(point.x),0.0,127.0));uint at=512u+column*32u;
 float bed=float(S[at])/1048576.0,flow=float(S[at+1u])/1048576.0;
 if(oldKind==1u)oldDisk=oldDisk&&point.y>=max(0.0,bed-0.125)&&point.y<bed&&flow==0.0;
 if(newKind==1u)newDisk=newDisk&&point.y>=max(0.0,bed-0.125)&&point.y<bed&&flow==0.0;
 if(oldKind==2u)oldDisk=oldDisk&&point.y>=bed&&point.y<bed+flow;
 if(newKind==2u)newDisk=newDisk&&point.y>=bed&&point.y<bed+flow;
 if(oldKind==3u&&(vKindFlags&64u)==0u)oldDisk=false;
 if(newKind==3u)newDisk=false; // true physical P body is drawn by the packet consumer.
 float alpha=min(0.5,(oldDisk?1.0-vBlend:0.0)+(newDisk?vBlend:0.0));if(alpha==0.0)discard;
 color=vec4(vec3(0.82,0.72,0.48)*alpha,alpha);
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
  // T639: the actual packet body plus Pstyle/id/generation, locations0..5.
  const auto block=glGetProgramResourceIndex(program,GL_SHADER_STORAGE_BLOCK,"State");valid=valid&&block==GL_INVALID_INDEX;record->publicationBinding=-1;
  const char* names[]={"aMass","aXQ20","aYQ21","aStyle","aId","aGeneration"};const GLenum properties[]={GL_TYPE,GL_ARRAY_SIZE,GL_LOCATION};
  for(GLuint location=0;location<6&&valid;++location){const auto input=glGetProgramResourceIndex(program,GL_PROGRAM_INPUT,names[location]);GLint fields[3]={};if(input!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_PROGRAM_INPUT,input,3,properties,3,nullptr,fields);valid=valid&&input!=GL_INVALID_INDEX&&fields[0]==GLint(location==3?GL_UNSIGNED_INT_VEC4:GL_UNSIGNED_INT)&&fields[1]==1&&fields[2]==GLint(location);}

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
 if(!transport.HasValidatedPublication())return false; // no private or untyped frame publication
 auto buffer=transport.StateBuffer(transport.CurrentBank());
 if(!buffer)return false;
 bool success=width>=128&&height>0&&PreparePrograms(transport.ContextGeneration())&&targets_.Ensure(int(width),int(height),transport.ContextGeneration())==p013::Status::OK;
 auto geometry=[&](size_t index){glUseProgram(programs_[index]);glUniform2f(uniforms_[index].Find("uResolution"),float(width),float(height));glUniform4f(uniforms_[index].Find("uBox"),0,0,float(width),float(height));};
 if(success){
  glBindFramebuffer(GL_FRAMEBUFFER,targets_.fbos[targets_.candidate]);glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);
  // 1. Base bed/flow material scene: full-screen triangle with State@0.
  geometry(0);glBindVertexArray(vao_);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,buffer);glUniform1i(uniforms_[0].Find("uN"),128);glDrawArrays(GL_TRIANGLES,0,3);
  // Compute -> attribute consumer barrier before glyph/packet fetches.
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
  const auto* visual=transport.Resources();
  if(visual){
   // 2. Glyph pass: WORK glyph segment as GL_ARRAY_BUFFER, uvec4 loc0/1.
   glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);geometry(1);glBindVertexArray(glyphVao_);glBindBuffer(GL_ARRAY_BUFFER,transport.WorkBuffer());
   const uint32_t glyphBytes=uint32_t(visual->visual[kSegGlyph]);
   const uint64_t glyphEnd=uint64_t(glyphBytes)+uint64_t(visual->v)*32u;
   if(glyphEnd<=visual->workBytes&&visual->v>0){
    const uint32_t offsets[2]={0u,16u};
    for(uint32_t location=0;location<2;++location){glVertexAttribIPointer(location,4,GL_UNSIGNED_INT,32,reinterpret_cast<const void*>(uintptr_t(glyphBytes+offsets[location])));glEnableVertexAttribArray(location);glVertexAttribDivisor(location,1);}
    glDrawArraysInstanced(GL_TRIANGLES,0,6,visual->v);
   }else success=false;
   // Real packet attributes from this exact state and style from this exact WORK.
   geometry(2);glBindVertexArray(packetVao_);glBindBuffer(GL_ARRAY_BUFFER,buffer);
   const uint32_t offsets[6]={0,4,8,0,24,28};for(uint32_t loc:{0u,1u,2u,4u,5u}){glVertexAttribIPointer(loc,1,GL_UNSIGNED_INT,256,reinterpret_cast<const void*>(transport.PacketByteStart()+offsets[loc]));glEnableVertexAttribArray(loc);glVertexAttribDivisor(loc,1);}
   glBindBuffer(GL_ARRAY_BUFFER,transport.WorkBuffer());glVertexAttribIPointer(3,4,GL_UNSIGNED_INT,16,reinterpret_cast<const void*>(visual->visual[kSegControl]+kControlPacketStyle.byteStart));glEnableVertexAttribArray(3);glVertexAttribDivisor(3,1);glDrawArraysInstanced(GL_TRIANGLES,0,6,visual->k);
   glBindBuffer(GL_ARRAY_BUFFER,0);
  }
  glBindVertexArray(vao_);
  success=success&&glGetError()==GL_NO_ERROR;
 }else success=false;
 if(success){glBindFramebuffer(GL_READ_FRAMEBUFFER,targets_.fbos[targets_.candidate]);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);glBlitFramebuffer(0,0,width,height,0,0,width,height,GL_COLOR_BUFFER_BIT,GL_NEAREST);success=glGetError()==GL_NO_ERROR;}
 // Joint accepted publication token/lease remains a real predecessor; never return dimensions as identity.
 return false;
 return false;
}
}
