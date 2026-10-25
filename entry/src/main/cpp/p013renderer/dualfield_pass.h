#pragma once
#include "render_contract.h"
#include <GLES3/gl3.h>
#include <vector>
namespace p013 {
struct DualVisual { double radiusPx=1; std::array<double,12> colors{}; };
class DualFieldPass {
 public:
  Status Draw(const FrameInput&, GLuint, const float*, const uint32_t*, size_t, size_t, bool, const DualVisual&, uint64_t);
  void Reset(bool);
  uint64_t epoch=0;size_t capacity=0; GLenum error=0;
  GLuint bed=0, particles=0, vao=0, xyBuffer=0, metaBuffer=0;
  uint64_t bufferAllocations=0, uploadBytes=0, sourceHash=0;
};
}
