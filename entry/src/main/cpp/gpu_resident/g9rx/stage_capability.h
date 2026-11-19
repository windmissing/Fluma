#pragma once
// T564 (problem T560 §2): G9_STAGE_CAPABILITY_V2 — layered device eligibility.
// COMPUTE, SCENE_ATTR and PRESENT are separate domains; a compute-only pass
// never authorizes a visible draw. Each record keeps the raw query value, the
// query status and the required minimum, so an UNSUPPORTED_QUERY is reported
// as itself, never guessed from an uninitialized zero.
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdint>
#include <string>
namespace grs::g9rx {
struct ProfileRequirements {
  size_t stateBytes = 0, workBytes = 0, arenaBytes = 0;
};
struct QueryRecord {
  uint32_t pname = 0;
  int64_t value = 0;
  int32_t queryStatus = 0;
  int32_t required = 0;
  const char *domain = "";
};
struct StageCapabilityV2 {
  // COMPUTE_ONLY gates (transport::Create keeps exactly these).
  static constexpr size_t kMaxQueries = 32;
  QueryRecord queries[kMaxQueries]{};
  uint32_t queryCount = 0;
  bool computeEligible = false;
  const char *computeFailure = "";
  int64_t ssboBindings = 0, computeBlocks = 0, combinedBlocks = 0,
          sharedBytes = 0, invocations = 0;
  int64_t sizeX = 0, sizeY = 0, sizeZ = 0, countX = 0, maxBlockBytes = 0;
  int64_t vertexBlocksQueried = 0, fragmentBlocksQueried = 0;
  // SCENE_ATTR_V2 gates (attribute scene, no vertex SSBO).
  bool sceneEligible = false;
  const char *sceneFailure = "";
  int64_t fragmentBlocks = 0, combinedFragmentBlocks = 0, maxAttribStride = 0,
          maxVertexAttribs = 0;
  // Cold-record identity.
  uint64_t buildGeneration = 0;
  EGLContext context = EGL_NO_CONTEXT;
  uint32_t route = 0, k = 0;
  std::string Renderer() const;
};
struct SceneCapabilityV2 {
  bool eligible = false;
  const char *failure = "";
};
inline void RecordQuery(StageCapabilityV2 &c, uint32_t pname, int64_t value,
                        int32_t status, int32_t required, const char *domain) {
  if (c.queryCount < StageCapabilityV2::kMaxQueries) {
    auto &q = c.queries[c.queryCount++];
    q.pname = pname;
    q.value = value;
    q.queryStatus = status;
    q.required = required;
    q.domain = domain;
  }
}
inline int32_t QueryCapabilityI(StageCapabilityV2 &c, uint32_t pname,
                                int64_t &out, const char *domain,
                                int32_t required) {
  GLint v = 0;
  glGetIntegerv(pname, &v);
  const auto status = int32_t(glGetError());
  out = v;
  RecordQuery(c, pname, v, status, required, domain);
  return status;
}
} // namespace grs::g9rx
