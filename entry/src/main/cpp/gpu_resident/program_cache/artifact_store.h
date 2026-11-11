#pragma once
// T475. Only the serial file worker accesses the application cache directory.
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
#include "sha256.h"
namespace grs::programcache {
inline constexpr size_t kEntryLimit = 32u * 1024 * 1024;
inline constexpr size_t kTotalLimit = 128u * 1024 * 1024;
inline constexpr size_t kCountLimit = 32;
inline constexpr size_t kReceiptLimit = 128u * 1024;
inline bool KeyValid(const std::string &s) {
  return s.size() == 64 &&
         s.find_first_not_of("0123456789abcdef") == std::string::npos;
}
inline std::string Hex(const void *p, size_t n) {
  static const char *digits = "0123456789abcdef";
  std::string out;
  const auto *bytes = static_cast<const unsigned char *>(p);
  for (size_t i = 0; i < n; i++) {
    out += digits[bytes[i] >> 4];
    out += digits[bytes[i] & 15];
  }
  return out;
}
inline std::string Hash(const void *p, size_t n) {
  p011gpu::Sha256 sha;
  sha.update(p, n);
  auto bytes = sha.finish();
  return Hex(bytes.data(), bytes.size());
}
inline std::string Hash(const std::string &s) {
  return Hash(s.data(), s.size());
}
enum Persistence { EXPORTED_IN_MEMORY, WRITE_PENDING, PERSISTED, WRITE_FAILED };
struct Artifact {
  std::string key;
  uint32_t format = 0;
  std::vector<uint8_t> bytes;
  std::atomic<int> state{EXPORTED_IN_MEMORY};
  bool invalid = false;
  uint64_t touched = 0;
};
class ProgramArtifactStore {
  std::mutex mutex_;
  std::condition_variable wake_;
  std::deque<std::function<void()>> jobs_;
  bool quit_ = false;
  std::thread worker_;
  std::filesystem::path directory_;
  std::map<std::string, std::shared_ptr<Artifact>> entries_;
  size_t bytes_ = 0;
  uint64_t clock_ = 0;
  std::map<std::string, std::string> rejected_;
  std::map<std::string, bool> unavailable_;
  std::atomic<bool> configured_{false};
  std::atomic<uint64_t> receiptFailures_{0};

#ifdef GRS_PROGRAM_CACHE_TEST_FAULTS
  std::atomic<unsigned> writeFault_{0};
#endif
  static bool Sync(FILE *f) {
    if (std::fflush(f))
      return false;
#ifdef _WIN32
    return _commit(_fileno(f)) == 0;
#else
    return fsync(fileno(f)) == 0;
#endif
  }
  bool Atomic(const std::filesystem::path &dest,
              const std::vector<uint8_t> &bytes) {
    if (directory_.empty())
      return false;
#ifdef GRS_PROGRAM_CACHE_TEST_FAULTS
    const auto fault = writeFault_.load();
    if (fault == 1) {
      errno = ENOSPC;
      return false;
    }
#else
    constexpr unsigned fault = 0;
#endif
    auto tmp = dest;
    tmp += ".tmp";
#ifdef _WIN32
    FILE *f = _wfopen(tmp.c_str(), L"wb");
#else
    FILE *f = std::fopen(tmp.c_str(), "wb");
#endif
    if (!f)
      return false;
    const auto writeSize = fault == 2 ? bytes.size() / 2 : bytes.size();
    bool ok =
        std::fwrite(bytes.data(), 1, writeSize, f) == bytes.size() && Sync(f);
    if (fault == 3)
      ok = false; // Inject sync failure after a complete temporary write.
    if (fault == 4)
      ok = false; // Inject replacement failure before touching destination.
    if (std::fclose(f))
      ok = false;
    if (ok) {
#ifdef _WIN32
      ok = MoveFileExW(tmp.c_str(), dest.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
      ok = ::rename(tmp.c_str(), dest.c_str()) == 0;
      if (ok) {
        int fd = ::open(directory_.c_str(), O_RDONLY | O_DIRECTORY);
        if (fd < 0)
          ok = false;
        else {
          ok = fsync(fd) == 0;
          ::close(fd);
        }
      }
#endif
    }
    if (!ok) {
      std::error_code ec;
      std::filesystem::remove(tmp, ec);
    }
    return ok;
  }
  void Loop() {
    for (;;) {
      std::function<void()> job;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        wake_.wait(lock, [&] { return quit_ || !jobs_.empty(); });
        if (quit_ && jobs_.empty())
          return;
        job = std::move(jobs_.front());
        jobs_.pop_front();
      }
      try {
        job();
      } catch (...) {
        ++receiptFailures_;
      }
    }
  }
  bool Reserve(
      size_t n) { // mutex_ held. Shared artifacts remain pinned and accounted.
    while (entries_.size() >= kCountLimit || bytes_ + n > kTotalLimit) {
      auto victim = entries_.end();
      for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->second.use_count() == 1 && it->second->state != WRITE_PENDING &&
            (victim == entries_.end() ||
             it->second->touched < victim->second->touched))
          victim = it;
      }
      if (victim == entries_.end())
        return false;
      bytes_ -= victim->second->bytes.size();
      entries_.erase(victim);
    }
    return true;
  }
  struct DiskEntry {
    std::string key;
    uint64_t bytes = 0;
    std::filesystem::file_time_type touched{};
  };
  // Called only on file worker. Reserve space BEFORE replacing a valid
  // artifact. Include receipts, headers and atomic temporary bytes in the total
  // disk bound.
  bool DiskRoom(const std::string &protectedKey, size_t incoming) {
    std::map<std::string, DiskEntry> grouped;
    uint64_t total = 0;
    for (const auto &f : std::filesystem::directory_iterator(directory_)) {
      if (!f.is_regular_file())
        continue;
      auto ext = f.path().extension().string();
      auto key = f.path().stem().string();
      if ((ext != ".bin" && ext != ".jsonl") || !KeyValid(key))
        continue;
      auto &entry = grouped[key];
      entry.key = key;
      entry.bytes += f.file_size();
      entry.touched = std::max(entry.touched, f.last_write_time());
      total += f.file_size();
    }
    std::vector<DiskEntry> ordered;
    for (const auto &item : grouped)
      ordered.push_back(item.second);
    std::sort(ordered.begin(), ordered.end(), [](const auto &a, const auto &b) {
      return a.touched < b.touched;
    });
    size_t count =
        grouped.size() +
        (protectedKey.empty() || grouped.count(protectedKey) ? 0 : 1);
    for (const auto &entry : ordered) {
      if (count <= kCountLimit && total + incoming <= kTotalLimit)
        break;
      if (entry.key == protectedKey)
        continue;
      bool pinned;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        auto at = entries_.find(entry.key);
        pinned = at != entries_.end() && (at->second.use_count() > 1 ||
                                          at->second->state == WRITE_PENDING);
        // Disk eviction cannot leave a memory artifact claiming PERSISTED.
        if (!pinned && at != entries_.end()) {
          bytes_ -= at->second->bytes.size();
          entries_.erase(at);
        }
      }
      if (pinned)
        continue;
      std::error_code ec;
      for (const auto *ext : {".bin", ".jsonl"}) {
        auto file = directory_ / (entry.key + ext);
        if (std::filesystem::is_regular_file(file, ec)) {
          auto size = std::filesystem::file_size(file, ec);
          if (!ec && std::filesystem::remove(file, ec))
            total -= size;
        }
      }
      if (!std::filesystem::exists(directory_ / (entry.key + ".bin")) &&
          !std::filesystem::exists(directory_ / (entry.key + ".jsonl")))
        --count;
    }
    return count <= kCountLimit && total + incoming <= kTotalLimit;
  }
  void ReadArtifact(const std::filesystem::directory_entry &f) {
    const auto keyName = f.path().stem().string();
    auto reject = [&](const char *reason) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (rejected_.size() >= kCountLimit)
        rejected_.erase(rejected_.begin());
      rejected_[keyName] = reason;
    };
    auto size = f.file_size();
    if (size <= 144 || size > kEntryLimit + 144) {
      reject("DISK_LENGTH_REJECTED");
      return;
    }
    std::vector<uint8_t> wire(size);
    std::ifstream in(f.path(), std::ios::binary);
    if (!in.read(reinterpret_cast<char *>(wire.data()), wire.size())) {
      reject("DISK_READ_FAILED");
      return;
    }
    if (std::memcmp(wire.data(), "T475B001", 8)) {
      reject("DISK_SCHEMA_REJECTED");
      return;
    }
    uint32_t format = 0, n = 0;
    std::memcpy(&format, wire.data() + 8, 4);
    std::memcpy(&n, wire.data() + 12, 4);
    std::string key(reinterpret_cast<char *>(wire.data() + 16), 64);
    std::string digest(reinterpret_cast<char *>(wire.data() + 80), 64);
    if (!n || n > kEntryLimit || n + 144 != wire.size() ||
        key != f.path().stem().string() ||
        Hash(wire.data() + 144, n) != digest) {
      reject("DISK_KEY_LENGTH_HASH_REJECTED");
      return;
    }
    auto a = std::make_shared<Artifact>();
    a->key = key;
    a->format = format;
    a->bytes.assign(wire.begin() + 144, wire.end());
    a->state = PERSISTED;
    std::lock_guard<std::mutex> lock(mutex_);
    if (Reserve(n)) {
      a->touched = ++clock_;
      entries_[key] = a;
      bytes_ += n;
    }
  }

