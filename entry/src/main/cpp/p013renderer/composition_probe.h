#pragma once
#include <GLES3/gl3.h>
#include "render_contract.h"
namespace p013 {
class CompositionProbe {
 public:
  Status Draw(const FrameInput&, uint64_t);
  void Reset(bool deleteGpu = false);
  GLuint Program() const { return program; }
  GLuint Vao() const { return vao; }
  uint64_t programAbandonCount = 0, vaoAbandonCount = 0;
  uint64_t epoch = 0, sourceHash = 0;
  GLenum error = GL_NO_ERROR;
  bool captured = false;
  FrameInput frame{};
  std::array<float, 12> points{};
  std::array<unsigned char, 24> rgba{}, linearRgba{}, srgbRgba{};
 private:
  GLuint program = 0, vao = 0;
};
}
