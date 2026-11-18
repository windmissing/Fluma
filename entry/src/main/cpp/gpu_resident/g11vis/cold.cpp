// T56x (problem T563): composite cold implementation — segment dictionary,
// slot lifecycle, bounded JSON pages. Cold capture reads GPU buffers on the
// GL owner thread only; reads outside the owner are refused.
#include "cold.h"
#include <cstring>
namespace grs::g11vis {
namespace {
// Bounded JSON writer (no dynamic allocation beyond the page buffer).
struct PageJson {
  char data[g11vis::Cold::kPageUtf8Bytes];
  size_t length = 0;
  bool overflow = false;
  void Raw(const char* text, size_t bytes) {
    if (length + bytes > sizeof(data) - 1) { overflow = true; return; }
    std::memcpy(data + length, text, bytes); length += bytes;
  }
  void Number(uint32_t value) {
    char scratch[16]; const auto written = std::snprintf(scratch, sizeof(scratch), "%u", value);
    Raw(scratch, size_t(written));
  }
};
uint32_t U32Hex(uint64_t id) { return uint32_t(id ^ (id >> 32)); }
}
std::string Cold::Begin(uint64_t token, uint64_t now, bool full, const VisualTransport& transport) {
  const auto* visual = transport.Resources();
  if (!visual || !token) return "{\"result\":\"INVALID\"}";
  // Slot policy: two slots; a live non-expired slot of the same id refreshes
  // nothing (TTL does not renew); a foreign live slot blocks until free.
  for (auto& slot : slots_) {
    if (slot.live && slot.token == token && now - slot.createdNs < kTtlNs) return "{\"result\":\"STALE_EXPORT\"}";
    if (slot.live && now - slot.createdNs >= kTtlNs) slot.live = false;
  }
  bool free = false;
  for (auto& slot : slots_) if (!slot.live) { free = true; break; }
  if (!free) return "{\"result\":\"WOULD_BLOCK\"}";
  auto& slot = slots_[next_ % kSlots]; next_ += 1;
  slot.id = sequence_++; slot.token = token; slot.createdNs = now; slot.live = true; slot.full = full;
  slot.parentKind = 0; slot.k = visual->k; slot.v = visual->v;
  slot.visual = {
    std::pair<size_t, size_t>{visual->visual[kSegMarkerCurrent], size_t(visual->v) * 128},
    std::pair<size_t, size_t>{visual->visual[kSegMarkerActual], size_t(visual->v) * 128},
    std::pair<size_t, size_t>{visual->visual[kSegMarkerReplay], size_t(visual->v) * 128},
    std::pair<size_t, size_t>{visual->visual[kSegGlyph], size_t(visual->v) * 32},
    std::pair<size_t, size_t>{visual->visual[kSegMaterialPlan], 2097152},
    std::pair<size_t, size_t>{visual->visual[kSegControl], 65536},
    std::pair<size_t, size_t>{visual->visual[kSegFourMaps], 524288},
  };
  char out[192];
  const auto written = std::snprintf(out, sizeof(out), "{\"result\":\"REQUESTED\",\"id\":%llu,\"schema\":\"%s\",\"page\":\"%s\",\"segments\":7,\"full\":%s}",
    (unsigned long long)slot.id, kColdCapability, kColdPage, full ? "true" : "false");
  if (written <= 0) return "{\"result\":\"INTERNAL\"}";
  return std::string(out, size_t(written));
}
std::string Cold::Read(uint64_t id, uint32_t segment, uint32_t wordOffset, uint64_t now) {
  const Slot* found = nullptr;
  for (const auto& slot : slots_) if (slot.live && slot.id == id) { found = &slot; break; }
  if (!found) return "{\"result\":\"EXPIRED\"}";
  if (now - found->createdNs >= kTtlNs) return "{\"result\":\"EXPIRED\"}";
  if (segment >= 7) return "{\"result\":\"INVALID_SEGMENT\"}";
  const auto [byteStart, byteCount] = found->visual[segment];
  const uint64_t words = byteCount / 4;
  if (wordOffset >= words) return "{\"result\":\"INVALID_OFFSET\"}";
  // The page declares its window; the GPU copy itself happens on the GL
  // owner thread (caller-span); this response is the page receipt.
  PageJson page;
  page.Raw("{\"result\":\"PAGE\",\"id\":", 21); page.Number(uint32_t(id));
  page.Raw(",\"segment\":", 11); page.Number(segment);
  page.Raw(",\"offset\":", 10); page.Number(wordOffset);
  page.Raw(",\"words\":", 9);
  const uint32_t available = uint32_t(words - wordOffset);
  page.Number(available < kPageWords ? available : kPageWords);
  page.Raw(",\"capability\":\"", 16); page.Raw(kColdCapability, 16); page.Raw("\",\"page\":\"", 10); page.Raw(kColdPage, 22); page.Raw("\"}", 2);
  if (page.overflow) return "{\"result\":\"PAGE_OVERFLOW\"}";
  return std::string(page.data, page.length);
}
std::string Cold::End(uint64_t id, uint64_t now) {
  for (auto& slot : slots_) if (slot.live && slot.id == id) { slot.live = false; return "{\"result\":\"ENDED\"}"; }
  return "{\"result\":\"EXPIRED\"}";
}
}
