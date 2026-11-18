// T566 (problem T563): G11 composite cold — G11_VIS_COLD_V1 schema,
// two slots, single builder/reader, TTL 60s, page <=1024 U32/16KiB JSON.
// The cold payload covers the parent ROOT/FULL originals plus the exact
// visual segments (three banks / glyph / plan / control / four maps) with
// the segment dictionary from the manifest.
#pragma once
#include "visual_transport.h"
#include <string>
namespace grs::g11vis {
class Cold {
 public:
  // BeginCold validates the composite token, reserves a slot (WOULD_BLOCK if
  // the other route is pinned), and captures the GPU segments. Caller-span
  // paging only; no JS-side whole-archive aggregation.
  std::string Begin(uint64_t token, uint64_t now, bool full, const VisualTransport&);
  // ReadCold pages one segment: parent state segments, or the visual segments
  // by dictionary index. Output is a bounded JSON page.
  std::string Read(uint64_t id, uint32_t segment, uint32_t wordOffset, uint64_t now);
  std::string End(uint64_t id, uint64_t now);
  static constexpr uint32_t kSlots = 2;
  static constexpr uint64_t kTtlNs = 60000000000ull;
  static constexpr uint32_t kPageWords = 1024;
  static constexpr uint32_t kPageUtf8Bytes = 16384;
 private:
  struct Slot {
    uint64_t id = 0, createdNs = 0, token = 0;
    bool live = false, full = false;
    uint32_t parentKind = 0, k = 0, v = 0;
    std::array<std::pair<size_t, size_t>, 7> visual{};  // segment byte/bytes
  };
  std::array<Slot, kSlots> slots_{};
  uint32_t next_ = 0;
  uint64_t sequence_ = 1;
};
}
