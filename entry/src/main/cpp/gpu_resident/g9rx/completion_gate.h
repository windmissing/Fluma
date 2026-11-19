#pragma once
#include "research_config.h"
#include <optional>
namespace grs::g9rx {
struct NativeEvidence {
  uint32_t dispatches = 0, barriers = 0;
  uint64_t fenceNs = 0;
  uint32_t gl = 0, egl = 0;
  bool fenceObserved = false;
};
class CompletionGate;
class Transport;
class FrozenExpectedView {
  const Profile &profile_;
  const Permit &permit_;
  const Input128 &input_;
  Digest seed_, shader_;
  const Transport *producer_;
  FrozenExpectedView(const Profile &profile, const Permit &permit,
                     const Input128 &input, const Digest &seed,
                     const Digest &shader, const Transport *producer)
      : profile_(profile), permit_(permit), input_(input), seed_(seed),
        shader_(shader), producer_(producer) {}
  friend class Transport;
  friend class CompletionGate;

public:
  uint32_t Word(size_t at) const;
  Digest Hash() const {
    p011gpu::Sha256 h;
    for (size_t i = 0; i < 224; ++i)
      HashWord(h, Word(i));
    return Finish(h);
  }
};
class ValidatedRecord {
  Permit permit_;
  Completion256 tail_{};
  Digest expectedSha_{};
  const Transport *producer_;
  ValidatedRecord(const Permit &p, const Completion256 &w,
                  const Digest &expected, const Transport *producer)
      : permit_(p), tail_(w), expectedSha_(expected), producer_(producer) {}
  friend class CompletionGate;

public:
  bool ProducedBy(const Transport *producer) const {
    return producer == producer_;
  }
  const Digest &ExpectedHash() const { return expectedSha_; }
  const Permit &Permission() const { return permit_; }
  const Completion256 &Words() const { return tail_; }
};
// This is a G9-neutral control proof, not a cast of a V4 outcome or tail.
class ValidatedBlockedControlRecord {
  ValidatedRecord record_;
  explicit ValidatedBlockedControlRecord(const ValidatedRecord &r)
      : record_(r) {}
  friend class CompletionGate;

public:
  const Permit &Permission() const { return record_.Permission(); }
  const Completion256 &Words() const { return record_.Words(); }
};
class CompletionGate {
  friend class Transport;
  static bool Common(const FrozenExpectedView &, const Completion256 &,
                     const NativeEvidence &);

private:
  static std::optional<ValidatedRecord> Accepted(const FrozenExpectedView &,
                                                 const Completion256 &,
                                                 const NativeEvidence &);
  static std::optional<ValidatedBlockedControlRecord>
  Blocked(const FrozenExpectedView &, const Completion256 &,
          const NativeEvidence &);
};
} // namespace grs::g9rx
