#pragma once
#include <cstddef>
#include <cstdint>

namespace grs::g9boot {
// Optional synchronous observer for the independent DM BOOT driver. The row
// and the two WORK words are borrowed only for the duration of the callback.
struct Capture {
  uint64_t ticket;
  void *context;
  void (*record)(void *, const char *, size_t, const uint32_t *workTail) noexcept;
};
#ifdef GRS_G9_DM_BOOT_CAPTURE
const Capture *CurrentCapture() noexcept;
#endif
} // namespace grs::g9boot
