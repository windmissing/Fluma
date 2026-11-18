#pragma once
#include "../../p013renderer/fullframe_targets.h"
#include "../program_cache/graphics_program_factory.h"
#include "../resource_gl.h"
#include "controller.h"
namespace grs::g13 {
class Scene {
  GLuint program_ = 0, vao_ = 0;
  programcache::GraphicsUniformTable uniforms_;
  p013::FullFrameTargets targets_;
  uint64_t viewGeneration_ = 1, presented_ = 0, context_ = 0, swapFrame_ = 0;
  std::string mapping_, fieldToken_, sceneState_ = "WAIT_ACCEPTED_FIELD";
  uint64_t staleFrames_ = 0;
  Config config_{};
  EGLContext actualContext_ = EGL_NO_CONTEXT;
  std::array<uint32_t, 64> policy_{};
  GLenum error_ = 0;

public:
  bool Prepare(uint64_t, const Config &);
  bool View(uint32_t, const std::array<uint32_t, 64> &);
  bool Draw(Controller &, int, int, const std::function<bool()> &);
  void Release(bool);
  std::string Receipt() const;
};
} // namespace grs::g13
