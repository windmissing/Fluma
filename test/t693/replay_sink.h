#pragma once
#include "accepted_readback.h"
#include <fstream>
#include <filesystem>
#include <memory>

namespace t693 {
class Replay {
  using Root = grs::m1::t692::Root;
  using Result = grs::m1::t692::Result;
  static constexpr size_t credit = grs::m1::t692::RootCreditBytes;
  struct Entry { Root root; size_t offset = 0; };
  static constexpr size_t slots = 512;
  std::unique_ptr<uint8_t[]> bytes = std::make_unique<uint8_t[]>(credit);
  std::unique_ptr<Entry[]> records = std::make_unique<Entry[]>(slots);
  size_t used = 0, count = 0, reserved = 0;
  uint64_t rejected = 0;

  static uint8_t* reserve(void* context, const Root&, size_t length) noexcept {
    auto& out = *static_cast<Replay*>(context);
    if (out.count + 1 >= slots || length > credit - out.used ||
        (out.count + 1) * sizeof(Entry) + 1024 > credit - out.used - length) return nullptr;
    out.reserved = length;
    return out.bytes.get() + out.used;
  }
  static void finish(void* context, const Root& root) noexcept {
    auto& out = *static_cast<Replay*>(context);
    if (out.count >= slots || (out.count + 1) * sizeof(Entry) + 1024 > credit - out.used) {
      if (!out.rejected) out.rejected = root.sequence;
      return;
    }
    out.records[out.count++] = {root, out.used};
    if (root.result == Result::Ok) out.used += out.reserved;
    else if (!out.rejected) out.rejected = root.sequence;
    out.reserved = 0;
  }
  const grs::m1::t692::Sink sink{this, reserve, finish};

 public:
  bool install() { return grs::m1::t692::Install(&sink); }
  bool store(const std::string& stem) {
    std::ofstream payload(stem + ".root.bin", std::ios::binary);
    std::ofstream index(stem + ".root.ndjson", std::ios::binary);
    if (!payload || !index) return false;
    for (size_t i = 0; i < count; ++i) {
      const auto& e = records[i];
      const auto& r = e.root;
      const auto& w = r.accepted.completion;
      index << "{\"sequence\":" << r.sequence << ",\"result\":" << int(r.result)
            << ",\"startedSteadyNs\":" << r.startedNs << ",\"elapsedNs\":" << r.elapsedNs
            << ",\"offset\":" << e.offset
            << ",\"kind\":" << r.request.kind << ",\"nonceLo\":" << r.request.nonce.lo
            << ",\"nonceHi\":" << r.request.nonce.hi
            << ",\"frameLo\":" << r.request.frame.lo << ",\"frameHi\":" << r.request.frame.hi
            << ",\"controlLo\":" << r.request.control.lo << ",\"controlHi\":" << r.request.control.hi
            << ",\"runLo\":" << r.scope.run.lo << ",\"runHi\":" << r.scope.run.hi
            << ",\"bank\":" << r.accepted.bank << ",\"publication\":" << r.accepted.publication
            << ",\"eventArena\":" << w[51] << ",\"eventStart\":" << w[52] << ",\"eventEnd\":" << w[53]
            << ",\"bankBytes\":" << r.bank.bytes << ",\"eventBytes\":" << r.events.bytes
            << ",\"bankName\":" << r.bank.buffer << ",\"eventName\":" << r.events.buffer
            << ",\"bankTarget\":" << r.bank.target << ",\"eventTarget\":" << r.events.target
            << ",\"bankOffset\":" << r.bank.offset << ",\"eventOffset\":" << r.events.offset
            << ",\"bankAccess\":" << r.bank.access << ",\"eventAccess\":" << r.events.access
            << ",\"bankMapped\":" << r.bank.mapped << ",\"eventsMapped\":" << r.events.mapped
            << ",\"bankUnmapped\":" << r.bank.unmapped << ",\"eventsUnmapped\":" << r.events.unmapped
            << ",\"bankRaw\":" << r.bank.raw << ",\"eventRaw\":" << r.events.raw << "}\n";
    }
    payload.write(reinterpret_cast<const char*>(bytes.get()), used);
    std::ofstream completion(stem + ".completion.bin", std::ios::binary);
    for (size_t i = 0; i < count; ++i) {
      const auto& words = records[i].root.accepted.completion;
      completion.write(reinterpret_cast<const char*>(words.data()), 1024);
    }
    index << "{\"roots\":" << count << ",\"payloadBytes\":" << used
          << ",\"firstRejected\":" << rejected << "}\n";
    payload.flush();completion.flush();index.flush();
    std::error_code ec;
    const auto indexBytes=std::filesystem::file_size(stem + ".root.ndjson",ec);
    return index && payload && completion && !ec && used + count * 1024 + indexBytes <= credit &&
           !rejected && count;
  }
};
}
