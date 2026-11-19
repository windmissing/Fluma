#pragma once
#ifndef GRS_G9RX_TEST_HOOKS
#error G9RX test seams are unavailable in production builds
#endif
#include "controller.h"
namespace grs::g9rx::testseam {
// This header has no NAPI registration. T645 can exercise the same protocol,
// mapper, FIFO, freeze and transport by supplying bounded native fixtures.
inline constexpr const char *kIdentity = "GNR1_NATIVE_HARNESS_NOT_PRODUCT";
inline RxProtocolPayload &Payload(Controller &controller) {
  return controller.Protocol().payload;
}
inline RxSync &Sync(Controller &controller) {
  return controller.Protocol().sync;
}
inline bool SameRetainedBody(const Input128 &a, const Input128 &b) {
  return a[2] == b[2] && Pair(a.data(), 4) == Pair(b.data(), 4) &&
         Pair(a.data(), 12) == Pair(b.data(), 12) &&
         std::equal(a.begin() + 26, a.begin() + 34, b.begin() + 26) &&
         std::equal(a.begin() + 40, a.end(), b.begin() + 40);
}
} // namespace grs::g9rx::testseam
