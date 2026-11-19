// T566 (problem T563): G11 composite scene — glyph pass with two-anchor
// quad data from the extended WORK, parent packet pass, and the mask logic
// in the shared fragment shader. No vertex SSBO; integer instance attributes.
#pragma once
#include "../../p013renderer/fullframe_targets.h"
#include "../program_cache/graphics_program_factory.h"
#include "controller.h"
#include <string>
namespace grs::g11vis {
class CompositeScene {
public:
  // Prepare the composite programs: scene (State@0 fragment SSBO), glyph
  // (uvec4 loc0/1 from WORK), packet (parent three-uint attrs + Pstyle uvec4
  // loc3 + id loc4 / generation loc5).
  bool PreparePrograms(uint64_t generation);
  bool PrepareFrame(uint32_t width, uint32_t height, uint64_t generation) {
    return width >= 128 && height > 0 && PreparePrograms(generation) &&
           targets_.Ensure(int(width), int(height), generation) ==
               p013::Status::OK;
  }
  void Reset(bool current);
  // Full composite draw: base bed/flow scene -> S/F/conversion glyphs ->
  // physical P packets -> present. Emits the composite token on success.
  bool Draw(Controller &, uint32_t width, uint32_t height, bool (*swap)(void *),
            void *user, std::string &token);

private:
  std::array<GLuint, 3> programs_{};
  std::array<UniformLocations, 3> uniforms_{};
  GLuint vao_ = 0, glyphVao_ = 0, packetVao_ = 0;
  p013::FullFrameTargets targets_;
  EGLContext context_ = EGL_NO_CONTEXT;
  uint64_t generation_ = 0, last_ = 0;
  bool Reflect(GLuint program, size_t index, uint64_t generation,
               const programcache::GraphicsProgramDesc &desc);
};
} // namespace grs::g11vis
