// T576 (problem T567): G12 water scene — CURRENT state fragment, physical P
// integer attributes, and the new J waterPacket32 integer instance VAO.
// Water optics derive free water from the same eta and real solid intervals;
// J never raises the visible water level.
#pragma once
#include "transport.h"
#include "../program_cache/graphics_program_factory.h"
#include "../../p013renderer/fullframe_targets.h"
#include <string>
namespace grs::g12water {
class Scene {
 public:
  bool PreparePrograms(uint64_t generation);
  void Reset(bool current);
  bool Draw(Transport&, uint32_t width, uint32_t height, bool (*swap)(void*), void* user, std::string& token);
 private:
  std::array<GLuint, 3> programs_{};std::array<UniformLocations, 3> uniforms_{};
  GLuint vao_ = 0, pVao_ = 0, jVao_ = 0;
  p013::FullFrameTargets targets_;
  EGLContext context_ = EGL_NO_CONTEXT;uint64_t generation_ = 0, last_ = 0;
  bool Reflect(GLuint program, size_t index, uint64_t generation, const programcache::GraphicsProgramDesc& desc);
};
}
