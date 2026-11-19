#include "parent_adapter.h"
#include "../g9dm/dm_seed_generated.h"
#include "parent_seed_generated.h"
#include <algorithm>
#include <cstring>
#include <sstream>
namespace grs::g11vis {
Digest HexDigest(const char *s) {
  Digest out{};
  if (!s || std::strlen(s) != 64)
    return out;
  auto digit = [](char c) {
    return c >= '0' && c <= '9'   ? c - '0'
           : c >= 'a' && c <= 'f' ? c - 'a' + 10
                                  : -1;
  };
  for (size_t i = 0; i < 32; ++i) {
    const int a = digit(s[i * 2]), b = digit(s[i * 2 + 1]);
    if (a < 0 || b < 0)
      return Digest{};
    out[i / 4] |= uint32_t(a * 16 + b) << (8 * (i % 4));
  }
  return out;
}
std::string HexDigest(const Digest &d) {
  const char *t = "0123456789abcdef";
  std::string out;
  for (size_t i = 0; i < 32; ++i) {
    auto b = uint8_t(d[i / 4] >> (8 * (i % 4)));
    out += t[b >> 4];
    out += t[b & 15];
  }
  return out;
}
template <class P> ParentLayout LayoutOf(const P &p) {
  ParentLayout l;
  l.k = p.k;
  l.eventCapacity = p.eventCapacity;
  l.pathCapacity = p.pathCapacity;
  l.stateBytes = p.stateBytes;
  l.workBytes = p.workBytes;
  l.arenaBytes = p.arenaBytes;
  l.rootColdBytes = p.rootColdBytes;
  l.fullColdBytes = p.fullColdBytes;
  for (size_t i = 0; i < 8; ++i)
    l.state[i] = {p.state[i].begin, p.state[i].bytes};
  for (size_t i = 0; i < 15; ++i)
    l.work[i] = {p.work[i].begin, p.work[i].bytes};
  return l;
}
ParentAdapter::ParentAdapter(ParentKind kind, ParentConfig c)
    : kind_(kind), config_(std::move(c)) {
  if (kind_ == ParentKind::R1) {
    const auto &c = std::get<g9spf::Config>(config_);
    layout_ = LayoutOf(*g9spf::SelectProfile(c.k));
    std::array<uint32_t, 16> ascii{};
    for (size_t i = 0; i < 64; ++i)
      ascii[i / 4] |= uint32_t(g9spf::kEngineeringSha[i]) << (8 * (i % 4));
    physicalLayout_ = g9spf::HashWords(ascii.data(), 16);
    physicalSpec_ = HexDigest(g9spf::kEngineeringSha);
  } else {
    const auto &c = std::get<g9dm::Config>(config_);
    layout_ = LayoutOf(*g9dm::SelectProfile(c.k));
    physicalLayout_ = HexDigest(g9dm::kPhysicalLayoutSHA);
    physicalSpec_ = HexDigest(g9dm::kPhysicalSpecSHA);
  }
}
std::optional<ParentAdapter> ParentAdapter::Research(ParentKind kind,
                                                     uint32_t k, bool full) {
  if (k != 192 && k != 512)
    return {};
  if (kind == ParentKind::R1) {
    g9spf::Config c;
    c.k = k;
    c.g = 16 * 1048576;
    c.muStart = 32768;
    c.muStop = 16384;
    c.band = 1048576;
    c.entrainRate = 4 * 65536;
    c.shear = 65536;
    c.settleTicks = 384;
    c.revision = 1;
    c.research = c.synthetic = true;
    c.fullTrialTrace = full;
    c.parameterSha = g9spf::ParameterHash(c);
    if (!c.Valid())
      return {};
    return ParentAdapter(kind, c);
  }
  if (kind == ParentKind::DM) {
    g9dm::Config c;
    if (!g9dm::DecodeConfig(k == 192 ? g9dm::kConfig192 : g9dm::kConfig512, c))
      return {};
    c.fullTrialTrace = full;
    return ParentAdapter(kind, c);
  }
  return {};
}
Digest ParentAdapter::ParameterHash() const {
  return std::visit([](const auto &c) { return c.parameterSha; }, config_);
}
uint32_t ParentAdapter::ParameterRevision() const {
  return std::visit([](const auto &c) { return c.revision; }, config_);
}
uint32_t ParentAdapter::Profile() const {
  return kind_ == ParentKind::R1 ? g9spf::kProfileCode : g9dm::kProfileCode;
}
bool ParentAdapter::Full() const {
  return std::visit([](const auto &c) { return c.fullTrialTrace; }, config_);
}
bool ParentAdapter::FillSeed(std::vector<uint32_t> &out, const Scope &id,
                             Digest &hash) const {
  out.assign(layout_.stateBytes / 4, 0u);
  if (kind_ == ParentKind::DM) {
    if (!g9dm::FillDmTemplate(out.data(), out.size(), layout_.k))
      return false;
  } else if (!FillR1SeedRaw(out.data(), out.size(), layout_.k))
    return false;

  auto put = [&](size_t at, uint64_t v) { g9spf::Put(out.data(), at, v); };
  put(4, id.run);
  put(6, id.owner);
  put(8, id.context);
  put(14, id.inputEpoch);
  out[24] = layout_.k;
  std::visit(
      [&](const auto &c) {
        out[21] = out[23] = c.g;
        out[98] = c.muStart;
        out[99] = c.muStop;
        out[100] = c.band;
        out[101] = c.entrainRate;
        out[102] = c.shear;
        out[103] = c.settleTicks;
        std::copy(c.parameterSha.begin(), c.parameterSha.end(),
                  out.begin() + 42);
      },
      config_);
  hash = accept647::HashWords(out.data(), out.size());
  return true;
}
void ParentAdapter::RootConfig(std::array<uint32_t, 128> &w,
                               const ParentPermit &permit,
                               const Digest &seed) const {
  w = {};
  std::visit(
      [&](const auto &c) {
        w[0] = c.muStart;
        w[1] = c.muStop;
        w[2] = c.band;
        w[3] = c.entrainRate;
        w[4] = c.shear;
        w[5] = c.settleTicks;
        w[6] = c.revision;
        w[7] = c.g;
      },
      config_);
  std::visit(
      [&](const auto &p) {
        std::copy(p.parameterSha.begin(), p.parameterSha.end(), w.begin() + 16);
        std::copy(p.layoutSha.begin(), p.layoutSha.end(), w.begin() + 24);
        std::copy(p.shaderSha.begin(), p.shaderSha.end(), w.begin() + 32);
        std::copy(p.inputSha.begin(), p.inputSha.end(), w.begin() + 40);
      },
      permit);
  std::copy(seed.begin(), seed.end(), w.begin() + 48);
  if (kind_ == ParentKind::DM) {
    const auto &c = std::get<g9dm::Config>(config_);
    std::copy(c.capsule64.begin(), c.capsule64.end(), w.begin() + 64);
  }
}
std::string ParentAdapter::Request() const {
  return "{\"schema\":\"G11_PARENT_RESEARCH_V1\",\"kind\":" +
         std::to_string(uint32_t(kind_)) +
         ",\"K\":" + std::to_string(layout_.k) +
         ",\"full\":" + (Full() ? "true" : "false") + ",\"parameterSHA\":\"" +
         HexDigest(ParameterHash()) +
         "\",\"inputDomain\":\"SYNTHETIC_TEST_NATIVE_CONTROL\"}";
}
std::string ParentAdapter::SeedReceipt() const {
  Scope scope{};
  std::vector<uint32_t> w;
  Digest hash;
  if (!FillSeed(w, scope, hash))
    return {};
  return "{\"schema\":\"G11_S40_SEED_TEMPLATE_V1\",\"kind\":" +
         std::to_string(uint32_t(kind_)) +
         ",\"K\":" + std::to_string(layout_.k) + ",\"sha256\":\"" +
         HexDigest(hash) + "\"}";
}
std::optional<ParentAdapter> ParentAdapter::Parse(ParentKind kind,
                                                  const std::string &request,
                                                  const std::string &seed) {
  if (request.size() > 4096 || seed.size() > 4096)
    return {};
  for (uint32_t k : {192u, 512u})
    for (bool full : {false, true}) {
      auto p = Research(kind, k, full);
      if (p && p->Request() == request && p->SeedReceipt() == seed)
        return p;
    }
  return {};
}
Words64 VisualConfig(const ParentAdapter &p, uint32_t v, uint32_t blend) {
  Words64 w{};
  if ((v != 2048 && v != 8192) || (blend != 0 && blend != 256))
    return w;
  w = {kConfigMagic,
       2,
       p.Index() ? kCompositeDmCode : kCompositeR1Code,
       p.Profile(),
       p.Layout().k,
       v,
       32,
       8,
       65536,
       blend,
       131072,
       32768,
       1,
       1,
       0,
       1,
       1,
       2,
       2,
       2};
  auto sha = p.ParameterHash();
  std::copy(sha.begin(), sha.end(), w.begin() + 20);
  const auto design = HexDigest(
      "e6c634595f1ac5b368bd2bc6f3903b86728d3dbe3b1cd195d2c11df3906edabf");
  const auto recipe = HexDigest(kVisualRecipeSHA);
  std::copy(design.begin(), design.end(), w.begin() + 28);
  std::copy(recipe.begin(), recipe.end(), w.begin() + 36);
  return w;
}
bool ValidateVisualConfig(const Words64 &w, const ParentAdapter &p) {
  return w == VisualConfig(p, w[5], w[9]) && w[0] == kConfigMagic;
}
} // namespace grs::g11vis
