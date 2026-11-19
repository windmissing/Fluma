#pragma once
#include "contract_generated.h"
#include "control_types.h"
#include "schema_generated.h"
#include "wire.h"
#include <vector>
namespace grs::g9rx {
struct Config {
  uint32_t k = 0, g = 0, muStart = 0, muStop = 0, band = 0, entrainRate = 0,
           shear = 0, settleTicks = 0, revision = 1;
  bool research = false, synthetic = false, fullTrialTrace = false;
  Words64 capsule64{};
  Digest parameterSha{}, seedSha{};
  bool Valid() const {
    return research && (capsule64 == kConfig192 || capsule64 == kConfig512) &&
           k == capsule64[3] && parameterSha == HashWords(capsule64.data(), 64);
  }
};
inline Digest ParameterHash(const Config &c) {
  return HashWords(c.capsule64.data(), 64);
}
inline bool DecodeConfig(const Words64 &words, Config &c) {
  if (words != kConfig192 && words != kConfig512)
    return false;
  c.capsule64 = words;
  c.k = words[3];
  c.g = words[4];
  c.muStart = words[5];
  c.muStop = words[6];
  c.band = words[7];
  c.entrainRate = words[8];
  c.shear = words[9];
  c.settleTicks = words[10];
  c.revision = words[11];
  c.parameterSha = ParameterHash(c);
  c.research = true;
  return true;
}
struct SeedFileIdentity {
  Digest sha{};
  std::vector<uint32_t> state;
};
struct Permit {
  ControlIdentity identity;
  uint64_t nonce = 0, plan = 0, frame = 0, control = 0, geometry = 0,
           deadline = 0, baseCommit = 0, quanta = 0, arenaGeneration = 0;
  uint64_t clockGeneration = 0, lifecycleGeneration = 0, rootBeginNs = 0;
  uint32_t kind = 0, ordinal = 0, baseBank = 0, candidateBank = 1, arena = 0;
  std::array<uint32_t, 8> parameterSha{}, layoutSha{}, shaderSha{}, inputSha{};
  bool operator==(const Permit &b) const {
    return identity == b.identity && nonce == b.nonce && plan == b.plan &&
           frame == b.frame && control == b.control && geometry == b.geometry &&
           deadline == b.deadline && baseCommit == b.baseCommit &&
           quanta == b.quanta && arenaGeneration == b.arenaGeneration &&
           clockGeneration == b.clockGeneration &&
           lifecycleGeneration == b.lifecycleGeneration &&
           rootBeginNs == b.rootBeginNs && kind == b.kind &&
           ordinal == b.ordinal && baseBank == b.baseBank &&
           candidateBank == b.candidateBank && arena == b.arena &&
           parameterSha == b.parameterSha && layoutSha == b.layoutSha &&
           shaderSha == b.shaderSha && inputSha == b.inputSha;
  }
};
enum class Result {
  OK,
  IN_PROGRESS,
  WOULD_BLOCK,
  INVALID,
  UNSUPPORTED,
  STALE,
  DEADLINE,
  GL_FAILED,
  SHADER_FAILED,
  CONTEXT_LOST,
  INPUT_CLOCK_UNVERIFIED,
  CALIBRATION_CONFIG_REQUIRED,
  SOURCE_IDENTITY_MISMATCH = 0x665
};
} // namespace grs::g9rx
