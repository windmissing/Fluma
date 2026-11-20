#pragma once
#include "t701_variant.h"
#include "../program_cache/program_factory.h"
#include "../integer.h"
#include <thread>

namespace grs::t701 {
inline uint64_t Generation(U64 context) {
  return uint64_t(context.hi) << 32 | context.lo;
}

// Reflect the linked executable, including on a cache hit. GLSL can optimize
// optional uniforms away. The five inputs of T341 pass 0 must remain active.
template<class Uniforms>
bool ReflectFinal(Uniforms &uniforms, GLuint program) {
  const bool reflected = kCache
      ? programcache::Reflect(uniforms, program) : uniforms.Prepare(program);
  if (!reflected)
    return false;
  for (auto name : {"uPass", "uWords", "uRows", "uK", "uKw"})
    if (uniforms.Find(name) < 0)
      return false;
  for (GLuint i = 0; i < uniforms.count; ++i) {
    char name[96]{};
    GLsizei length = 0;
    GLint size = 0;
    GLenum type = 0;
    glGetActiveUniform(program, i, sizeof(name), &length, &size, &type, name);
    const GLenum expected =
        std::strcmp(name, "uRequestedAngle") == 0 ? GL_INT :
        std::strcmp(name, "uNonce") == 0 || std::strcmp(name, "uFrame") == 0 ||
        std::strcmp(name, "uControl") == 0 ? GL_UNSIGNED_INT_VEC2 : GL_UNSIGNED_INT;
    if (length <= 0 || size != 1 || type != expected || uniforms.Find(name) < 0)
      return false;
  }
  constexpr GLenum property = GL_BUFFER_BINDING;
  const char *blocks[] = {"Base", "Candidate", "Workspace", "Events"};
  for (GLint i = 0; i < 4; ++i) {
    const GLuint index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, blocks[i]);
    if (index == GL_INVALID_INDEX)
      return false;
    GLint binding = -1;
    glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, index, 1, &property,
                           1, nullptr, &binding);
    if (binding != i)
      return false;
  }
  return true;
}

// The legacy single dispatch only flushes. Obtain an export-only completion
// certificate without changing the original product warmup outcome.
template<class Current>
bool ExportFence(Current current, uint64_t start) {
  if (!current())
    return false;
  GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  glFlush();
  GLenum error = glGetError();
  GLenum wait = GL_TIMEOUT_EXPIRED;
  while (fence && !error && wait == GL_TIMEOUT_EXPIRED && current() &&
         programcache::Stamp() - start < 500000000) {
    wait = glClientWaitSync(fence, 0, 0);
    error = glGetError();
    if (wait == GL_TIMEOUT_EXPIRED && !error)
      std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
  if (fence)
    glDeleteSync(fence);
  const auto cleanup = glGetError();
  return !error && !cleanup && current() &&
         (wait == GL_ALREADY_SIGNALED || wait == GL_CONDITION_SATISFIED);
}
}
