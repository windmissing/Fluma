#pragma once
#include "wire.h"
#include <algorithm>
#include <cmath>
namespace grs::g9rx {
using U128 = unsigned __int128;
struct SpatialRecipe {
  int64_t dxQ = 0, dyQ = 0, jx = 0;
  uint64_t jy = 0;
  U128 cap = 0;
  uint32_t flags = 8;
};
inline bool Delta(uint64_t b, uint64_t maximum, int64_t &out) {
  uint32_t e = (b >> 52) & 2047;
  uint64_t fraction = b & ((1ULL << 52) - 1);
  if (e == 2047)
    return false;
  if (!e) {
    out = 0;
    return true;
  }
  int shift = int(e) - 1023 - 52 + 20;
  U128 v = (U128(1) << 52) | fraction;
  if (shift >= 0) {
    if (shift > 63)
      return false;
    v <<= shift;
  } else if (-shift >= 128)
    v = 0;
  else
    v >>= -shift;
  if (v > maximum)
    return false;
  out = int64_t(v);
  if (b >> 63)
    out = -out;
  return true;
}
inline bool Recipe(double dx, double dy, SpatialRecipe &out) {
  uint64_t xb, yb;
  std::memcpy(&xb, &dx, 8);
  std::memcpy(&yb, &dy, 8);
  if (!std::isfinite(dx) || !std::isfinite(dy) || std::abs(dx) > 128 ||
      std::abs(dy) > 256 || !Delta(xb, 128ULL * 1048576, out.dxQ) ||
      !Delta(yb, 256ULL * 1048576, out.dyQ))
    return false;
  uint64_t ax = out.dxQ < 0 ? -out.dxQ : out.dxQ,
           ay = out.dyQ < 0 ? -out.dyQ : out.dyQ;
  if (U128(ax) * ax + U128(ay) * ay < U128(262144) * 262144)
    return false;
  uint64_t rx = 2 * ax, ry = out.dyQ > 0 ? 2 * uint64_t(out.dyQ) : 0;
  out.flags = 8 | (rx > 24 * 1048576 ? 1 : 0) | (ry > 32 * 1048576 ? 2 : 0);
  uint64_t vx = std::min<uint64_t>(24 * 1048576, rx),
           vy = std::min<uint64_t>(32 * 1048576, ry);
  out.jx = int64_t(65536 * vx) * (out.dxQ < 0 ? -1 : 1);
  out.jy = 65536 * vy;
  out.cap = (U128(vx) * vx + U128(vy) * vy) << 43;
  return out.jx || out.jy;
}
inline bool Map(const uint32_t *raw, const uint32_t *mapping, double &x,
                double &y) {
  double lx = Real(raw, 12), ly = Real(raw, 14), scale = Real(raw, 24);
  for (size_t at = 12; at < 30; at += 2)
    if (!std::isfinite(Real(raw, at)))
      return false;
  if (Pair(raw, 24) != Pair(mapping, 38) ||
      Pair(raw, 26) != Pair(mapping, 40) ||
      Pair(raw, 28) != Pair(mapping, 42) || Real(raw, 26) != 0 ||
      Real(raw, 28) != 0)
    return false;
  volatile double px = lx * scale, py = ly * scale;
  double left = Real(mapping, 30), top = Real(mapping, 32),
         width = Real(mapping, 34), height = Real(mapping, 36);
  if (!std::isfinite(px) || !std::isfinite(py) || width <= 0 || height <= 0 ||
      px < left || py < top || px >= left + width || py >= top + height)
    return false;
  volatile double tx = px - left, rx = tx / width;
  x = rx * 128;
  volatile double bottom = top + height, ty = bottom - py, ry = ty / height;
  y = ry * 256;
  if (!std::isfinite(x) || !std::isfinite(y) || x < 0 || x >= 128 || y < 0 ||
      y > 256)
    return false;
  int64_t q;
  uint64_t bits;
  std::memcpy(&bits, &x, 8);
  return Delta(bits, 128ULL * 1048576, q) && q < 128LL * 1048576;
}
} // namespace grs::g9rx
