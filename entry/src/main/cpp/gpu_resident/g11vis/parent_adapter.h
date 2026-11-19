#pragma once
// T639: real parent types at every boundary; no cross-parent layout casts.
#include "../acceptance_t647/contract.h"
#include "../g9dm/research_config.h"
#include "../g9spf/research_config.h"
#include "generated_contract.h"
#include <variant>
namespace grs::g11vis {
using Digest = accept647::Digest;
using Input128 = std::array<uint32_t, 128>;
using Words64 = std::array<uint32_t, 64>;
enum class ParentKind : uint32_t { R1 = 1, DM = 2 };
using ParentConfig = std::variant<g9spf::Config, g9dm::Config>;
using ParentPermit = std::variant<g9spf::Permit, g9dm::Permit>;
struct Scope {
  uint64_t run = 0, owner = 0, context = 0, inputEpoch = 0, pageSession = 0,
           permission = 0, window = 0, config = 0, surface = 0;
};
struct Span {
  size_t begin = 0, bytes = 0;
};
struct ParentLayout {
  uint32_t k = 0, eventCapacity = 0, pathCapacity = 0;
  size_t stateBytes = 0, workBytes = 0, arenaBytes = 0, rootColdBytes = 0,
         fullColdBytes = 0;
  std::array<Span, 8> state{};
  std::array<Span, 15> work{};
};
Digest HexDigest(const char *);
std::string HexDigest(const Digest &);
class ParentAdapter {
  ParentKind kind_;
  ParentConfig config_;
  ParentLayout layout_{};
  Digest physicalLayout_{}, physicalSpec_{};
  ParentAdapter(ParentKind, ParentConfig);

public:
  static std::optional<ParentAdapter> Research(ParentKind, uint32_t k,
                                               bool full);
  ParentKind Kind() const { return kind_; }
  uint32_t Index() const { return kind_ == ParentKind::R1 ? 0 : 1; }
  const ParentConfig &Configuration() const { return config_; }
  const ParentLayout &Layout() const { return layout_; }
  Digest ParameterHash() const;
  Digest PhysicalLayoutHash() const { return physicalLayout_; }
  Digest PhysicalSpecHash() const { return physicalSpec_; }
  uint32_t ParameterRevision() const;
  uint32_t Profile() const;
  bool Full() const;
  bool FillSeed(std::vector<uint32_t> &, const Scope &, Digest &actual) const;
  void RootConfig(std::array<uint32_t, 128> &, const ParentPermit &,
                  const Digest &seed) const;
  std::string Request() const;
  std::string SeedReceipt() const;
  static std::optional<ParentAdapter>
  Parse(ParentKind, const std::string &request, const std::string &seedReceipt);
};
struct CompositePermit {
  ParentPermit parent;
  uint64_t rootBeginNs = 0, visualGeneration = 0, visualTicks = 0;
  uint32_t compositeCode = 0;
  Digest visualConfigSha{}, pipelineSha{};
  bool operator==(const CompositePermit &p) const {
    return parent == p.parent && rootBeginNs == p.rootBeginNs &&
           visualGeneration == p.visualGeneration &&
           visualTicks == p.visualTicks && compositeCode == p.compositeCode &&
           visualConfigSha == p.visualConfigSha && pipelineSha == p.pipelineSha;
  }
  template <class F> decltype(auto) Visit(F &&f) const {
    return std::visit(std::forward<F>(f), parent);
  }
  uint32_t Kind() const {
    return Visit([](const auto &p) { return p.kind; });
  }
  uint64_t Nonce() const {
    return Visit([](const auto &p) { return p.nonce; });
  }
  uint64_t Quanta() const {
    return Visit([](const auto &p) { return p.quanta; });
  }
  uint64_t Deadline() const {
    return Visit([](const auto &p) { return p.deadline; });
  }
  uint32_t BaseBank() const {
    return Visit([](const auto &p) { return p.baseBank; });
  }
  uint32_t CandidateBank() const {
    return Visit([](const auto &p) { return p.candidateBank; });
  }
};
Words64 VisualConfig(const ParentAdapter &, uint32_t v, uint32_t blend);
bool ValidateVisualConfig(const Words64 &, const ParentAdapter &);
} // namespace grs::g11vis
