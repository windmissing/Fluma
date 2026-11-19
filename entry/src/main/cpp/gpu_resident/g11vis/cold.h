#pragma once
#include "../cold_slot_broker.h"
#include "visual_transport.h"
namespace grs::g11vis {
// Immutable native payloads, process-wide two-slot/one-reader broker ownership.
class Cold {
public:
  static constexpr uint32_t kSlots = 2, kPageWords = 1024,
                            kPageUtf8Bytes = 16384;
  static constexpr uint64_t kTtlNs = 60000000000ull;
  std::string Begin(const std::string &expected, const std::string &actual,
                    uint64_t now, bool full, const VisualTransport &,
                    const std::array<uint32_t, 256> &quota);
  bool Pump(VisualTransport &,
            uint64_t now); // one bounded owner-GL read per call
  std::string State(uint64_t id, uint64_t now);
  std::string Read(uint64_t id, uint32_t segment, uint32_t wordOffset,
                   uint64_t now);
  std::string End(uint64_t id, uint64_t now);
  void Invalidate();
  void Maintain(uint64_t now);
  bool Building() const { return builder_ >= 0; }
  bool Empty() const;
  std::string Capabilities(const VisualTransport &) const;

private:
  enum class Stage { FREE, BUILDING, READY, RETIRING };
  struct Slot {
    coldbroker::Lease
        credit; // native block destruction/reset strictly precedes refund
    std::unique_ptr<uint32_t[]> block;
    Stage stage = Stage::FREE;
    uint64_t id = 0, request = 0, created = 0;
    Scope scope{};
    std::string token;
    size_t bytes = 0, count = 0, current = 0, done = 0;
    bool full = false, reader = false, cancel = false;
    uint32_t gl = 0;
    const char *result = "FREE";
    Digest archiveSHA{};
    std::array<GpuSegment, 17> segments{};
    std::array<size_t, 17> offsets{};
    p011gpu::Sha256 segmentHash, archiveHash;
    size_t archiveDone = 0;
  };
  std::array<Slot, 2> slots_{};
  int builder_ = -1;
  bool reading_ = false;
  void Close(size_t, const char *);
  std::string Manifest(const Slot &) const;
};
} // namespace grs::g11vis
