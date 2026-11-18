#pragma once
#include "../program_cache/sha256.h"
#include "contract_generated.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>
#include <time.h>
namespace grs::g13 {
using Config = std::array<uint32_t, 128>;
using Input = Config;
using Permit = Config;
using Completion = std::array<uint32_t, 256>;
inline uint64_t Now() {
  timespec t{};
  return clock_gettime(CLOCK_MONOTONIC, &t) == 0
             ? uint64_t(t.tv_sec) * 1000000000ull + t.tv_nsec
             : 0;
}
inline const uint64_t kBoot = Now();
inline std::atomic<uint64_t> gGeneration{0};
inline uint64_t U64(const uint32_t *p) {
  return uint64_t(p[0]) | (uint64_t(p[1]) << 32);
}
inline void Put(uint32_t *p, uint64_t v) {
  p[0] = uint32_t(v);
  p[1] = uint32_t(v >> 32);
}
inline float F32(uint32_t b) {
  float f;
  std::memcpy(&f, &b, 4);
  return f;
}
inline uint32_t Bits(float f) {
  uint32_t b;
  std::memcpy(&b, &f, 4);
  return b;
}
inline std::array<uint32_t, 8> Hash(const void *data, size_t bytes) {
  p011gpu::Sha256 s;
  s.update(data, bytes);
  std::array<uint32_t, 8> out{};
  p011gpu::shaWords(s.finish(), out.data());
  return out;
}
inline std::string Hex(const uint32_t *words) {
  const char *digits = "0123456789abcdef";
  std::string s;
  for (size_t i = 0; i < 32; i++) {
    uint8_t b = uint8_t(words[i / 4] >> (8 * (i % 4)));
    s += digits[b >> 4];
    s += digits[b & 15];
  }
  return s;
}
inline std::array<uint32_t, 8> FromHex(const std::string &s) {
  std::array<uint32_t, 8> r{};
  if (s.size() != 64)
    return r;
  for (size_t i = 0; i < 32; i++) {
    auto digit = [](char c) -> int {
      return c >= '0' && c <= '9'   ? c - '0'
             : c >= 'a' && c <= 'f' ? c - 'a' + 10
                                    : -1;
    };
    int a = digit(s[2 * i]), b = digit(s[2 * i + 1]);
    if (a < 0 || b < 0)
      return {};
    r[i / 4] |= uint32_t(a * 16 + b) << (8 * (i % 4));
  }
  return r;
}
template <size_t N>
inline bool SelfHash(const std::array<uint32_t, N> &data, size_t start) {
  auto copy = data;
  std::fill_n(copy.data() + start, 8, 0u);
  auto hash = Hash(copy.data(), N * 4);
  return std::equal(hash.begin(), hash.end(), data.begin() + start);
}
template <size_t N>
inline void SealHash(std::array<uint32_t, N> &data, size_t start) {
  std::fill_n(data.data() + start, 8, 0u);
  auto hash = Hash(data.data(), N * 4);
  std::copy(hash.begin(), hash.end(), data.begin() + start);
}
inline std::string Quote(const std::string &s) {
  std::string r = "\"";
  for (unsigned char c : s) {
    if (c == '"' || c == '\\') {
      r += '\\';
      r += char(c);
    } else if (c >= 32)
      r += char(c);
    else {
      const char *d = "0123456789abcdef";
      r += "\\u00";
      r += d[c >> 4];
      r += d[c & 15];
    }
  }
  return r + '"';
}
inline std::string Result(const char *api, const char *state,
                          uint32_t error = 0, uint64_t attempt = 0) {
  std::ostringstream o;
  o << "{\"route\":13,\"schema\":1,\"api\":" << Quote(api)
    << ",\"state\":" << Quote(state) << ",\"error\":" << error << ",\"boot\":\""
    << kBoot << "\",\"generation\":\"" << gGeneration.load()
    << "\",\"attempt\":\"" << attempt << "\"}";
  return o.str();
}
struct Identity {
  uint64_t run = 0, owner = 0, context = 0, surface = 0, geometry = 0,
           config = 0, session = 0, permission = 0, inputEpoch = 0, control = 0;
  bool operator==(const Identity &b) const {
    return run == b.run && owner == b.owner && context == b.context &&
           surface == b.surface && geometry == b.geometry &&
           config == b.config && session == b.session &&
           permission == b.permission && inputEpoch == b.inputEpoch &&
           control == b.control;
  }
};
struct Seed {
  std::string path, sha, receipt;
  uint32_t diagnostic = 0, forcedJ = 0, testHook = 0;
  bool nativeControl = false;
};
inline const Profile *FindProfile(uint32_t x, uint32_t y) {
  for (auto &p : kProfiles)
    if (p.nx == x && p.ny == y)
      return &p;
  return nullptr;
}
inline uint32_t RequiredAudits(bool source, uint32_t diagnostic) {
  if (source)
    return (1u << 0) | (1u << 1) | (1u << 2) | (1u << 13) | (1u << 14) |
           (1u << 15) | (1u << 17) | (1u << 18) | (1u << 19);
  if (diagnostic == 1)
    return (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) | (1u << 5) |
           (1u << 6) | (1u << 7) | (1u << 8) | (1u << 13) | (1u << 14) |
           (1u << 17) | (1u << 19);
  return ((1u << 20) - 1) & ~((1u << 15) | (1u << 18));
}
inline uint32_t ValidateConfig(const Config &c, bool diagnostic) {
  if (c[0] != kMagic_config || c[1] != 1 || c[2] != kMagic_profile ||
      c[3] != 128)
    return 1;
  auto p = FindProfile(c[4], c[5]);
  if (!p || (!diagnostic && p == kProfiles))
    return 5;
  for (size_t i = 84; i < 128; i++)
    if (c[i])
      return 5;
  if (c[35] || c[6] < 1 || c[6] > 3 || c[7] > 3 || c[28] > 4096 || c[19] != 1 ||
      c[31] != 1)
    return 5;
  for (auto i : {8, 9, 10, 11, 18, 21, 22, 23, 24, 25, 26, 29, 32, 33, 34})
    if (!std::isfinite(F32(c[size_t(i)])))
      return 5;
  if (F32(c[8]) != float(p->nx) * F32(c[10]) ||
      F32(c[9]) != float(p->ny) * F32(c[10]) || F32(c[9]) != 1.f ||
      F32(c[11]) != 1.f || F32(c[10]) != 1.f / float(p->ny) ||
      F32(c[29]) != 1.f)
    return 5;
  if ((F32(c[18]) == 0 && !diagnostic) ||
      (F32(c[18]) != 0 &&
       (F32(c[18]) < std::ldexp(1.f, -20) || F32(c[18]) > .125f)) ||
      std::abs(F32(c[21])) > 2 || std::abs(F32(c[32])) > .5f ||
      std::abs(F32(c[33])) > .5f || std::hypot(F32(c[24]), F32(c[25])) > 64.f ||
      c[20] > 3 || (c[20] == 2 && F32(c[22]) < 2))
    return 5;
  if (c[27] > 3 || std::abs(F32(c[26])) > 8.f || (!diagnostic && c[27] > 1) ||
      c[30] != 1)
    return 5;
  if (c[6] == 3 && (c[20] != 0 || c[24] != 0 || c[25] != 0))
    return 5;
  if (c[6] == 2 && (c[20] != 0 || c[24] != 0 || c[25] != 0))
    return 5;
  if (!diagnostic &&
      (c[6] != 1 || (c[7] != 0 && c[7] != 2) || c[20] == 3 || c[32] || c[33]))
    return 4;
  for (auto group :
       std::array<std::pair<size_t, const char *>, 3>{{{36, kHash_algorithmSHA},
                                                       {44, kHash_layoutSHA},
                                                       {52, kHash_gatesSHA}}}) {
    auto hash = FromHex(group.second);
    if (!std::equal(hash.begin(), hash.end(), c.begin() + group.first))
      return 3;
  }
  return SelfHash(c, 76) ? 0 : 3;
}
} // namespace grs::g13
