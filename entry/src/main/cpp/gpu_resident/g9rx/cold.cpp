#include "cold.h"
#include <new>
namespace grs::g9rx {
std::string Cold::Begin(const Controller &controller, uint64_t replace,
                        bool full, CheckedTime time) {
  if (!time.ok)
    return "{\"state\":\"CLOCK_FAILED\"}";
  std::lock_guard<std::mutex> lock(mutex_);
  Expire(time.ns);
  for (auto &s : slots_)
    if (replace && s.request == replace) {
      if (s.building)
        return "{\"state\":\"BUSY\"}";
      s.Free();
      s = Slot{};
    }
  if (full && !controller.FullTraceEnabled())
    return "{\"state\":\"FULL_NOT_CONFIGURED\"}";
  const auto *p = controller.TransportProfile();
  if (!p)
    return "{\"state\":\"NOT_READY\"}";
  const ColdSegment *segments =
      p->k == 192 ? (full ? kCold192FULL.data() : kCold192ROOT.data())
                  : (full ? kCold512FULL.data() : kCold512ROOT.data());
  size_t bytes =
      segments[full ? 9 : 3].byteStart + segments[full ? 9 : 3].bytes;
  for (auto &s : slots_)
    if (!s.block) {
      coldbroker::Lease lease;
      uint64_t request = 0, trace = 0;
      if (!lease.Reserve(bytes, 376119296, 637, request, trace))
        return "{\"state\":\"BROKER_BUSY\"}";
      auto data =
          std::unique_ptr<uint32_t[]>(new (std::nothrow) uint32_t[bytes / 4]);
      if (!data)
        return "{\"state\":\"ALLOCATION_FAILED\"}";
      s.credit = std::move(lease);
      s.block = std::move(data);
      s.request = request;
      s.trace = trace;
      s.created = time.ns;
      s.words = bytes / 4;
      s.pending = true;
      s.full = full;
      return "{\"state\":\"QUEUED\",\"request\":\"" + std::to_string(request) +
             "\",\"trace\":\"" + std::to_string(trace) + "\"}";
    }
  return "{\"state\":\"SLOTS_FULL\"}";
}
void Cold::Build(Controller &controller, CheckedTime time) {
  if (!time.ok)
    return;
  Slot *selected = nullptr;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    Expire(time.ns);
    for (auto &s : slots_)
      if (s.pending && !s.building) {
        selected = &s;
        s.building = true;
        break;
      }
  }
  if (!selected)
    return;
  bool ok = controller.CaptureCold(
      selected->block.get(), selected->words, selected->full, selected->request,
      selected->trace, time.ns, selected->journal.data());
  std::lock_guard<std::mutex> lock(mutex_);
  selected->building = false;
  if (ok) {
    selected->credit.BuilderDone();
    selected->pending = false;
  } else if (controller.TransportProfile() == nullptr) {
    selected->Free();
    *selected = Slot{};
  }
}
std::string Cold::State(uint64_t request, CheckedTime time) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!time.ok)
    return "{\"state\":\"CLOCK_FAILED\"}";
  Expire(time.ns);
  for (auto &s : slots_)
    if (s.request == request && s.block) {
      std::string out =
          "{\"state\":\"" + std::string(s.pending ? "PENDING" : "READY") +
          "\",\"request\":\"" + std::to_string(s.request) + "\",\"trace\":\"" +
          std::to_string(s.trace) +
          "\",\"pages\":" + std::to_string((s.words + 1023) / 1024) +
          ",\"words\":" + std::to_string(s.words) +
          ",\"capability\":\"G9_DM_RX_COLD_V1\",\"journal\":[";
      if (!s.pending)
        for (size_t i = 0; i < 256; ++i) {
          if (i)
            out += ',';
          out += std::to_string(s.journal[i]);
        }
      return out + "]}";
    }
  return "{\"state\":\"NOT_FOUND\"}";
}

std::string Cold::Page(uint64_t trace, uint32_t page, CheckedTime time) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!time.ok)
    return "{\"state\":\"CLOCK_FAILED\"}";
  Expire(time.ns);
  for (auto &s : slots_)
    if (s.trace == trace && s.block && !s.pending && !s.building) {
      size_t at = size_t(page) * 1024;
      if (at >= s.words)
        return "{\"state\":\"PAGE_RANGE\"}";
      if (!s.credit.ReadBegin())
        return "{\"state\":\"READER_BUSY\"}";
      struct ReadEnd {
        coldbroker::Lease &lease;
        ~ReadEnd() { lease.ReadEnd(); }
      } readEnd{s.credit};
      size_t count = std::min<size_t>(1024, s.words - at);
      std::string out = "{\"page\":\"G9_DM_RX_SEGMENT_PAGE_V1\",\"trace\":\"" +
                        std::to_string(trace) +
                        "\",\"index\":" + std::to_string(page) + ",\"words\":[";
      for (size_t i = 0; i < count; ++i) {
        if (i)
          out += ',';
        out += std::to_string(s.block.get()[at + i]);
      }
      out += "]}";
      if (out.size() > 16384)
        return "{\"state\":\"PAGE_LIMIT\"}";
      return out;
    }
  return "{\"state\":\"NOT_READY\"}";
}
std::string Cold::End(uint64_t trace, CheckedTime time) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!time.ok)
    return "{\"state\":\"CLOCK_FAILED\"}";
  Expire(time.ns);
  for (auto &s : slots_)
    if (s.trace == trace && s.block) {
      if (s.building)
        return "{\"state\":\"BUSY\"}";
      s.Free();
      s = Slot{};
      return "{\"state\":\"ENDED\"}";
    }
  return "{\"state\":\"NOT_FOUND\"}";
}
void Cold::Reset() {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &s : slots_)
    if (!s.building) {
      s.Free();
      s = Slot{};
    }
}
} // namespace grs::g9rx
