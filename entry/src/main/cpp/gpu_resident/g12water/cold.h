// T576 (problem T567): G12 cold — G12_POROUS_COLD_V1 schema, ROOT/FULL_CORE
// /FULL_TRIALS modes, two slots, single reader, TTL 60s, page <=1024U32.
#pragma once
#include "transport.h"
#include <string>
namespace grs::g12water {
class Cold {
 public:
  enum Mode : uint32_t { ROOT = 0, FULL_CORE = 1, FULL_TRIALS = 2 };
  std::string Begin(uint64_t token, uint64_t now, uint32_t mode, const Transport&);
  std::string Read(uint64_t id, uint32_t segment, uint32_t wordOffset, uint64_t now);
  std::string End(uint64_t id, uint64_t now);
  static constexpr uint32_t kSlots = 2;
  static constexpr uint64_t kTtlNs = 60000000000ull;
  static constexpr uint32_t kPageWords = 1024;
  static constexpr uint32_t kPageUtf8Bytes = 16384;
  static constexpr uint32_t kDescriptorsMax = 32;
 private:
  struct Slot { uint64_t id = 0, createdNs = 0, token = 0; bool live = false; uint32_t mode = 0, k = 0, kw = 0; };
  std::array<Slot, kSlots> slots_{};
  uint32_t next_ = 0;uint64_t sequence_ = 1;
};
}
