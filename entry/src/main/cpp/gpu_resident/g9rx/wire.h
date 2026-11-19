#pragma once
#include "../program_cache/sha256.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
namespace grs::g9rx {
using Words64 = std::array<uint32_t, 64>;
using Input128 = std::array<uint32_t, 128>;
using Completion256 = std::array<uint32_t, 256>;
using Digest = std::array<uint32_t, 8>;
inline uint64_t Pair(const uint32_t *p, size_t at) {
  return uint64_t(p[at]) | (uint64_t(p[at + 1]) << 32);
}
inline void Put(uint32_t *p, size_t at, uint64_t v) {
  p[at] = uint32_t(v);
  p[at + 1] = uint32_t(v >> 32);
}
inline bool Add(uint64_t a, uint64_t b, uint64_t &out) {
  if (b > UINT64_MAX - a)
    return false;
  out = a + b;
  return true;
}
inline void HashWord(p011gpu::Sha256 &h, uint32_t v) {
  uint8_t b[] = {uint8_t(v), uint8_t(v >> 8), uint8_t(v >> 16),
                 uint8_t(v >> 24)};
  h.update(b, 4);
}
inline Digest Finish(p011gpu::Sha256 &h) {
  auto b = h.finish();
  Digest d{};
  for (size_t i = 0; i < 32; ++i)
    d[i / 4] |= uint32_t(b[i]) << (8 * (i % 4));
  return d;
}
inline Digest HashWords(const uint32_t *w, size_t n) {
  p011gpu::Sha256 h;
  for (size_t i = 0; i < n; ++i)
    HashWord(h, w[i]);
  return Finish(h);
}
inline std::string Hex(const Digest &d) {
  const char *t = "0123456789abcdef";
  std::string s;
  for (size_t i = 0; i < 32; ++i) {
    uint8_t b = d[i / 4] >> (8 * (i % 4));
    s += t[b >> 4];
    s += t[b & 15];
  }
  return s;
}
inline Digest FromHex(const char *s) {
  Digest out{};
  for (size_t i = 0; i < 32; ++i) {
    auto digit = [](char c) {
      return uint32_t(c <= '9' ? c - '0' : c - 'a' + 10);
    };
    out[i / 4] |= ((digit(s[2 * i]) << 4) | digit(s[2 * i + 1]))
                  << (8 * (i % 4));
  }
  return out;
}
inline double Real(const uint32_t *w, size_t at) {
  uint64_t b = Pair(w, at);
  double v;
  std::memcpy(&v, &b, 8);
  return v;
}
inline void RealPut(uint32_t *w, size_t at, double v) {
  uint64_t b;
  std::memcpy(&b, &v, 8);
  Put(w, at, b);
}
} // namespace grs::g9rx