public:
  ProgramArtifactStore() : worker_([this] { Loop(); }) {}
  ~ProgramArtifactStore() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      quit_ = true;
    }
    wake_.notify_all();
    worker_.join();
  }
  ProgramArtifactStore(const ProgramArtifactStore &) = delete;
  bool Configure(const std::filesystem::path &filesDir) {
    auto done = std::make_shared<std::promise<bool>>();
    auto future = done->get_future();
    Enqueue([this, filesDir, done] {
      if (configured_) {
        done->set_value(directory_ == filesDir / "gpu-program-cache/v1");
        return;
      }
      try {
        directory_ = filesDir / "gpu-program-cache/v1";
        std::filesystem::create_directories(directory_);
        for (const auto &f : std::filesystem::directory_iterator(directory_)) {
          if (f.path().extension() == ".tmp" && f.is_regular_file()) {
            std::error_code ec;
            std::filesystem::remove(f.path(), ec);
          }
        }
        if (!DiskRoom("", 0))
          throw std::runtime_error("CACHE_CAPACITY");
        for (const auto &f : std::filesystem::directory_iterator(directory_)) {
          if (f.is_regular_file() && f.path().extension() == ".bin" &&
              KeyValid(f.path().stem().string()))
            ReadArtifact(f);
        }
        configured_ = true;
        done->set_value(true);
      } catch (...) {
        done->set_value(false);
      }
    });
    return future.get();
  }
  void Enqueue(std::function<void()> job) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (quit_ || jobs_.size() >= 128)
      throw std::runtime_error("CACHE_QUEUE_FULL");
    jobs_.push_back(std::move(job));
    wake_.notify_one();
  }
  std::shared_ptr<Artifact> Get(const std::string &key) {
    std::shared_ptr<Artifact> a;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = entries_.find(key);
      if (it == entries_.end() || it->second->invalid)
        return {};
      a = it->second;
      a->touched = ++clock_;
    }
    try {
      Enqueue([this, a] {
        if (!configured_)
          return;
        std::error_code ec;
        std::filesystem::last_write_time(
            directory_ / (a->key + ".bin"),
            std::filesystem::file_time_type::clock::now(), ec);
      });
    } catch (...) {
      ++receiptFailures_;
    }
    return a;
  }
  void Reject(const std::string &key) {
    std::shared_ptr<Artifact> a;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = entries_.find(key);
      if (it != entries_.end()) {
        a = it->second;
        a->invalid = true;
      }
    }
    try {
      Enqueue([this, key, a] {
        if (directory_.empty())
          return;
        std::error_code ec;
        std::filesystem::remove(directory_ / (key + ".bin"), ec);
      });
    } catch (...) {
      ++receiptFailures_;
    }
  }
  std::shared_ptr<Artifact> Put(std::string key, uint32_t format,
                                std::vector<uint8_t> bytes) {
    if (!KeyValid(key) || bytes.empty() || bytes.size() > kEntryLimit)
      return {};
    auto a = std::make_shared<Artifact>();
    a->key = key;
    a->format = format;
    a->bytes = std::move(bytes);
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto old = entries_.find(key);
      if (old != entries_.end()) {
        // Same-key submissions share immutable bytes. Rejection explicitly
        // invalidates a generation.
        if (!old->second->invalid)
          return old->second;
        if (old->second.use_count() > 1 || old->second->state == WRITE_PENDING)
          return {};
        bytes_ -= old->second->bytes.size();
        entries_.erase(old);
      }
      if (!Reserve(a->bytes.size()))
        return {};
      a->touched = ++clock_;
      a->state = WRITE_PENDING;
      entries_[key] = a;
      bytes_ += a->bytes.size();
    }
    try {
      Enqueue([this, a] {
        try {
          if (!configured_ || !DiskRoom(a->key, 144 + a->bytes.size())) {
            a->state = WRITE_FAILED;
            return;
          }
          std::vector<uint8_t> wire(144 + a->bytes.size());
          std::memcpy(wire.data(), "T475B001", 8);
          uint32_t n = uint32_t(a->bytes.size());
          std::memcpy(wire.data() + 8, &a->format, 4);
          std::memcpy(wire.data() + 12, &n, 4);
          std::memcpy(wire.data() + 16, a->key.data(), 64);
          auto digest = Hash(a->bytes.data(), a->bytes.size());
          std::memcpy(wire.data() + 80, digest.data(), 64);
          std::memcpy(wire.data() + 144, a->bytes.data(), n);
          a->state = Atomic(directory_ / (a->key + ".bin"), wire)
                         ? PERSISTED
                         : WRITE_FAILED;
        } catch (...) {
          a->state = WRITE_FAILED;
        }
      });
    } catch (...) {
      a->state = WRITE_FAILED;
    }
    return a;
  }
  // JSONL retains the most recent bounded event history per key. Receipts are
  // automatic.
  void Receipt(std::string key, std::string json,
               std::shared_ptr<Artifact> artifact = {}) {
    if (!KeyValid(key) || json.size() > 60000) {
      ++receiptFailures_;
      return;
    }
    try {
      Enqueue([this, key, json, artifact] {
        try {
          if (!configured_) {
            ++receiptFailures_;
            return;
          }
          auto text = json;
          if (!text.empty() && text.back() == '}')
            text.pop_back();
          const char *states[] = {"EXPORTED_IN_MEMORY", "WRITE_PENDING",
                                  "PERSISTED", "WRITE_FAILED"};
          if (artifact)
            text += ",\"binarySHA\":\"" +
                    Hash(artifact->bytes.data(), artifact->bytes.size()) + "\"";
          text += ",\"persistence\":\"" +
                  std::string(artifact ? states[artifact->state.load()]
                                       : "CACHE_NOT_STORED") +
                  "\"}\n";
          auto file = directory_ / (key + ".jsonl");
          std::error_code ec;
          auto size = std::filesystem::file_size(file, ec);
          std::string history;
          if (!ec && size <= kReceiptLimit) {
            history.resize(size);
            std::ifstream in(file, std::ios::binary);
            if (!in.read(history.data(), size))
              history.clear();
          }
          history += text;
          while (history.size() > kReceiptLimit) {
            auto newline = history.find('\n');
            if (newline == std::string::npos) {
              history.clear();
              break;
            }
            history.erase(0, newline + 1);
          }
          std::vector<uint8_t> wire(history.begin(), history.end());
          if (!DiskRoom(key, wire.size()) || !Atomic(file, wire))
            ++receiptFailures_;
        } catch (...) {
          ++receiptFailures_;
        }
      });
    } catch (...) {
      ++receiptFailures_;
    }
  }
  void Drain() {
    auto done = std::make_shared<std::promise<void>>();
    auto f = done->get_future();
    Enqueue([done] { done->set_value(); });
    f.get();
  }
  std::string Rejection(const std::string &key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto at = rejected_.find(key);
    return at == rejected_.end() ? "" : at->second;
  }
  bool ExportUnavailable(const std::string &key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return unavailable_.count(key) != 0;
  }
  void DisableExport(const std::string &key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (unavailable_.size() >= kCountLimit)
      unavailable_.erase(unavailable_.begin());
    unavailable_[key] = true;
  }
#ifdef GRS_PROGRAM_CACHE_TEST_FAULTS
  void SetWriteFault(unsigned fault) { writeFault_ = fault; }
#endif
  size_t Count() {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
  }
  bool Ready() const { return configured_; }
  uint64_t ReceiptFailures() const { return receiptFailures_; }
};
inline ProgramArtifactStore &Store() {
  static ProgramArtifactStore store;
  return store;
}
} // namespace grs::programcache
