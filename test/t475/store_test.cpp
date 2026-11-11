#define GRS_PROGRAM_CACHE_TEST_FAULTS
#include "artifact_store.h"
#include <cassert>
#include <iostream>
using namespace grs::programcache;
static std::vector<uint8_t> Read(const std::filesystem::path &path) {
  std::ifstream in(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
int main(int argc, char **argv) {
  assert(argc == 2);
  std::filesystem::path root = argv[1];
  auto key = Hash("source");
  auto dir = root / "gpu-program-cache/v1";
  assert(!std::filesystem::exists(dir));
  {
    ProgramArtifactStore s;
    assert(s.Configure(root));
    assert(!s.Get(key));
    auto a = s.Put(key, 7, {1, 2, 3});
    assert(a);
    s.Receipt(key, "{\"stage\":\"EXPORT\"}", a);
    s.Drain();
    assert(a->state == PERSISTED);
    assert(Read(dir / (key + ".jsonl")).size() > 0);
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i)
      threads.emplace_back([&] { assert(s.Put(key, 8, {9, 9}) == a); });
    for (auto &thread : threads)
      thread.join();
    assert(a->bytes == std::vector<uint8_t>({1, 2, 3}));
    assert(!s.Put(key, 7, {}));
    assert(!s.Put(key, 7, std::vector<uint8_t>(kEntryLimit + 1)));
  }
  {
    std::ofstream tmp(dir / (key + ".bin.tmp"));
    tmp << "interrupted";
  }
  {
    ProgramArtifactStore s;
    assert(s.Configure(root));
    auto a = s.Get(key);
    assert(a && a->format == 7 && a->state == PERSISTED);
    assert(!std::filesystem::exists(dir / (key + ".bin.tmp")));
  }
  auto old = Read(dir / (key + ".bin"));
  {
    std::fstream bin(dir / (key + ".bin"),
                     std::ios::binary | std::ios::in | std::ios::out);
    bin.seekp(144);
    bin.put('x');
  }
  {
    ProgramArtifactStore s;
    assert(s.Configure(root));
    assert(!s.Get(key));
  }
  {
    std::ofstream bin(dir / (key + ".bin"), std::ios::binary | std::ios::trunc);
    bin.write(reinterpret_cast<char *>(old.data()), 50);
  }
  {
    ProgramArtifactStore s;
    assert(s.Configure(root));
    assert(!s.Get(key));
  }
  // Block only the temporary filename. A failed replacement must retain old
  // bytes.
  {
    std::ofstream bin(dir / (key + ".bin"), std::ios::binary | std::ios::trunc);
    bin.write(reinterpret_cast<char *>(old.data()), old.size());
  }
  std::filesystem::create_directory(dir / (key + ".bin.tmp"));
  {
    ProgramArtifactStore s;
    assert(s.Configure(root));
    // Direct atomic-write failure on a new key, while the existing valid key
    // stays intact.
    auto failedKey = Hash("write failure");
    std::filesystem::create_directory(dir / (failedKey + ".bin.tmp"));
    auto a = s.Put(failedKey, 7, {4});
    s.Receipt(failedKey, "{\"stage\":\"WRITE\"}", a);
    s.Drain();
    assert(a->state == WRITE_FAILED);
    assert(Read(dir / (key + ".bin")) == old);
    auto receipt = Read(dir / (failedKey + ".jsonl"));
    assert(std::string(receipt.begin(), receipt.end()).find("WRITE_FAILED") !=
           std::string::npos);
    std::filesystem::remove(dir / (failedKey + ".bin.tmp"));
    // Receipt persistence failure is observable from native state.
    auto receiptKey = Hash("receipt failure");
    std::filesystem::create_directory(dir / (receiptKey + ".jsonl.tmp"));
    auto before = s.ReceiptFailures();
    s.Receipt(receiptKey, "{}");
    s.Drain();
    assert(s.ReceiptFailures() > before);
    std::filesystem::remove(dir / (receiptKey + ".jsonl.tmp"));
  }
  std::filesystem::remove(dir / (key + ".bin.tmp"));
  {
    ProgramArtifactStore s;
    assert(s.Configure(root));
    auto pinned = s.Get(key);
    for (int i = 0; i < 40; i++) {
      auto a = s.Put(Hash(std::to_string(i)), 7, {1});
      assert(a);
      s.Drain();
    }
    assert(s.Get(key) == pinned && s.Count() <= kCountLimit);
    for (int i = 0; i < 80; ++i) {
      s.Receipt(key, "{\"payload\":\"" + std::string(10000, 'a') + "\"}",
                pinned);
      s.Drain();
    }
    assert(std::filesystem::file_size(dir / (key + ".jsonl")) <= kReceiptLimit);
  }
  {
    ProgramArtifactStore s;
    auto blocked = root / "blocked";
    {
      std::ofstream file(blocked);
      file << 'x';
    }
    assert(!s.Configure(blocked));
    auto a = s.Put(Hash("unconfigured"), 7, {1});
    s.Drain();
    assert(a && a->state == WRITE_FAILED);
  }
  for (unsigned fault = 1; fault <= 4; ++fault) {
    ProgramArtifactStore s;
    auto faultRoot = root / ("fault-" + std::to_string(fault));
    assert(s.Configure(faultRoot));
    auto faultDir = faultRoot / "gpu-program-cache/v1";
    // Seed an existing valid disk version after cold preload. Force the next
    // atomic replacement itself to fail, preserving exactly that old version.
    {
      std::ofstream bin(faultDir / (key + ".bin"), std::ios::binary);
      bin.write(reinterpret_cast<const char *>(old.data()), old.size());
    }
    s.SetWriteFault(fault);
    auto a = s.Put(key, 9, {7, 8, 9});
    s.Drain();
    assert(a && a->state == WRITE_FAILED);
    assert(Read(faultDir / (key + ".bin")) == old);
    assert(!std::filesystem::exists(faultDir / (key + ".bin.tmp")));
  }
  // Exercise byte capacity separately from entry count with live reader pins.
  {
    ProgramArtifactStore s;
    assert(s.Configure(root / "byte-capacity"));
    std::vector<std::shared_ptr<Artifact>> pins;
    for (int i = 0; i < 4; ++i) {
      auto a = s.Put(Hash("large-" + std::to_string(i)), 7,
                     std::vector<uint8_t>(kEntryLimit, 1));
      assert(a);
      pins.push_back(a);
      s.Drain();
    }
    assert(!s.Put(Hash("over-capacity"), 7, {1}));
    pins.clear();
    auto replacement = s.Put(Hash("after-unpin"), 7, {1});
    assert(replacement);
    s.Drain();
  }
  uint64_t total = 0;
  for (const auto &file : std::filesystem::directory_iterator(dir))
    if (file.is_regular_file())
      total += file.file_size();
  assert(total <= kTotalLimit);
  std::cout << "PASS persistence/restart, bounded receipts, concurrent "
               "immutable same key, bad hash/truncation, interrupted temp, "
               "write failure/old file preservation, receipt failure "
               "visibility, pinned LRU/byte limit, configure failure, injected "
               "ENOSPC/partial/sync/rename failure preserves same old file\n";
}
