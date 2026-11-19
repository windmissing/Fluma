#pragma once
#include "../../p013renderer/fullframe_targets.h"
#include "../program_cache/graphics_program_factory.h"
#include "controller.h"
#include "stage_capability.h"
namespace grs::g9rx {
class Scene {
  std::array<GLuint, 2> programs_{};
  std::array<UniformLocations, 2> uniforms_{};
  GLuint vao_ = 0, attrVao_ = 0;
  EGLContext context_ = EGL_NO_CONTEXT;
  uint64_t generation_ = 0, last_ = 0;
  p013::FullFrameTargets targets_;
  StageCapabilityV2 caps_{};
  bool sceneAttrReady_ = false;
  std::atomic<uint32_t> firstGl_{0}, firstEgl_{0};
  bool Reflect(GLuint, size_t, uint64_t,
               const programcache::GraphicsProgramDesc &);

public:
  bool ObserveErrors() {
    auto gl = glGetError();
    auto egl = eglGetError();
    uint32_t zero = 0;
    if (gl)
      firstGl_.compare_exchange_strong(zero, gl);
    zero = 0;
    if (egl != EGL_SUCCESS)
      firstEgl_.compare_exchange_strong(zero, egl);
    return gl == GL_NO_ERROR && egl == EGL_SUCCESS;
  }
  uint32_t FirstGl() const { return firstGl_.load(); }
  uint32_t FirstEgl() const { return firstEgl_.load(); }
  bool PreparePrograms(uint64_t generation);
  // T564 (problem T560 §2): SCENE_ATTR_V2 gate — fragment SSBO >=1 for the
  // scene pass, zero vertex SSBO for the packet pass, plus a real link of the
  // integer-attribute packet program. Preserves the failing domain.
  Result PrepareAttributeScene(const StageCapabilityV2 &compute,
                               SceneCapabilityV2 *out);
  bool PrepareTargets(uint32_t width, uint32_t height, uint64_t generation) {
    return targets_.Ensure(width, height, generation) == p013::Status::OK;
  }
  bool Draw(Controller &, uint32_t width, uint32_t height, bool (*swap)(void *),
            void *user, std::string &token);
  void Reset(bool current);
  const StageCapabilityV2 &Capability() const { return caps_; }
};
} // namespace grs::g9rx
