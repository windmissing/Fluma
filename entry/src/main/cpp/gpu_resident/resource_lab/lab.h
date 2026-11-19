#pragma once
#include "../cold_diagnostics.h"
#include "../cold_trace.h"
#include "../program_cache/artifact_store.h"
#include "../resource_ledger.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <sys/stat.h>
#include <thread>
namespace grs::resource538 {
// T565 (problem T560 §4): RESOURCE_LAB_FILES_V2 bounds.
inline constexpr size_t kLabFilesMax = 32;
inline constexpr size_t kLabFileBytesMax = 2 * 1024 * 1024;
inline constexpr uint64_t kLabFilesTotalMax = 64 * 1024 * 1024;
inline constexpr size_t kLabChunkBytes = 4096;
inline constexpr size_t kLabReplyBytesMax = 8192;
inline constexpr size_t kLabIndexPerPage = 8;
inline constexpr uint64_t kLabExportTtlNs = 60ull * 1000000000ull;
// Only this separate module defines cache faults. No production App is linked.
class Lab {
  struct Row {
    std::array<char, 256> raw{};
  };
  static_assert(sizeof(Row) == 256);
  std::mutex mutex;
  std::condition_variable cv;
  std::thread worker;
  bool cancelled = false, released = false, running = false;
  struct Lifecycle {
    uint64_t generation = 0, terminalNs = 0, cancelRequestedNs = 0,
             cancelObservedNs = 0;
    enum Phase { RUNNING, CANCELLING, DRAINING, FINISHED } phase = FINISHED;
    std::array<char, 96> firstFailure{};
    const char *outcome = "INCOMPLETE";
    std::array<uint8_t, 32> recordsSHA{};
  } life_;
  std::atomic<bool> drainCancel_{false};
  std::shared_ptr<programcache::ProgramArtifactStore> activeStore_;
  static_assert(
      sizeof(Lifecycle) + sizeof(drainCancel_) + sizeof(activeStore_) +
              sizeof(programcache::ProgramArtifactStore::DrainCompletion) +
              sizeof(programcache::ProgramArtifactStore::DrainObservation) +
              sizeof(uint64_t) + 64 <=
          1024,
      "T640 fixed lifecycle/drain metadata");
  void Draining() {
    std::lock_guard<std::mutex> lock(mutex);
    life_.phase = Lifecycle::DRAINING;
  }
  void Failure(const char *value) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!life_.firstFailure[0])
      std::snprintf(life_.firstFailure.data(), life_.firstFailure.size(), "%s",
                    value);
  }

  uint64_t serial = 0, job = 0;
  size_t count = 0, dropped = 0;
  std::array<Row, 256> records{};
  cold503::TraceManager trace;
  cold503::Scope scope{};
  resource501::Ledger ledger;
  std::string directory;
  // T565 (problem T560 §4): private-domain paged file export state. Single
  // reader, no builder, fixed TTL, fixed catalog (32 x 256B), fixed chunk.
  static constexpr size_t kSavedNameMax = 96;
  struct SavedFileEntry {
    char name[kSavedNameMax + 1]{};
    uint64_t device = 0, inode = 0, size = 0, mtime = 0, mtimeNs = 0, ctime = 0,
             ctimeNs = 0;
    std::array<uint8_t, 32> sha{};
  };
  static_assert(sizeof(SavedFileEntry) <= 256, "catalog entry bound");
  struct ExportState {
    bool active = false;
    uint64_t job = 0, exportId = 0, startedNs = 0;
    uint32_t files = 0;
    uint64_t totalBytes = 0;
  };
  ExportState export_{};
  std::array<FILE *, kLabFilesMax> handles_{};
  std::array<SavedFileEntry, kLabFilesMax> savedFiles_{};
  uint32_t savedCount_ = 0;
  void Log(const char *stage, const char *result, uint64_t a = 0,
           uint64_t b = 0) {
    std::lock_guard<std::mutex> lock(mutex);
    if (count == records.size()) {
      ++dropped;
      return;
    }
    auto &r = records[count++];
    cold503::Writer w(r.raw.data(), r.raw.size());
    w.Raw("{\"stage\":");
    w.String(stage, std::min<size_t>(48, std::strlen(stage)));
    w.Raw(",\"result\":");
    w.String(result, std::min<size_t>(64, std::strlen(result)));
    w.Raw(",\"ns\":");
    w.U64(BridgeTimeline::Now());
    w.Raw(",\"a\":");
    w.U64(a);
    w.Raw(",\"b\":");
    w.U64(b);
    w.Raw("}");
    if (!w.Good() || w.Size() >= r.raw.size()) {
      std::strcpy(r.raw.data(), "{\"state\":\"INCOMPLETE_RECORD\"}");
      ++dropped;
    } else
      r.raw[w.Size()] = 0;
  }
  void SaveBytes(const char *name, const char *bytes, size_t size) {
    if (Stop())
      return;
    if (size > cold503::DiagnosticBytes) {
      Failure("FILE_UTF8_LIMIT");Log("FILE", "UTF8_LIMIT");
      return;
    }
    std::filesystem::create_directories(directory);
    const auto path = std::filesystem::path(directory) / name;
    // T565 (problem T560 §4): allowlist entry — only files whose complete
    // write+sync+close succeeded become export candidates, with stat identity.
    struct stat identity {};
    bool persisted = false;
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) {
      Failure("FILE_OPEN_FAILED");Log("FILE", "OPEN_FAILED", errno);
      return;
    }
    const auto wrote = std::fwrite(bytes, 1, size, f);
    const bool ok = std::fflush(f) == 0 && fsync(fileno(f)) == 0;
    const auto closed = std::fclose(f);
    persisted = ok && !closed && wrote == size;if(!persisted)Failure("FILE_WRITE_SYNC_CLOSE_FAILED");
    Log("FILE", name, wrote, persisted);
    p011gpu::Sha256 hash;
    hash.update(bytes, size);
    const auto digest = hash.finish();
    Log("FILE_SHA256", programcache::Hex(digest.data(), digest.size()).c_str());
    if (persisted && stat(path.c_str(), &identity) == 0 &&
        savedCount_ < kLabFilesMax) {
      auto &entry = savedFiles_[savedCount_++];
      const auto n = std::strlen(name);
      if (n <= kSavedNameMax) {
        std::memcpy(entry.name, name, n);
        entry.name[n] = 0;
      } else {
        std::memcpy(entry.name, name, kSavedNameMax);
        entry.name[kSavedNameMax] = 0;
      }
      entry.device = identity.st_dev;
      entry.inode = identity.st_ino;
      entry.size = identity.st_size;
      entry.mtime = identity.st_mtime;
      entry.mtimeNs = identity.st_mtim.tv_nsec;
      entry.ctime = identity.st_ctime;
      entry.ctimeNs = identity.st_ctim.tv_nsec;
      std::memcpy(entry.sha.data(), digest.data(), digest.size());
    }
  }
  void Storage(programcache::ProgramArtifactStore &store, const char *name) {
    const auto raw = store.Diagnostics502();
    SaveBytes(name, raw.data(), raw.size());
  }
  bool Stop() {
    std::lock_guard<std::mutex> lock(mutex);
    if (cancelled && !life_.cancelObservedNs)
      life_.cancelObservedNs = BridgeTimeline::Now();
    return cancelled;
  }
  void Wait() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&] { return cancelled || released; });
  }
  uint64_t Build(uint32_t k, uint32_t kw, bool source, uint64_t replace = 0,
                 const char *finish = "OK") {
    if (Stop())
      return 0;
    const size_t state = 16 * k + 16 * kw + 2304, rows = source ? 512 : 256,
                 stride = source ? 128 : 64;
    // SOURCE rows contain (2*k+1) records; Q maximum includes every dispatch.
    const size_t events = source
                              ? size_t(128) * (2 * k + 1)
                              : size_t(64) * (4 * (48 * k + 16 * kw + 254) + k);
    const auto words = 256 + state + events;
    uint64_t request = 0, id = 0;
    auto now = BridgeTimeline::Now();
    const auto *result =
        trace.Begin(scope, scope, now, replace, words, request, id, state);
    Log("TRACE_BEGIN", result, trace.Capacity(), trace.Reserved());
    if (!id)
      return 0;
    const int slot = trace.Start();
    if (slot < 0)
      return 0;
    auto old = trace.TakeOld(slot);
    old.reset();
    trace.OldFreed(slot);
    std::unique_ptr<uint32_t[]> block(new (std::nothrow) uint32_t[words]);
    trace.Adopt(slot, std::move(block));
    auto &s = trace.slots[slot];
    if (s.block) {
      for (size_t i = 0; i < words; ++i) {
        if (i % 4096 == 0 && Stop()) {
          trace.Finish(slot, scope, BridgeTimeline::Now(), "CANCELLED", 0, 0);
          return 0;
        }
        s.block[i] = uint32_t(i * 2654435761u);
      }
      auto *c = s.block.get();
      c[0] = 1381192263;
      c[1] = 3;
      c[2] = 256;
      c[3] = 3235823838u;
      c[4] = 1;
      c[5] = 0;
      c[6] = uint32_t(scope.control.run);
      c[7] = 0;
      c[10] = uint32_t(scope.control.context);
      c[11] = 0;
      c[12] = uint32_t(scope.control.surface);
      c[13] = 0;
      c[27] = source ? 1 : 2;
      c[source ? 169 : 53] = uint32_t(events / stride);
    }
    trace.Finish(slot, scope, BridgeTimeline::Now(),
                 s.block ? finish : "ALLOCATION_FAILED", state, events);
    ledger.HostStorage("LAB_TRACE_SLOT", uintptr_t(&s), words * 4,
                       s.capacity * 4);
    Log("TRACE_FINISH", s.result, trace.Capacity(), words * 4);
    if (s.block) {
      p011gpu::Sha256 sha;
      for (size_t at = 0; at < words; at += 4096) {
        if (Stop())
          return id;
        sha.update(s.block.get() + at, std::min<size_t>(4096, words - at) * 4);
      }
      const auto digest = sha.finish();
      const auto hex = programcache::Hex(digest.data(), digest.size());
      Log("FIXTURE_SHA256", hex.c_str());
      Log("FIXTURE_RECIPE", "i*2654435761u; header T445 V3 synthetic", k, kw);
    }
    return id;
  }
  void Pages(uint64_t id) {
    cold503::TraceManager::Read read;
    const auto *result =
        trace.Acquire(id, 0, BridgeTimeline::Now(), scope, read);
    Log("PAGE_ACQUIRE", result);
    if (std::strcmp(result, "OK"))
      return;
    std::array<char, cold503::PageBytes> bytes{};
    const auto pages =
        (256 + read.stateWords + read.eventWords + cold503::PageWords - 1) /
        cold503::PageWords;
    size_t maxBytes = 0;
    uint64_t bad = 0;
    p011gpu::Sha256 digest;
    for (size_t page = 0; page <= pages && !Stop(); ++page) {
      cold503::Writer w(bytes.data(), bytes.size());
      cold503::TracePage(w, read, uint32_t(page));
      maxBytes = std::max(maxBytes, w.Size());
      if (!w.Good())
        ++bad;
      digest.update(w.Data(), w.Size());
    }
    const auto sha = digest.finish();
    Log("PAGE_WRITER", bad ? "UTF8_LIMIT" : "ENCODED_SYNTHETIC", maxBytes,
        pages);
    Log("PAGE_SHA_PREFIX", "SHA256",
        uint64_t(sha[0]) << 24 | uint64_t(sha[1]) << 16 |
            uint64_t(sha[2]) << 8 | sha[3]);
    trace.Release(read, BridgeTimeline::Now(), scope);
  }
  void TraceCases(const std::string &name) {
    if (name == "TRACE_MAX_SOURCE" || name == "TRACE_MAX_Q") {
      for (auto k : {192u, 512u})
        for (auto kw : {64u, 128u}) {
          if (Stop())
            return;
          auto id = Build(k, kw, name == "TRACE_MAX_SOURCE");
          Pages(id);
          trace.End(id, BridgeTimeline::Now(), scope);
        }
      return;
    }
    auto id = Build(512, 128, false);
    if (name == "TWO_SLOT_PIN_REPLACE") {
      Build(512, 128, false, id, "INJECTED_BUILD_FAILED");
      Log("FAILED_REPLACE_PRESERVES_PUBLISHED",
          trace.published >= 0 ? trace.slots[trace.published].result : "NONE",
          trace.Capacity());
      cold503::TraceManager::Read pinned;
      std::thread reader([&] {
        const auto *r =
            trace.Acquire(id, 0, BridgeTimeline::Now(), scope, pinned);
        Log("READER_BARRIER", r, trace.Capacity());
        cv.notify_all();
        Wait();
      });
      struct ReaderCleanup {
        Lab &lab;
        std::thread &thread;
        cold503::TraceManager::Read &read;
        ~ReaderCleanup() {
          lab.Release();
          if (thread.joinable())
            thread.join();
          if (read.slot >= 0)
            lab.trace.Release(read, BridgeTimeline::Now(), lab.scope);
        }
      } cleanup{*this, reader, pinned};
      // Publish reader pin before entering concurrent control operation.
      {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&] {
          return cancelled ||
                 (count &&
                  std::strstr(records[count - 1].raw.data(), "READER_BARRIER"));
        });
      }
      auto newer = Build(512, 128, false, id);
      auto third = Build(512, 128, false, newer);
      Log("SECOND_REPLACE", third ? "UNEXPECTED_ID" : "WOULD_BLOCK",
          trace.Capacity());
      trace.End(newer, BridgeTimeline::Now(), scope);
      Log("END_PINNED", "RETIRED_READER_STILL_OWNS", trace.Capacity());
      reader.join();
      if (pinned.slot >= 0) {
        trace.Release(pinned, BridgeTimeline::Now(), scope);
        pinned.slot = -1;
      }
      Log("READER_RELEASE", "FREE_RETAINS_CAPACITY", trace.Capacity());
      return;
    }
    if (name == "TTL_WALL") {
      const auto start = trace.slots[trace.published].created;
      while (!Stop()) {
        const auto now = BridgeTimeline::Now();
        trace.Maintain(now, scope);
        cold503::TraceManager::Read read;
        const auto *observed = trace.Acquire(id, 0, now, scope, read);
        Log("TTL_POLL_NO_RENEW", observed, now - start);
        if (read.slot >= 0)
          trace.Release(read, now, scope);
        if (now - start >= cold503::TtlNs) {
          Log("TTL_WALL", "EXPIRED", now - start, trace.Capacity());
          break;
        }
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait_for(lock, std::chrono::seconds(1), [&] { return cancelled; });
      }
      Pages(id);
      return;
    }
    const auto created = trace.slots[trace.published].created;
    trace.Maintain(created + cold503::TtlNs - 1, scope);
    Log("INJECTED_CLOCK", "PRE_EDGE", trace.published >= 0);
    trace.Maintain(created + cold503::TtlNs, scope);
    Log("INJECTED_CLOCK", "AT_EDGE", trace.published >= 0);
    trace.requestSerial = UINT64_MAX;
    uint64_t req = 0, next = 0;
    Log("ID_LIMIT", trace.Begin(scope, scope, created + cold503::TtlNs, 0, 1024,
                                req, next, 256));
    trace.requestSerial = 0;
    auto reverse = Build(192, 64, true);
    if (reverse) {
      auto &s = trace.slots[trace.published];
      trace.Maintain(s.created - 1, scope);
      Log("CLOCK_REVERSE", s.result);
    }
  }
  void Diagnostic(const std::string &name) {
    cold503::Gate gate;
    Log("DIAGNOSTIC_GATE", gate.Begin(scope, scope, cold503::DiagnosticBytes));
    Log("DIAGNOSTIC_GATE_BUSY",
        gate.Begin(scope, scope, cold503::DiagnosticBytes));
    auto capture = std::unique_ptr<diagnostic492::Capture>(
        new (std::nothrow) diagnostic492::Capture);
    if (Stop()) {
      gate.End();
      return;
    }
    if (!capture) {
      Failure("CAPTURE_ALLOCATION_FAILED");Log("CAPTURE", "ALLOCATION_FAILED");
      gate.End();
      return;
    }
    for (size_t g = 0; g < 4; ++g) {
      if (Stop()) {
        gate.End();
        return;
      }
      auto s = scope.control;
      s.run += g;
      diagnostic492::Root root;
      root.scope = s;
      root.nonce = g + 1;
      root.kind = 2;
      capture->Intent(root);
      diagnostic492::Point p;
      p.scope = s;
      p.root = g + 1;
      std::strcpy(p.stage.data(), "FIXTURE");
      capture->Fault(p, "FIXTURE_FIRST_CAUSE", root);
      auto &group = capture->groups[g];
      group.count = 64;
      group.quantumCount = 64;
      group.tailCount = 64;
      for (auto &point : group.first)
        point = p;
      for (auto &point : group.firstQuantum)
        point = p;
      for (auto &point : group.tail)
        point = p;
    }
    ++gate.copied;
    Log("DIAGNOSTIC_COPY", "SINGLE_CAPTURE", gate.copied);
    cold503::SmallDiagnostic summary;
    summary.Copy(*capture, scope);
    auto bytes = std::unique_ptr<char[]>(new char[cold503::DiagnosticBytes]);
    cold503::Writer w(bytes.get(), cold503::DiagnosticBytes);
    cold503::DiagnosticJson(w, *capture, summary, cold503::DiagnosticBytes,
                            false);
    const auto size = w.Size();
    bytes[size < cold503::DiagnosticBytes ? size
                                          : cold503::DiagnosticBytes - 1] = 0;
    Log("DIAGNOSTIC_WRITER", "ACTUAL_BYTES", size, sizeof(*capture));
    SaveBytes("diagnostic-complete.json", w.Data(), w.Size());
    for (auto limit : {size, size ? size - 1 : 0, cold503::DiagnosticBytes}) {
      if (Stop()) {
        gate.End();
        return;
      }
      w.Reset();
      cold503::Writer edge(bytes.get(), limit);
      cold503::DiagnosticJson(edge, *capture, summary, limit, false);
      if (!edge.Good()) {
        cold503::Writer fallback(bytes.get(), cold503::PageBytes);
        cold503::DiagnosticFallback(fallback, summary, "INJECTED_CREDIT",
                                    limit);
        Log("FALLBACK", fallback.Good() ? "COMPLETE_JSON" : "FAILED",
            fallback.Size(), limit);
      } else
        Log("CREDIT",
            std::string(edge.Data(), std::min<size_t>(edge.Size(), 256))
                        .find("INCOMPLETE") != std::string::npos
                ? "INCOMPLETE"
                : "COMPLETE",
            edge.Size(), limit);
    }
    if (Stop()) {
      gate.End();
      return;
    }
    for (auto &group : capture->groups) {
      group.count = 768;
      group.quantumCount = 768;
      for (auto *points : {&group.first, &group.firstQuantum})
        for (auto &p : *points)
          std::memset(p.stage.data(), 1, p.stage.size() - 1);
    }
    summary.Copy(*capture, scope);
    cold503::Writer worst(
        bytes.get(),
        std::min<size_t>(cold503::DiagnosticBytes, 3 * 1024 * 1024));
    cold503::DiagnosticJson(worst, *capture, summary, worst.Capacity(), false);
    bytes[worst.Size() < cold503::DiagnosticBytes
              ? worst.Size()
              : cold503::DiagnosticBytes - 1] = 0;
    SaveBytes("diagnostic-worst-escape.json", worst.Data(), worst.Size());
    Log("WORST_ESCAPE_CLAMP",
        std::strstr(bytes.get(), "INCOMPLETE") ? "FALLBACK" : "COMPLETE",
        worst.Size(), worst.Capacity());
    if (name == "DIAGNOSTIC_BUSY_FAIL")
      for (const auto *fault : {"ALLOCATION_FAILED", "NAPI_FAILED", "IO_FAILED",
                                "CANCELLED", "FINAL_MANIFEST_FAILED"}) {
        if (Stop())
          break;
        cold503::Writer small(bytes.get(), cold503::PageBytes);
        cold503::DiagnosticFallback(small, summary, fault, cold503::PageBytes);
        Log("INJECTED_FAILURE", fault, small.Size(), small.Good());
        SaveBytes((std::string(fault) + ".json").c_str(), small.Data(),
                  small.Size());
      }
    gate.End();
  }
  bool Drain(programcache::ProgramArtifactStore &store) {
    programcache::ProgramArtifactStore::DrainObservation observed;
    const auto result = store.DrainForTest(std::chrono::steady_clock::now() +
                                               std::chrono::seconds(5),
                                           drainCancel_, &observed);
    const char *name =
        result == programcache::ProgramArtifactStore::DrainResult::DRAINED
            ? "DRAINED"
        : result == programcache::ProgramArtifactStore::DrainResult::CANCELLED
            ? "CANCELLED"
        : result == programcache::ProgramArtifactStore::DrainResult::TIMEOUT
            ? "TIMEOUT"
            : "STOPPED";
    Log("DRAIN", name, observed.queuedAtEntry, observed.fenceSequence);
    Log("DRAIN_ENTER_INSERT", "NATIVE_STEADY_NS", observed.enterNs,
        observed.insertNs);
    Log("DRAIN_POP_RETURN", "NATIVE_STEADY_NS", observed.popNs,
        observed.returnNs);
    if (result != programcache::ProgramArtifactStore::DrainResult::DRAINED) {
      Draining();
      if (result != programcache::ProgramArtifactStore::DrainResult::CANCELLED)
        Failure(name);
      return false;
    }
    return !Stop();
  }
  void Cache() {
    auto owner = std::make_shared<programcache::ProgramArtifactStore>();
    auto &store = *owner;
    {
      std::lock_guard<std::mutex> lock(mutex);
      activeStore_ = owner;
    }
    struct Retire {
      Lab &lab;
      std::shared_ptr<programcache::ProgramArtifactStore> &owner;
      ~Retire() {
        lab.Draining();
        owner->RetireForTest();
        {
          std::lock_guard<std::mutex> lock(lab.mutex);
          lab.activeStore_.reset();
        }
        owner.reset();
      }
    } retire{*this, owner};
    if (!store.Configure(directory)) {
      Failure("CACHE_CONFIGURE_FAILED");Log("CACHE_CONFIGURE", "FAILED", errno);
      return;
    }
    const std::string key(64, 'a');
    if (Stop())
      return;
    store.SetWriteFault(1);
    auto a = store.Put(key, 1, std::vector<uint8_t>(4096, 37));
    if (!Drain(store))
      return;
    Log("CACHE_ENOSPC",
        a && a->state == programcache::WRITE_FAILED ? "WRITE_FAILED"
                                                    : "OBSERVED_OTHER",
        store.Count());
    Storage(store, "cache-enospc.json");
    a.reset();
    store.Reject(key);
    if (!Drain(store))
      return;
    if (Stop())
      return;
    store.SetWriteFault(3);
    store.SetRemoveFault(true);
    auto b = store.Put(std::string(64, 'b'), 1, std::vector<uint8_t>(4096, 42));
    if (!Drain(store))
      return;
    Log("CACHE_TMP", "SYNC_REMOVE_INJECTED", store.Count());
    Storage(store, "cache-temp-remove.json");
    if (Stop())
      return;
    store.SetWriteFault(0);
    store.SetRemoveFault(false);
    store.Put(std::string(64, 'c'), 1, std::vector<uint8_t>(4096, 43));
    if (!Drain(store))
      return;
    Log("CACHE_RECOVER", "DRAINED", store.Count());
    Storage(store, "cache-recovered.json");
    std::array<std::shared_ptr<programcache::Artifact>, 32> pins;
    for (unsigned i = 0; i < 32 && !Stop(); ++i) {
      auto k = programcache::Hash(std::to_string(i));
      pins[i] = store.Put(k, 1, std::vector<uint8_t>(4096, uint8_t(i)));
    }
    if (!Drain(store))
      return;
    Log("CACHE_PIN_BOUNDARY",
        store.Put(programcache::Hash("overflow"), 1,
                  std::vector<uint8_t>(4096, 1))
            ? "ADMITTED"
            : "PIN_REJECTED",
        store.Count());
    for (auto &pin : pins)
      pin.reset();
    b.reset();
    if (!Drain(store))
      return;
    const auto cache =
                   std::filesystem::path(directory) / "gpu-program-cache/v1",
               moved = std::filesystem::path(directory) / "lab-scan-hidden";
    std::error_code ec;
    std::filesystem::rename(cache, moved, ec);
    if (!ec) {
      store.Put(programcache::Hash("scan"), 1, std::vector<uint8_t>(4096, 8));
      if (!Drain(store))
        return;
      Log("CACHE_SCAN", "DIRECTORY_UNAVAILABLE");
      Storage(store, "cache-scan-failed.json");
      std::filesystem::rename(moved, cache, ec);
      Log("CACHE_SCAN_RESTORE", ec ? "FAILED" : "RESTORED", ec.value());
    }
    {
      std::lock_guard<std::mutex> lock(mutex);
      released = false;
    }
    store.Enqueue([this] {
      Log("CACHE_WORKER_BARRIER", "BARRIER_ENTER");
      cv.notify_all();
      Wait();
    });
    {
      std::unique_lock<std::mutex> lock(mutex);
      cv.wait(lock, [&] {
        return cancelled || (count && std::strstr(records[count - 1].raw.data(),
                                                  "CACHE_WORKER_BARRIER"));
      });
    }
    unsigned queued = 0;
    try {
      for (unsigned i = 0; i < 129 && !Stop(); ++i) {
        store.Enqueue([] {});
        ++queued;
      }
    } catch (const std::exception &e) {
      Log("CACHE_QUEUE", e.what(), queued);
    }
    Release();
    if (!Drain(store))
      return;
    Storage(store, "cache-queue-boundary.json");
    std::array<std::shared_ptr<programcache::Artifact>, 4> large;
    for (unsigned i = 0; i < 4 && !Stop(); ++i) {
      large[i] = store.Put(programcache::Hash("large" + std::to_string(i)), 1,
                           std::vector<uint8_t>(programcache::kEntryLimit, 9));
      if (!Drain(store))
        return;
      Log("CACHE_32M_ENTRY", large[i] ? "ADMITTED" : "REJECTED", store.Count(),
          programcache::kEntryLimit);
    }
    auto overflow = store.Put(programcache::Hash("capacity"), 1,
                              std::vector<uint8_t>(1, 1));
    Log("CACHE_128M_PINNED", overflow ? "ADMITTED" : "REJECTED", store.Count());
    Storage(store, "cache-128m-pinned.json");
    for (auto &a : large)
      a.reset();
    if (!Drain(store))
      return;
    auto oversize =
        store.Put(programcache::Hash("entry-over"), 1,
                  std::vector<uint8_t>(programcache::kEntryLimit + 1, 1));
    Log("CACHE_ENTRY_LIMIT", oversize ? "ADMITTED" : "REJECTED",
        programcache::kEntryLimit + 1);
    auto diagnostics = store.Diagnostics502();
    p011gpu::Sha256 h;
    h.update(diagnostics.data(), diagnostics.size());
    auto digest = h.finish();
    Log("CACHE_STORAGE_SHA256",
        programcache::Hex(digest.data(), digest.size()).c_str());
    std::filesystem::path receipt =
        std::filesystem::path(directory) / "cache-storage.json";
    FILE *file = std::fopen(receipt.c_str(), "wb");
    if (file) {
      const auto n =
          std::fwrite(diagnostics.data(), 1, diagnostics.size(), file);
      const auto sync = std::fflush(file) == 0 && fsync(fileno(file)) == 0;
      std::fclose(file);
      Log("CACHE_STORAGE_FILE",
          sync && n == diagnostics.size() ? "cache-storage.json" : "IO_FAILED",
          n);
    }
  }
  void Run(std::string name) {
    bool ended = false;
    try {
      Log("START", name.c_str());
      if (name == "CHECKPOINT_BARRIER") {
        Log("CHECKPOINT_BARRIER", "BARRIER_ENTER");
        Wait();
        Stop();
      } else if (name.find("TRACE_") == 0 || name == "TWO_SLOT_PIN_REPLACE" ||
                 name.find("TTL_") == 0)
        TraceCases(name);
      else if (name.find("DIAGNOSTIC_") == 0)
        Diagnostic(name);
      else if (name == "CACHE_FAULT")
        Cache();
      else
        Log("RESOURCE_PHASE_SAMPLE", "USE_PRODUCTION_READ_ONLY_PAGE");
      ended = true;
    } catch (const std::exception &e) {
      Failure(e.what());
      Log("FAIL", e.what());
    } catch (...) {
      Failure("UNKNOWN_EXCEPTION");
      Log("FAIL", "UNKNOWN_EXCEPTION");
    }
    Draining();
    for (int i = 0; i < 2; ++i) {
      if (trace.slots[i].state != cold503::TraceManager::FREE)
        trace.End(trace.slots[i].id, BridgeTimeline::Now(), scope);
      auto block = trace.Trim(i);
      block.reset();
      ledger.HostStorage("LAB_TRACE_SLOT", uintptr_t(&trace.slots[i]), 0, 0,
                         false);
    }
    if (ended)
      Log("END", Stop() ? "CANCELLED" : "COMPLETED_FIXTURE_NOT_PHYSICS",
          trace.Capacity());
    std::lock_guard<std::mutex> lock(mutex);
    if (cancelled && !life_.cancelObservedNs)
      life_.cancelObservedNs = BridgeTimeline::Now();
    life_.outcome = life_.firstFailure[0] ? "CASE_FAILED"
                    : cancelled           ? "CANCELLED"
                    : ended               ? "COMPLETED_FIXTURE"
                                          : "INCOMPLETE";
    p011gpu::Sha256 digest;
    for (size_t i = 0; i < count; ++i)
      digest.update(records[i].raw.data(), std::strlen(records[i].raw.data()));
    life_.recordsSHA = digest.finish();
    life_.terminalNs = BridgeTimeline::Now();
    life_.phase = Lifecycle::FINISHED;
    running = false;
  }

