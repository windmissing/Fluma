#pragma once
#include "controller.h"
#include "../program_cache/graphics_program_factory.h"
#include "../../p013renderer/fullframe_targets.h"
namespace grs::g9spf {
class Scene {
 std::array<GLuint,2> programs_{};std::array<UniformLocations,2> uniforms_{};GLuint vao_=0;
 EGLContext context_=EGL_NO_CONTEXT;uint64_t generation_=0,last_=0;p013::FullFrameTargets targets_;
 bool Reflect(GLuint,size_t,uint64_t,const programcache::GraphicsProgramDesc&);
public:
 bool PreparePrograms(uint64_t generation);
 bool PrepareTargets(uint32_t width,uint32_t height,uint64_t generation){return targets_.Ensure(width,height,generation)==p013::Status::OK;}
 bool Draw(Controller&,uint32_t width,uint32_t height,bool(*swap)(void*),void* user,std::string& token);
 void Reset(bool current);
};
}
