#include "tdata_uploader.h"
namespace p013 {
Status TDataUploader::Create(uint64_t generation) {
  Reset(true);
  epoch = generation;
  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 128, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  error = glGetError();
  if (error != GL_NO_ERROR || !texture) {
    Reset(true);
    return Status::GL_RESOURCE_FAILED;
  }
  textureAllocCount++;
  return Status::OK;
}
Status TDataUploader::Upload(const float* data, uint64_t) {
  const double t = NowMs();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 128, 1, GL_RGBA, GL_FLOAT, data);
  error = glGetError();
  uploadMs = NowMs() - t;
  if (error != GL_NO_ERROR) return Status::GL_RESOURCE_FAILED;
  uploadCount++;
  return Status::OK;
}
void TDataUploader::Reset(bool current) {
  if (texture && !current) textureAbandonCount++;
  if (texture && current) glDeleteTextures(1, &texture);
  texture = 0;
  epoch = 0;
}
}  // namespace p013