public:
  enum class CancelResult {
    REQUESTED,
    ALREADY_REQUESTED,
    ALREADY_TERMINAL,
    STALE_JOB
  };
  ~Lab() {
    Cancel();
    if (worker.joinable())
      worker.join();
    CloseHandlesLocked();
  }
  std::string Start(const std::string &name, const std::string &files) {
    static constexpr const char *allowed[] = {
        "CHECKPOINT_BARRIER", "RESOURCE_PHASE_SAMPLE", "TRACE_MAX_SOURCE",
        "TRACE_MAX_Q",        "TWO_SLOT_PIN_REPLACE",  "TTL_WALL",
        "TTL_EDGE_INJECTED",  "DIAGNOSTIC_CREDIT",     "DIAGNOSTIC_BUSY_FAIL",
        "CACHE_FAULT"};
    bool valid = false;
    for (auto s : allowed)
      valid |= name == s;
    if (!valid)
      return "INVALID_CASE";
    {
      std::lock_guard<std::mutex> lock(mutex);
      if (running)
        return "LAB_BUSY";
      if (export_.active &&
          BridgeTimeline::Now() - export_.startedNs < kLabExportTtlNs)
        return "LAB_EXPORT_BUSY";
      CloseHandlesLocked();
      export_ = {};
    }
    if (worker.joinable())
      worker.join();
    const auto root = std::filesystem::weakly_canonical(files);
    const auto base = root / "t531-resource-lab" /
                      (std::to_string(getpid()) + "-" +
                       std::to_string(BridgeTimeline::Now()) + "-" +
                       std::to_string(serial + 1));
    if (root.empty() || !root.is_absolute() ||
        base.lexically_relative(root).empty())
      return "LAB_DIRECTORY_REJECTED";
    const auto canonical = std::filesystem::weakly_canonical(base);
    const auto relative = canonical.lexically_relative(root);
    if (relative.empty() || *relative.begin() == "..")
      return "LAB_DIRECTORY_REJECTED";
    directory = canonical.string();
    trace = {};
    scope = {};
    scope.pid = getpid();
    scope.control.epoch = scope.control.run = scope.control.context =
        scope.control.surface = scope.control.session = 1;
    std::strcpy(scope.route.data(), "m2");
    std::strcpy(scope.source.data(), "T531_RESOURCE_LAB_V1_FIXTURE_SYNTHETIC");
    {
      std::lock_guard<std::mutex> lock(mutex);
      job = ++serial;
      count = dropped = 0;
      savedCount_ = 0;
      savedFiles_ = {};
      running = true;
      cancelled = released = false;
      drainCancel_ = false;
      life_ = {};
      life_.generation = job;
      life_.phase = Lifecycle::RUNNING;
    }
    try {
      worker = std::thread([this, name] { Run(name); });
    } catch (...) {
      std::lock_guard<std::mutex> lock(mutex);
      running = false;
      life_.phase = Lifecycle::FINISHED;
      life_.terminalNs = BridgeTimeline::Now();
      life_.outcome = "CASE_FAILED";
      std::strcpy(life_.firstFailure.data(), "WORKER_ALLOCATION_FAILED");
      return "WORKER_ALLOCATION_FAILED";
    }
    return std::to_string(job);
  }
  CancelResult Cancel(uint64_t id = 0, uint64_t now = 0) {
    std::shared_ptr<programcache::ProgramArtifactStore> store;
    CancelResult result;
    {
      std::lock_guard<std::mutex> lock(mutex);
      if (id && id != job)
        return CancelResult::STALE_JOB;
      if (life_.phase == Lifecycle::FINISHED)
        return CancelResult::ALREADY_TERMINAL;
      if (cancelled)
        return CancelResult::ALREADY_REQUESTED;
      cancelled = released = true;
      life_.cancelRequestedNs = now ? now : BridgeTimeline::Now();
      life_.phase = Lifecycle::CANCELLING;
      drainCancel_ = true;
      store = activeStore_;
      result = CancelResult::REQUESTED;
      cv.notify_all();
    }
    if (store)
      store->CancelDrainForTest(drainCancel_);
    return result;
  }
  std::string CancelJson(uint64_t id) {
    const uint64_t now = BridgeTimeline::Now();
    const auto result = Cancel(id, now);
    const char *label =
        result == CancelResult::REQUESTED           ? "REQUESTED"
        : result == CancelResult::ALREADY_REQUESTED ? "ALREADY_REQUESTED"
        : result == CancelResult::ALREADY_TERMINAL  ? "ALREADY_TERMINAL"
                                                    : "STALE_JOB";
    std::lock_guard<std::mutex> lock(mutex);
    return "{\"cancelResult\":\"" + std::string(label) +
           "\",\"jobId\":" + std::to_string(id) +
           ",\"generation\":" + std::to_string(life_.generation) +
           ",\"cancelCallNs\":" + std::to_string(now) + "}";
  }
  void Release() {
    std::lock_guard<std::mutex> lock(mutex);
    released = true;
    cv.notify_all();
  }
  std::string Poll(uint64_t id) {
    std::array<char, 131072> bytes{};
    cold503::Writer w(bytes.data(), bytes.size());
    std::lock_guard<std::mutex> lock(mutex);
    w.Raw("{\"revision\":\"T531_RESOURCE_LAB_RECORD_V3\",\"evidenceOrigin\":"
          "\"FIXTURE_SYNTHETIC\",\"physicsValidated\":false,\"currentGranted\":"
          "false,\"deviceGLMeasured\":false,\"jobId\":");
    w.U64(job);
    w.Raw(",\"generation\":");
    w.U64(life_.generation);
    w.Raw(",\"pollSampleNs\":");
    w.U64(BridgeTimeline::Now());
    w.Raw(",\"terminalNs\":");
    w.U64(life_.terminalNs);
    w.Raw(",\"cancelRequestedNs\":");
    w.U64(life_.cancelRequestedNs);
    w.Raw(",\"cancelObservedNs\":");
    w.U64(life_.cancelObservedNs);
    w.Raw(",\"state\":");
    const char *state = id != job                              ? "STALE_JOB"
                        : life_.phase == Lifecycle::RUNNING    ? "RUNNING"
                        : life_.phase == Lifecycle::CANCELLING ? "CANCELLING"
                        : life_.phase == Lifecycle::DRAINING   ? "DRAINING"
                                                               : "FINISHED";
    w.String(state, std::strlen(state));
    w.Raw(",\"pendingResources\":");
    w.String(running ? "WORKER_RAII_RETIREMENT" : "NONE", running ? 21 : 4);
    if (id == job && !running) {
      w.Raw(",\"outcome\":");
      w.String(life_.outcome, std::strlen(life_.outcome));
      w.Raw(",\"firstFailure\":");
      w.String(life_.firstFailure.data(),
               std::strlen(life_.firstFailure.data()));
      w.Raw(",\"recordsSHA256\":");
      w.Hex(life_.recordsSHA.data(), 32);
    }
    w.Raw(",\"fixedNewMetadataBytes\":");
    w.Number(sizeof(Lifecycle) + sizeof(drainCancel_) + sizeof(activeStore_) +
             sizeof(programcache::ProgramArtifactStore::DrainCompletion) +
             sizeof(programcache::ProgramArtifactStore::DrainObservation) +
             sizeof(uint64_t) + 64);
    w.Raw(",\"scope\":");
    cold503::ScopeJson(w, scope);
    w.Raw(",\"sessionDirectory\":");
    w.String(directory.data(), directory.size());
    w.Raw(
        ",\"knownHostPayloadLimit\":58993832,\"excluded\":\"helper fixed "
        "objects; cache pins 128MiB; worker stacks; allocator\",\"dropped\":");
    w.Number(dropped);
    w.Raw(",\"records\":[");
    for (size_t i = 0; i < count; ++i) {
      if (i)
        w.Raw(",");
      w.Raw(records[i].raw.data());
    }
    w.Raw("]}");
    return w.Good() ? std::string(w.Data(), w.Size())
                    : "{\"state\":\"INCOMPLETE\"}";
  }
  bool Matches(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex);
    return id != 0 && id == job;
  }
  // T565 (problem T560 §4): same-process private-domain paged export of the
  // job's own SaveBytes allowlist. Single reader, fixed 60s TTL (no renew),
  // openat-style pinned root via the session directory (no caller paths, no
  // '..', no symlinks), stat identity checked before and after each read.
  std::string FilesBeginV2(uint64_t jobId) {
    std::lock_guard<std::mutex> lock(mutex);
    cold503::Writer w = LabReply();
    const auto now = BridgeTimeline::Now();
    if (export_.active && now - export_.startedNs < kLabExportTtlNs) {
      w.Raw("{\"state\":\"LAB_EXPORT_BUSY\"}");
      return Finish(w);
    }
    if (export_.active) {
      CloseHandlesLocked();
      export_ = {};
    }
    if (!jobId || jobId != job) {
      w.Raw("{\"state\":\"STALE_JOB\"}");
      return Finish(w);
    }
    if (running) {
      w.Raw("{\"state\":\"LAB_JOB_RUNNING\"}");
      return Finish(w);
    }
    if (savedCount_ == 0) {
      w.Raw("{\"state\":\"NO_FILES\"}");
      return Finish(w);
    }
    struct OpenRollback {
      Lab &lab;
      bool committed = false;
      ~OpenRollback() {
        if (!committed)
          lab.CloseHandlesLocked();
      }
    } rollback{*this};
    uint64_t total = 0;
    for (uint32_t i = 0; i < savedCount_; ++i) {
      const auto &entry = savedFiles_[i];
      if (entry.size > kLabFileBytesMax ||
          total + entry.size > kLabFilesTotalMax) {
        w.Raw("{\"state\":\"EXPORT_LIMIT\"}");
        return Finish(w);
      }
      total += entry.size;
      const auto path = std::filesystem::path(directory) / entry.name;
      struct stat identity {};
      if (lstat(path.c_str(), &identity) != 0 || !Identity(entry, identity)) {
        w.Raw("{\"state\":\"FILE_CHANGED\"}");
        return Finish(w);
      }
      int fd = ::open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
      FILE *f = fd < 0 ? nullptr : fdopen(fd, "rb");
      if (!f && fd >= 0)
        ::close(fd);
      if (!f) {
        w.Raw("{\"state\":\"FILE_OPEN_FAILED\"}");
        return Finish(w);
      }
      handles_[i] = f;
      if (!VerifyFile(i)) {
        w.Raw("{\"state\":\"FILE_CHANGED\"}");
        return Finish(w);
      }
    }
    export_ = {true, job, ++serial, now, savedCount_, total};
    rollback.committed = true;
    w.Raw(
        "{\"revision\":\"RESOURCE_LAB_FILES_V2\",\"state\":\"OK\",\"jobId\":");
    w.U64(job);
    w.Raw(",\"exportId\":");
    w.U64(export_.exportId);
    w.Raw(",\"expiresAtMonotonicNs\":");
    w.U64(now + kLabExportTtlNs);
    w.Raw(",\"fileCount\":");
    w.Number(savedCount_);
    w.Raw(",\"totalBytes\":");
    w.Number(total);
    w.Raw("}");
    return Finish(w);
  }
  std::string FilesIndexV2(uint64_t exportId, uint32_t page) {
    std::lock_guard<std::mutex> lock(mutex);
    cold503::Writer w = LabReply();
    if (!export_.active || exportId != export_.exportId) {
      w.Raw("{\"state\":\"STALE_EXPORT\"}");
      return Finish(w);
    }
    if (BridgeTimeline::Now() - export_.startedNs >= kLabExportTtlNs) {
      CloseHandlesLocked();
      export_.active = false;
      w.Raw("{\"state\":\"EXPIRED\"}");
      return Finish(w);
    }
    if (savedCount_ == 0 ||
        page >= (savedCount_ + kLabIndexPerPage - 1) / kLabIndexPerPage) {
      w.Raw("{\"state\":\"INVALID_PAGE\"}");
      return Finish(w);
    }
    w.Raw("{\"revision\":\"RESOURCE_LAB_FILES_V2\",\"state\":\"OK\","
          "\"exportId\":");
    w.U64(exportId);
    w.Raw(",\"page\":");
    w.Number(page);
    w.Raw(",\"entries\":[");
    const uint32_t first = page * kLabIndexPerPage,
                   last = std::min<uint32_t>(first + kLabIndexPerPage,
                                             savedCount_);
    for (uint32_t i = first; i < last; ++i) {
      const auto &e = savedFiles_[i];
      if (i > first)
        w.Raw(",");
      w.Raw("{\"fileId\":");
      w.Number(i);
      w.Raw(",\"name\":");
      w.String(e.name, std::strlen(e.name));
      w.Raw(",\"bytes\":");
      w.Number(e.size);
      w.Raw(",\"sha256\":");
      w.Hex(e.sha.data(), e.sha.size());
      w.Raw("}");
    }
    w.Raw("],\"done\":");
    w.Raw(last >= savedCount_ ? "true" : "false");
    w.Raw("}");
    return Finish(w);
  }
  std::string FilePageV2(uint64_t exportId, uint32_t fileId, uint64_t offset) {
    std::lock_guard<std::mutex> lock(mutex);
    cold503::Writer w = LabReply();
    if (!export_.active || exportId != export_.exportId) {
      w.Raw("{\"state\":\"STALE_EXPORT\"}");
      return Finish(w);
    }
    if (BridgeTimeline::Now() - export_.startedNs >= kLabExportTtlNs) {
      CloseHandlesLocked();
      export_.active = false;
      w.Raw("{\"state\":\"EXPIRED\"}");
      return Finish(w);
    }
    if (fileId >= savedCount_ || !handles_[fileId]) {
      w.Raw("{\"state\":\"INVALID_FILE\"}");
      return Finish(w);
    }
    const auto &entry = savedFiles_[fileId];
    FILE *f = handles_[fileId];
    if (offset % kLabChunkBytes != 0 || offset >= entry.size) {
      w.Raw("{\"state\":\"INVALID_OFFSET\"}");
      return Finish(w);
    }
    const auto path = std::filesystem::path(directory) / entry.name;
    struct stat identity {};
    if (!VerifyFile(fileId)) {
      CloseHandlesLocked();
      export_ = {};
      w.Raw("{\"state\":\"FILE_CHANGED\"}");
      return Finish(w);
    }
    const uint32_t n =
        uint32_t(std::min<uint64_t>(kLabChunkBytes, entry.size - offset));
    std::array<char, kLabChunkBytes> chunk{};
    if (std::fseek(f, offset, SEEK_SET) != 0) {
      CloseHandlesLocked();
      export_ = {};
      w.Raw("{\"state\":\"INCOMPLETE\"}");
      return Finish(w);
    }
    const auto read = std::fread(chunk.data(), 1, n, f);
    if (read != n) {
      CloseHandlesLocked();
      export_ = {};
      w.Raw("{\"state\":\"INCOMPLETE\"}");
      return Finish(w);
    }
    if (!VerifyFile(fileId)) {
      CloseHandlesLocked();
      export_ = {};
      w.Raw("{\"state\":\"FILE_CHANGED\"}");
      return Finish(w);
    }
    p011gpu::Sha256 sha;
    sha.update(chunk.data(), n);
    const auto digest = sha.finish();
    static_assert((kLabChunkBytes + 2) / 3 * 4 + 1024 <= 16384,
                  "base64+json bound");
    std::array<char, (kLabChunkBytes + 2) / 3 * 4 + 16> base64{};
    const auto encoded =
        LabBase64(chunk.data(), n, base64.data(), base64.size());
    w.Raw("{\"revision\":\"RESOURCE_LAB_FILES_V2\",\"state\":\"OK\","
          "\"exportId\":");
    w.U64(exportId);
    w.Raw(",\"fileId\":");
    w.Number(fileId);
    w.Raw(",\"offset\":");
    w.Number(offset);
    w.Raw(",\"nBytes\":");
    w.Number(n);
    w.Raw(",\"totalBytes\":");
    w.Number(entry.size);
    w.Raw(",\"eof\":");
    w.Raw(offset + n >= entry.size ? "true" : "false");
    w.Raw(",\"chunkSHA256\":");
    w.Hex(digest.data(), digest.size());
    w.Raw(",\"base64\":");
    w.String(base64.data(), encoded);
    w.Raw("}");
    return Finish(w);
  }
  std::string FilesEndV2(uint64_t exportId) {
    std::lock_guard<std::mutex> lock(mutex);
    cold503::Writer w = LabReply();
    const bool active = export_.active && exportId == export_.exportId;
    if (!active) {
      w.Raw("{\"state\":\"STALE_EXPORT\"}");
      return Finish(w);
    }
    CloseHandlesLocked();
    export_ = {};
    w.Raw("{\"revision\":\"RESOURCE_LAB_FILES_V2\",\"state\":\"OK\","
          "\"exportId\":");
    w.U64(exportId);
    w.Raw(",\"released\":");
    w.Raw(active ? "true" : "false");
    w.Raw("}");
    return Finish(w);
  }

