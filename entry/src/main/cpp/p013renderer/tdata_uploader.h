#pragma once
#include <GLES3/gl3.h>

#include "render_contract.h"
namespace p013 {
class TDataUploader {
 public:
  Status Create(uint64_t);
  Status Upload(const float*, uint64_t);
  GLuint Texture() const { return texture; }
  void Reset(bool);
  uint64_t epoch = 0, uploadCount = 0, textureAllocCount = 0,
           textureAbandonCount = 0;
  double uploadMs = 0;
  GLenum error = 0;

 private:
  GLuint texture = 0;
};
}  // namespace p013
