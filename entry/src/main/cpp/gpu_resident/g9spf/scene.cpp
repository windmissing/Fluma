#include "scene.h"
#include "g9spf_scene.h"
#include <string>
namespace grs::g9spf {
bool Scene::Reflect(GLuint program,size_t index,uint64_t generation,const programcache::GraphicsProgramDesc& desc){
 auto* record=programcache::GraphicsCurrent(program,generation);if(!record||record->abi!=desc.bindingAbi||!uniforms_[index].Prepare(program))return false;
 record->reflectBegin=programcache::Stamp();const auto block=glGetProgramResourceIndex(program,GL_SHADER_STORAGE_BLOCK,"State");const GLenum property=GL_BUFFER_BINDING;GLint binding=-1;if(block!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_SHADER_STORAGE_BLOCK,block,1,&property,1,nullptr,&binding);
 bool valid=block!=GL_INVALID_INDEX&&binding==0&&uniforms_[index].Find("uResolution")>=0&&uniforms_[index].Find("uBox")>=0;
 if(index==0)valid=valid&&uniforms_[index].Find("uLayer")>=0&&uniforms_[index].Find("uFrame")>=0;
 const auto color=glGetProgramResourceIndex(program,GL_PROGRAM_OUTPUT,"color");const GLenum properties[]={GL_TYPE,GL_ARRAY_SIZE,GL_LOCATION};GLint fields[3]={};if(color!=GL_INVALID_INDEX)glGetProgramResourceiv(program,GL_PROGRAM_OUTPUT,color,3,properties,3,nullptr,fields);
 valid=valid&&fields[0]==GL_FLOAT_VEC4&&fields[1]==1&&fields[2]==0;const auto error=glGetError();record->publicationBinding=binding;record->colorType=fields[0];record->colorSize=fields[1];record->colorLocation=fields[2];record->reflectEnd=programcache::Stamp();if(error&&!record->productError)record->productError=error;
 programcache::GraphicsReceipt(*record,valid&&!error?"G9_REFLECT_V1_OK":"G9_REFLECT_V1_FAILED");return valid&&!error;
}
bool Scene::PreparePrograms(uint64_t generation){
 const auto context=eglGetCurrentContext();if(context==EGL_NO_CONTEXT||!generation)return false;if(context_!=context||generation_!=generation)Reset(false);if(programs_[0]&&programs_[1]&&vao_)return true;
 context_=context;generation_=generation;
 const std::array<programcache::GraphicsProgramDesc,2> descriptions={{
  {"SCENE_FULL",{{{GL_VERTEX_SHADER,kG9SceneVertex},{GL_FRAGMENT_SHADER,kG9SceneFragment}}},"G9_SPF_SCENE_V1|State:std430:uint[]@0|column32@512|S/F/p|uResolution:vec2|uBox:vec4|uLayer:int|uFrame:sampler2D|color:vec4@0"},
  {"SCENE_PARTICLES",{{{GL_VERTEX_SHADER,kG9PacketVertex},{GL_FRAGMENT_SHADER,kG9PacketFragment}}},"G9_SPF_PACKETS_V1|State:std430:uint[]@0|packet64@70144|gl_InstanceID|y2:Q21|uResolution:vec2|uBox:vec4|color:vec4@0"}
 }};
 glGenVertexArrays(1,&vao_);if(!vao_||glGetError()!=GL_NO_ERROR){Reset(true);return false;}
 for(size_t i=0;i<2;++i){GLenum error=0;if(!programcache::BuildGraphics(descriptions[i],programs_[i],error,generation)||!Reflect(programs_[i],i,generation,descriptions[i])||programcache::GraphicsPrepared(programs_[i],generation)!=GL_NO_ERROR){Reset(true);return false;}}
 return true;
}
void Scene::Reset(bool current){
 current=current&&context_==eglGetCurrentContext();targets_.Reset(current);
 for(auto p:programs_)if(p){programcache::ForgetGraphics(p,generation_,context_);if(current)glDeleteProgram(p);}if(current&&vao_)glDeleteVertexArrays(1,&vao_);programs_={};uniforms_={};vao_=0;context_=EGL_NO_CONTEXT;generation_=last_=0;
}
bool Scene::Draw(Controller& controller,uint32_t width,uint32_t height,bool(*swap)(void*),void* user,std::string& token){
 auto view=controller.AcquireScene();if(!view.valid)return false;
 bool success=!view.stale&&width>=128&&height>0&&PreparePrograms(view.identity.context)&&PrepareTargets(width,height,view.identity.context)&&controller.CanPresent(view.generation,programcache::Stamp());
 auto geometry=[&](size_t index){glUseProgram(programs_[index]);glUniform2f(uniforms_[index].Find("uResolution"),float(width),float(height));glUniform4f(uniforms_[index].Find("uBox"),0,0,float(width),float(height));};
 if(success){glBindFramebuffer(GL_FRAMEBUFFER,targets_.fbos[targets_.candidate]);glViewport(0,0,width,height);glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glBindVertexArray(vao_);glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,view.buffer);geometry(0);glUniform1i(uniforms_[0].Find("uLayer"),0);glDrawArrays(GL_TRIANGLES,0,3);
  geometry(1);glDrawArraysInstanced(GL_TRIANGLES,0,6,view.k);success=glGetError()==GL_NO_ERROR;
 }
 if(success&&controller.CanPresent(view.generation,programcache::Stamp())){geometry(0);glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,width,height);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,targets_.textures[targets_.candidate]);glUniform1i(uniforms_[0].Find("uFrame"),1);glUniform1i(uniforms_[0].Find("uLayer"),2);glDrawArrays(GL_TRIANGLES,0,3);success=glGetError()==GL_NO_ERROR&&controller.CanPresent(view.generation,programcache::Stamp())&&swap&&swap(user)&&controller.CanPresent(view.generation,programcache::Stamp());}
 else success=false;
 auto fence=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);glFlush();if(!fence||!controller.ReleaseScene(view.generation,fence))success=false;
 if(success){targets_.Commit();last_=view.generation;token="G9_SPF_SCENE_V1|"+std::to_string(view.identity.run)+"|"+std::to_string(view.identity.context)+"|"+std::to_string(view.identity.surface)+"|"+std::to_string(view.generation)+"|"+std::to_string(width)+"|"+std::to_string(height);}
 return success;
}
}
