#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include "program_cache/sha256.h"

namespace t693 {
struct Row {
  uint64_t sequence, steadyNs;
  uint32_t tid;
  uint16_t function;
  uint8_t edge, flags;
  uint32_t detail, stamp;
};
static_assert(sizeof(Row) == 32);
constexpr size_t credit = 12 * 1024 * 1024;
constexpr size_t rowCapacity = 160000;
constexpr size_t dictCapacity = credit - rowCapacity * sizeof(Row) - 65536;
constexpr size_t dictSlots = 65536;

class Trace {
  struct Slot {
    uint64_t hash = 0;
    uint32_t at = 0;
    uint32_t id = 0;
    uint32_t size = 0;
  };
  std::unique_ptr<Row[]> rows = std::make_unique<Row[]>(rowCapacity);
  std::unique_ptr<char[]> dictionary = std::make_unique<char[]>(dictCapacity);
  std::unique_ptr<Slot[]> table = std::make_unique<Slot[]>(dictSlots);
  std::array<uint32_t, 255> functions{};
  uint16_t functionCount = 0;
  uint32_t nextId = 1;
  std::atomic<size_t> rowCount{0}, dictBytes{0};
  std::atomic<bool> done{false}, failed{false};
  std::mutex producer;
  std::thread writer;
  std::ofstream rowOutput, dictOutput, indexOutput, functionOutput;
  size_t rowWritten = 0, dictWritten = 0, functionWritten = 0, indexBytes = 0;
  std::atomic<size_t> publishedFunctions{0};
  uint64_t firstRejected = UINT64_MAX;
  std::string stem;

  static uint64_t hash(std::string_view s) {
    uint64_t h = 14695981039346656037ull;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ull; }
    return h;
  }
  uint32_t intern(std::string_view value) {
    const uint64_t h = hash(value);
    for (size_t i = 0; i < dictSlots; ++i) {
      Slot& slot = table[(h + i) % dictSlots];
      if (slot.id) {
        if (slot.hash == h && slot.size == value.size() &&
            (!slot.size || std::memcmp(dictionary.get() + slot.at, value.data(), slot.size) == 0))
          return slot.id;
        continue;
      }
      const size_t offset = dictBytes.load(std::memory_order_relaxed);
      if (value.size() > dictCapacity - offset || dictCapacity - offset - value.size() < 4)
        return 0;
      const auto n = static_cast<uint32_t>(value.size());
      std::memcpy(dictionary.get() + offset, &n, 4);
      if (n) std::memcpy(dictionary.get() + offset + 4, value.data(), n);
      slot = {h, static_cast<uint32_t>(offset + 4), nextId++, n};
      dictBytes.store(offset + 4 + n, std::memory_order_release);
      return slot.id;
    }
    return 0;
  }
  void writeBlock(std::ofstream& output, const char* data, size_t bytes,
                  const char* kind, size_t& watermark) {
    if (!bytes) return;
    if (indexBytes + 128 > 61440) { failed = true; return; }
    indexBytes += 128;
    output.write(data, bytes);
    output.flush();
    if (!output) { failed = true; return; }
    p011gpu::Sha256 sha;
    sha.update(data, bytes);
    auto digest = sha.finish();
    static constexpr char hex[] = "0123456789abcdef";
    indexOutput << kind << ' ' << watermark << ' ' << bytes << ' ';
    for (auto c : digest) indexOutput << hex[c >> 4] << hex[c & 15];
    indexOutput << '\n';
    indexOutput.flush();
    if (!indexOutput) { failed = true; return; }
    watermark += bytes;
  }
  void flush() {
    auto next = std::chrono::steady_clock::now();
    for (;;) {
      if (!done && !failed && std::chrono::steady_clock::now() < next) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        continue;
      }
      next = std::chrono::steady_clock::now() + std::chrono::seconds(2);
      const auto rowEnd = rowCount.load(std::memory_order_acquire) * sizeof(Row);
      const auto dictEnd = dictBytes.load(std::memory_order_acquire);
      const auto functionEnd = publishedFunctions.load(std::memory_order_acquire) * sizeof(uint32_t);
      writeBlock(functionOutput, reinterpret_cast<const char*>(functions.data()) + functionWritten, functionEnd - functionWritten, "F", functionWritten);
      writeBlock(dictOutput, dictionary.get() + dictWritten, dictEnd - dictWritten, "D", dictWritten);
      writeBlock(rowOutput, reinterpret_cast<const char*>(rows.get()) + rowWritten,
                 rowEnd - rowWritten, "R", rowWritten);
      if (done.load() && dictEnd == dictBytes.load() && rowEnd == rowCount.load() * sizeof(Row)) return;
      std::this_thread::sleep_for(std::chrono::milliseconds(25));
      if (failed && indexBytes + 128 > 61440) return;
      if (!indexOutput || !rowOutput || !dictOutput || !functionOutput) return;
    }
  }
 public:
  ~Trace() { close(); }
  bool open(const std::string& path, const std::string& identity) {
    stem = path;
    if (identity.size() > 1024) return false;
    for (const char* suffix : {".rows", ".meta", ".dict", ".fn", ".index", ".end"})
      if (std::filesystem::exists(path + suffix)) return false;
    rowOutput.open(path + ".rows", std::ios::binary);
    dictOutput.open(path + ".dict", std::ios::binary);
    functionOutput.open(path + ".fn", std::ios::binary);
    indexOutput.open(path + ".index", std::ios::binary);
    std::ofstream header(path + ".meta", std::ios::binary);
    header << "T693_LE32_V1\n" << identity << '\n';
    if (!rowOutput || !dictOutput || !indexOutput || !functionOutput || !header) return false;
    writer = std::thread([this]{ flush(); });
    return true;
  }
  bool append(uint64_t seq, uint64_t wall, uint32_t tid, uint32_t function,
              uint8_t edge, std::string_view detail, std::string_view stamp) {
    std::lock_guard<std::mutex> lock(producer);
    if (failed || done) return false;
    uint16_t fn = 0;
    while (fn < functionCount && functions[fn] != function) ++fn;
    const auto at = rowCount.load(std::memory_order_relaxed);
    if (at < rowCapacity && fn < 255) {
      if (fn == functionCount) {
        functions[functionCount++] = function;
        publishedFunctions.store(functionCount, std::memory_order_release);
      }
      const auto d = intern(detail);
      const auto s = intern(stamp);
      if (d && s) {
        rows[at] = {seq,wall,tid,fn,edge,0,d,s};
        rowCount.store(at + 1, std::memory_order_release);
        return true;
      }
    }
    firstRejected = seq;
    failed = true;
    return false;
  }
  void close() {
    if (!writer.joinable()) return;
    done = true;
    writer.join();
    std::ofstream footer(stem + ".end", std::ios::binary);
    footer << (failed ? "INCOMPLETE" : "SEALED") << ' ' << rowWritten << ' '
           << dictWritten << ' ' << firstRejected << '\n';
    for (size_t i = 0; i < functionCount; ++i) footer << functions[i] << '\n';
  }
};
}