public:
  void AbortExport() {
    std::lock_guard<std::mutex> lock(mutex);
    CloseHandlesLocked();
    export_ = {};
  }

private:
  static bool Identity(const SavedFileEntry &entry, const struct stat &st) {
    return S_ISREG(st.st_mode) && uint64_t(st.st_dev) == entry.device &&
           uint64_t(st.st_ino) == entry.inode &&
           uint64_t(st.st_size) == entry.size &&
           uint64_t(st.st_mtime) == entry.mtime &&
           uint64_t(st.st_mtim.tv_nsec) == entry.mtimeNs &&
           uint64_t(st.st_ctime) == entry.ctime &&
           uint64_t(st.st_ctim.tv_nsec) == entry.ctimeNs;
  }
  bool VerifyFile(uint32_t id) {
    const auto &e = savedFiles_[id];
    auto *f = handles_[id];
    struct stat before {
    }, path{}, after{}, pathAfter{};
    const auto name = std::filesystem::path(directory) / e.name;
    if (!f || fstat(fileno(f), &before) || lstat(name.c_str(), &path) ||
        !Identity(e, before) || !Identity(e, path))
      return false;
    p011gpu::Sha256 sha;
    std::array<char, 4096> block{};
    if (std::fseek(f, 0, SEEK_SET))
      return false;
    uint64_t bytes = 0;
    while (bytes < e.size) {
      size_t n = std::fread(
          block.data(), 1, std::min<uint64_t>(block.size(), e.size - bytes), f);
      if (!n)
        return false;
      sha.update(block.data(), n);
      bytes += n;
    }
    return sha.finish() == e.sha && !fstat(fileno(f), &after) &&
           !lstat(name.c_str(), &pathAfter) && Identity(e, after) &&
           Identity(e, pathAfter);
  }
  // T565: standard base64 (RFC 4648, with padding) for the 4096B chunk; the
  // caller-provided buffer must hold (n+2)/3*4 bytes.
  static size_t LabBase64(const char *data, size_t n, char *out,
                          size_t capacity) {
    if ((n + 2) / 3 * 4 > capacity)
      return 0;
    static const char *table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t at = 0, w = 0;
    while (n - at >= 3) {
      const uint32_t v = (uint8_t(data[at]) << 16) |
                         (uint8_t(data[at + 1]) << 8) | uint8_t(data[at + 2]);
      out[w++] = table[(v >> 18) & 63];
      out[w++] = table[(v >> 12) & 63];
      out[w++] = table[(v >> 6) & 63];
      out[w++] = table[v & 63];
      at += 3;
    }
    if (n - at == 1) {
      const uint32_t v = uint8_t(data[at]) << 16;
      out[w++] = table[(v >> 18) & 63];
      out[w++] = table[(v >> 12) & 63];
      out[w++] = '=';
      out[w++] = '=';
    } else if (n - at == 2) {
      const uint32_t v =
          (uint8_t(data[at]) << 16) | (uint8_t(data[at + 1]) << 8);
      out[w++] = table[(v >> 18) & 63];
      out[w++] = table[(v >> 12) & 63];
      out[w++] = table[(v >> 6) & 63];
      out[w++] = '=';
    }
    return w;
  }
  void CloseHandlesLocked() {
    for (auto &f : handles_) {
      if (f) {
        std::fclose(f);
        f = nullptr;
      }
    }
  }
  cold503::Writer LabReply() {
    return cold503::Writer(replyBytes_.data(), sizeof(replyBytes_));
  }
  std::string Finish(cold503::Writer &w) {
    if (!w.Good() || w.Size() >= sizeof(replyBytes_)) {
      CloseHandlesLocked();export_={};return "{\"state\":\"INCOMPLETE\"}";}
    return std::string(w.Data(), w.Size());
  }
  std::array<char, 16384> replyBytes_{};
};
} // namespace grs::resource538
