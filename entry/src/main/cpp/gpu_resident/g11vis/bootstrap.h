#pragma once
#include "bootstrap_generated.h"
#include "parent_adapter.h"
#include <GLES3/gl31.h>
#include <memory>
namespace grs::g11vis {
class VisualTransport;
// Constructed solely after the actual producer has fixed its physical
// seed/program.
class TypedParentInitialToken {
  const ParentAdapter *parent_;
  const Scope *scope_;
  const Digest *physical_;
  const Digest *pipeline_;
  uint64_t id_;
  TypedParentInitialToken(const ParentAdapter &p, const Scope &s,
                          const Digest &seed, const Digest &pipeline,
                          uint64_t id)
      : parent_(&p), scope_(&s), physical_(&seed), pipeline_(&pipeline),
        id_(id) {}
  friend class VisualTransport;
  friend class VisualBootstrap;
};
struct BootstrapTicket {
  uint64_t value = 0;
};
struct SeedPageMeta {
  uint32_t segment = 0, offset = 0, bytes = 0, total = 0;
  Digest segmentSHA{};
};
struct VisualSeedReceipt {
  std::array<uint32_t, 128> words{};
};
static_assert(sizeof(VisualSeedReceipt) == 512,
              "T660 separate permanent bootstrap POD");
class WitnessBroker {
public:
  static bool Reserve(uint64_t owner, uint64_t context, size_t bytes,
                      uint64_t &ticket);
  static bool Release(uint64_t ticket, uint64_t context, bool glRetired,
                      bool readerReleased);
};
class VisualBootstrap {
  VisualSeedReceipt receipt_{}; // sole permanent identity/segment bookkeeping
  std::unique_ptr<uint8_t[]> native_;
  size_t capacity_ = 0;
  GLuint gpu_ = 0;
  GLsync fence_ = nullptr;
  uint64_t credit_ = 0;
  bool reader_ = false, invalid_ = false;
  uint32_t maps_ = 0, rawGl_ = 0;
  bool Good();
  bool Live(const BootstrapTicket &, uint64_t now) const;
  void Pair(size_t, uint64_t);
  uint64_t Pair(size_t) const;
  friend class VisualTransport;
  bool ReserveInitial(const Scope &, const VisualResources &, uint64_t now);

public:
  enum class Result {
    EMPTY,
    IN_PROGRESS,
    RAW_READY,
    SEALED,
    WOULD_BLOCK,
    INVALID,
    EXPIRED,
    RETIRED_NOT_FREED,
    GL_FAILED
  };
  Result BeginVisualBootstrap(const TypedParentInitialToken &, const Words64 &,
                              const VisualResources &, GLuint work,
                              uint64_t now, BootstrapTicket *);
  Result PollVisualBootstrap(const BootstrapTicket &,
                             uint64_t now); // actual GL owner only
  Result ReadVisualBootstrap(const BootstrapTicket &, uint32_t segment,
                             uint32_t byteOffset, uint8_t *caller,
                             size_t capacity, SeedPageMeta *, uint64_t now);
  Result SealVisualBootstrap(const BootstrapTicket &, VisualSeedReceipt *,
                             uint64_t now);
  Result End(const BootstrapTicket &, uint64_t now);
  void Invalidate() { invalid_ = true; }
  bool Retire(bool current, bool contextActuallyRetired);
  bool Ready() const {
    return receipt_.words[2] == 4u && !credit_ && !native_ && !gpu_;
  }
  const VisualSeedReceipt &Receipt() const { return receipt_; }
  const uint8_t *Raw() const {
    return native_ ? native_.get() + 4096 : nullptr;
  }
  Digest SeedHash() const;
  std::string Manifest() const;
  BootstrapTicket Ticket() const { return {Pair(56)}; }
  uint32_t RawGl() const { return rawGl_; }
  size_t Capacity() const { return capacity_; }
};
} // namespace grs::g11vis
