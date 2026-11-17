#include "scene.h"
#include "g9spf_scene.h"
#include <string>
namespace grs::g9spf {
bool Scene::Reflect(GLuint program,size_t index,uint64_t generation,const programcache::GraphicsProgramDesc& desc){
 auto* record=programcache::GraphicsCurrent(program,generation);if(!record||record->abi!=desc.bindingAbi||!uniforms_[index].Prepare(program))return false;
 record->reflectBegin=programcache::Stamp();
 bool valid=uniforms_[index].Find("uResolution")>=0&&uniforms_[index].Find("uBox")>=0;
 if(index==0){
  // Scene pass keeps its State fragment read: std430 block bound at 0.
  const auto block=glGetProgramResourceIndex(program,GL_SHADER_STORAGE_BLOCK,"State");const GLenum property=GL_BUFFER_BINDING;GLint binding=-1;if(block!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_SHADER_STORAGE_BLOCK,block,1,&property,1,nullptr,&binding);
  valid=valid&&block!=GL_INVALID_INDEX&&binding==0&&uniforms_[index].Find("uLayer")>=0&&uniforms_[index].Find("uFrame")>=0;
  record->publicationBinding=binding;
 }else{
  // T564 (problem T560 §2): the V2 packet program declares NO vertex SSBO;
  // reflection must verify the three unsigned-integer instance attributes
  // exist at locations 0/1/2 instead of a State block.
  const auto block=glGetProgramResourceIndex(program,GL_SHADER_STORAGE_BLOCK,"State");
  valid=valid&&block==GL_INVALID_INDEX;record->publicationBinding=-1;
  const GLenum properties[]={GL_TYPE,GL_ARRAY_SIZE,GL_LOCATION};
  for(GLuint location=0;location<3&&valid;++location){
   const auto input=glGetProgramResourceIndex(program,GL_PROGRAM_INPUT,std::string("a").append(location==0?"Mass":location==1?"XQ20":"YQ21").c_str());
   GLint fields[3]={};if(input!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_PROGRAM_INPUT,input,3,properties,3,nullptr,fields);
   valid=valid&&input!=GL_INVALID_INDEX&&fields[0]==GL_UNSIGNED_INT&&fields[1]==1&&fields[2]==GLint(location);
  }
 }
 const auto color=glGetProgramResourceIndex(program,GL_PROGRAM_OUTPUT,"color");const GLenum properties[]={GL_TYPE,GL_ARRAY_SIZE,GL_LOCATION};GLint fields[3]={};if(color!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_PROGRAM_OUTPUT,color,3,properties,3,nullptr,fields);
 valid=valid&&fields[0]==GL_FLOAT_VEC4&&fields[1]==1&&fields[2]==0;const auto error=glGetError();record->colorType=fields[0];record->colorSize=fields[1];record->colorLocation=fields[2];record->reflectEnd=programcache::Stamp();if(error&&!record->productError)record->productError=error;
 programcache::GraphicsReceipt(*record,valid&&!error?"G9_REFLECT_V1_OK":"G9_REFLECT_V1_FAILED");return valid&&!error;
}
Result Scene::PrepareAttributeScene(const StageCapabilityV2& compute,SceneCapabilityV2* out){
 // T564 (problem T560 §2): layered SCENE_ATTR_V2 gate — separate from compute.
 // Requires compute eligibility already established, one fragment
 // shader-storage block for the scene pass, zero vertex SSBO for the packet
 // pass (the V2 program declares none), and a real link of the packet
 // program on this same context.
 if(!out)return Result::INVALID;
 out->eligible=false;out->failure="";
 if(!compute.computeEligible){out->failure="COMPUTE_NOT_ELIGIBLE";return Result::UNSUPPORTED;}
 StageCapabilityV2 c=compute;
 int32_t bad=0;
 bad|=QueryCapabilityI(c,GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS,c.fragmentBlocks,"SCENE_ATTR",1);
 bad|=QueryCapabilityI(c,GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS,c.combinedFragmentBlocks,"SCENE_ATTR",1);
 bad|=QueryCapabilityI(c,GL_MAX_VERTEX_ATTRIB_STRIDE,c.maxAttribStride,"SCENE_ATTR",256);
 bad|=QueryCapabilityI(c,GL_MAX_VERTEX_ATTRIBS,c.maxVertexAttribs,"SCENE_ATTR",3);
 caps_=c;
 if(bad){out->failure="UNSUPPORTED_QUERY";return Result::UNSUPPORTED;}
 if(c.fragmentBlocks<1||c.combinedFragmentBlocks<1||c.maxAttribStride<256||c.maxVertexAttribs<3||c.vertexBlocksQueried!=0&&c.vertexBlocksQueried<0){out->failure="SCENE_ATTR_LIMIT";return Result::UNSUPPORTED;}
 // Actual gate: build + reflect the V2 programs on this context.
 const auto context=eglGetCurrentContext();if(context==EGL_NO_CONTEXT){out->failure="NO_CONTEXT";return Result::CONTEXT_LOST;}
 if(!PreparePrograms(compute.buildGeneration)){out->failure="PACKET_PROGRAM_LINK";return Result::SHADER_FAILED;}
 out->eligible=true;sceneAttrReady_=true;return Result::OK;
}
bool Scene::PreparePrograms(uint64_t generation){
 const auto context=eglGetCurrentContext();if(context==EGL_NO_CONTEXT||!generation)return false;if(context_!=context||generation_!=generation)Reset(false);if(programs_[0]&&programs_[1]&&vao_&&attrVao_)return true;
 context_=context;generation_=generation;
 // T564 (problem T560 §2): the V2 packet ABI uses three unsigned-integer
 // instance attributes from the CURRENT state buffer; the program cache ABI
 // tag changes explicitly so old binaries are never reused.
 const std::array<programcache::GraphicsProgramDesc,2> descriptions={{
  {"SCENE_FULL",{{{GL_VERTEX_SHADER,kG9SceneVertex},{GL_FRAGMENT_SHADER,kG9SceneFragment}}},"G9_SPF_SCENE_V1|State:std430:uint[]@0|column32@512|S/F/p|uResolution:vec2|uBox:vec4|uLayer:int|uFrame:sampler2D|color:vec4@0"},
  {"SCENE_PARTICLES",{{{GL_VERTEX_SHADER,kG9PacketVertex},{GL_FRAGMENT_SHADER,kG9PacketFragment}}},"G9_SPF_PACKETS_ATTR_U32_V2|no-vertex-SSBO|aMass:u32@0|aXQ20:u32@1|aYQ21:u32@2|stride256:divisor1|packet64@280576:profile|gl_InstanceID|y2:Q21|uResolution:vec2|uBox:vec4|color:vec4@0"}
 }};
 glGenVertexArrays(1,&vao_);glGenVertexArrays(1,&attrVao_);
 if(!vao_||!attrVao_||glGetError()!=GL_NO_ERROR){Reset(true);return false;}
 for(size_t i=0;i<2;++i){GLenum error=0;if(!programcache::BuildGraphics(descriptions[i],programs_[i],error,generation)||!Reflect(programs_[i],i,generation,descriptions[i])||programcache::GraphicsPrepared(programs_[i],generation)!=GL_NO_ERROR){Reset(true);return false;}}
 return true;
}
void Scene::Reset(bool current){
 current=current&&context_==eglGetCurrentContext();targets_.Reset(current);
 for(auto p:programs_)if(p){programcache::ForgetGraphics(p,generation_,context_);if(current)glDeleteProgram(p);}if(current&&vao_)glDeleteVertexArrays(1,&vao_);if(current&&attrVao_)glDeleteVertexArrays(1,&attrVao_);programs_={};uniforms_={};vao_=attrVao_=0;context_=EGL_NO_CONTEXT;generation_=last_=0;sceneAttrReady_=false;
}
bool Scene::Draw(Controller& controller,uint32_t width,uint32_t height,bool(*swap)(void*),void* user,std::string& token){
 auto view=controller.AcquireScene();if(!view.valid)return false;
 bool success=!view.stale&&width>=128&&height>0&&PreparePrograms(view.identity.context)&&PrepareTargets(width,height,view.identity.context)&&controller.CanPresent(view.generation,programcache::Stamp());
 auto geometry=[&](size_t index){glUseProgram(programs_[index]);glUniform2f(uniforms_[index].Find("uResolution"),float(width),float(height));glUniform4f(uniforms_[index].Find("uBox"),0,0,float(width),float(height));};
 if(success){glBindFramebuffer(GL_FRAMEBUFFER,targets_.fbos[targets_.candidate]);glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glBindVertexArray(vao_);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,view.buffer);geometry(0);glUniform1i(uniforms_[0].Find("uLayer"),0);glDrawArrays(GL_TRIANGLES,0,3);
  // T564 (problem T560 §2): compute writes -> scene reads need the consumer
  // barriers on the same owner before any attribute fetch; the CURRENT state
  // buffer is bound directly as GL_ARRAY_BUFFER (zero copy, no new buffer,
  // no CPU map) with three glVertexAttribIPointer instance attributes.
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT|GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
  const auto* profile=controller.TransportProfile();
  const uint32_t packetByteStart=profile?uint32_t(profile->state[6].begin):280576u;
  const uint32_t stride=256;
  const uint64_t lastEnd=uint64_t(packetByteStart)+uint64_t(stride)*uint64_t(view.k)+8u;
  const uint64_t stateBytes=profile?profile->stateBytes:0;
  geometry(1);glBindVertexArray(attrVao_);glBindBuffer(GL_ARRAY_BUFFER,view.buffer);
  if(lastEnd<=stateBytes&&view.k>0){
   const uint32_t offsets[3]={0u,4u,8u};
   for(uint32_t location=0;location<3;++location){glVertexAttribIPointer(location,1,GL_UNSIGNED_INT,stride,reinterpret_cast<const void*>(uintptr_t(packetByteStart+offsets[location])));glEnableVertexAttribArray(location);glVertexAttribDivisor(location,1);}
   glDrawArraysInstanced(GL_TRIANGLES,0,6,view.k);
  }else success=false;
  glBindBuffer(GL_ARRAY_BUFFER,0);glBindVertexArray(vao_);
  success=success&&glGetError()==GL_NO_ERROR;
 }
 if(success&&controller.CanPresent(view.generation,programcache::Stamp())){geometry(0);glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,width,height);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,targets_.textures[targets_.candidate]);glUniform1i(uniforms_[0].Find("uFrame"),1);glUniform1i(uniforms_[0].Find("uLayer"),2);glDrawArrays(GL_TRIANGLES,0,3);success=glGetError()==GL_NO_ERROR&&controller.CanPresent(view.generation,programcache::Stamp())&&swap&&swap(user)&&controller.CanPresent(view.generation,programcache::Stamp());}
 else success=false;
 auto fence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();if(!fence||!controller.ReleaseScene(view.generation,fence))success=false;
 if(success){targets_.Commit();last_=view.generation;token="G9_SPF_SCENE_ATTR_V2|"+std::to_string(view.identity.run)+"|"+std::to_string(view.identity.context)+"|"+std::to_string(view.identity.surface)+"|"+std::to_string(view.generation)+"|"+std::to_string(width)+"|"+std::to_string(height);}
 return success;
}
}
