#pragma once
#include <GLES3/gl3.h>

#include "render_contract.h"
namespace p013 {
struct ShaderSources {
  const char* vertex;
  const char* fragment;
};
struct FrameUniforms {
  FrameInput frame;
  LightParams light;
};
class ShaderPass {
 public:
  Status Create(const ShaderSources&, uint64_t);
  Status Draw(const FrameUniforms&, GLuint);
  void Reset(bool);
  GLuint Program() const { return program; }
  GLuint Vao() const { return vao; }
  uint64_t epoch = 0;
  double buildMs = 0, uniformMs = 0, drawMs = 0;
  GLenum error = 0;
  uint64_t compileLogHash = 0, programAbandonCount = 0, vaoAbandonCount = 0;

 private:
  GLuint Compile(GLenum, const char*);
  GLuint program = 0, vao = 0;
  std::array<GLint, 12> locations{};
};
}  // namespace p013
