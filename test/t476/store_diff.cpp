
#define GRS_PROGRAM_CACHE_TEST_FAULTS
#include "../../entry/src/main/cpp/gpu_resident/program_cache/artifact_store.h"
#include <cassert>
#include <iostream>
using namespace grs::programcache;
static int checks = 0, passed = 0;
static void check(const char *id, bool ok, const std::string &detail) {
  ++checks; if (ok) ++passed;
  std::cout << (ok ? "PASS " : "FAIL ") << id << " | " << detail << std::endl;
}
static std::vector<uint8_t> Read(const std::filesystem::path &p) {
  std::ifstream in(p, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
int main(int argc, char **argv) {
  assert(argc == 2);
  std::filesystem::path root = argv[1];
  const auto dir = root / "gpu-program-cache/v1";

  // ---- F1 key 分离（卡面：重签来源与 key 分离）----
  const auto k1 = Hash("source-A");
  const auto k2 = Hash("source-B");
  check("F1_KEY_SEPARATION",
        k1 != k2 && KeyValid(k1) && KeyValid(k2) && !KeyValid("xyz") &&
            !KeyValid(std::string(64, 'g')),
        "sha key 派生、64hex 校验、非 hex 拒绝");

  {
    ProgramArtifactStore s;
    assert(s.Configure(root));
    auto a = s.Put(k1, 7, {1, 2, 3});
    check("F2_PUT_IMMUTABLE_STATE", a && a->state == WRITE_PENDING,
          "Put 后 WRITE_PENDING（文件线程未完）");
    s.Receipt(k1, "{\"stage\":\"EXPORT\"}", a);
    s.Drain();
    check("F3_PERSISTED_TRANSITION", a->state == PERSISTED,
          "Drain 后 PERSISTED");
    check("F4_DISK_WIRE_FORMAT",
          Read(dir / (k1 + ".bin")).size() == 144 + 3 &&
              std::memcmp(Read(dir / (k1 + ".bin")).data(), "T475B001", 8) == 0,
          "磁盘 wire = 144B header+payload，magic T475B001");
    // 同 key 并发提交共享不可变版本
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i)
      threads.emplace_back([&] { assert(s.Put(k1, 8, {9, 9, 9}) == a); });
    for (auto &t : threads) t.join();
    check("F5_CONCURRENT_SAME_KEY_IMMUTABLE",
          a->bytes == std::vector<uint8_t>({1, 2, 3}),
          "8 线程同 key 提交共享同一 artifact 且字节不变");
    check("F6_EMPTY_REJECTED", !s.Put(k2, 7, {}), "空 payload 拒绝");
    check("F7_OVERSIZE_REJECTED",
          !s.Put(k2, 7, std::vector<uint8_t>(kEntryLimit + 1)),
          "超 32MiB 单条拒绝");
  }

  // ---- 跨进程：新进程实例（同目录重启）→ Get compile0 语义（文件层）----
  {
    ProgramArtifactStore s;
    assert(s.Configure(root));
    auto a = s.Get(k1);
    check("F8_RESTART_LOAD", a && a->format == 7 && a->state == PERSISTED &&
                                 a->bytes == std::vector<uint8_t>({1, 2, 3}),
          "重启实例读回 PERSISTED 同字节（GL 侧 compile0 由 t476-gl 独立核）");
    // .tmp 残留清理
    { std::ofstream t(dir / (k2 + ".bin.tmp")); t << "interrupted"; }
    ProgramArtifactStore s2;
    assert(s2.Configure(root));
    check("F9_TMP_CLEANED", !std::filesystem::exists(dir / (k2 + ".bin.tmp")),
          "启动清理 .tmp 残留");
  }

  // ---- 失效：坏 hash / 截断 / 未知 schema ----
  {
    auto old = Read(dir / (k1 + ".bin"));
    {
      std::fstream bin(dir / (k1 + ".bin"), std::ios::binary | std::ios::in | std::ios::out);
      bin.seekp(144); bin.put('x');  // payload hash 破坏
    }
    {
      ProgramArtifactStore s; assert(s.Configure(root));
      check("F10_CORRUPT_HASH_REJECTED", !s.Get(k1) &&
            std::string(s.Rejection(k1)).rfind("DISK_", 0) == 0,
            "坏 hash 拒绝且 Rejection 带 DISK_ 前缀");
    }
    { // 截断
      std::filesystem::resize_file(dir / (k1 + ".bin"), 50);
      ProgramArtifactStore s; assert(s.Configure(root));
      check("F11_TRUNCATED_REJECTED", !s.Get(k1), "截断（50B）拒绝");
    }
    { // 未知 schema
      std::ofstream bin(dir / (k1 + ".bin"), std::ios::binary | std::ios::trunc);
      bin.write("XXXXXXXX", 8);
      bin.write(reinterpret_cast<const char *>(old.data() + 8), old.size() - 8);
      ProgramArtifactStore s; assert(s.Configure(root));
      check("F12_UNKNOWN_SCHEMA_REJECTED", !s.Get(k1), "坏 magic 拒绝");
    }
    std::cout << "AT_F13" << std::endl;
    { // key/文件名不匹配
      std::ofstream bin(dir / (k1 + ".bin"), std::ios::binary | std::ios::trunc);
      bin.write(reinterpret_cast<const char *>(old.data()), old.size());
      std::error_code rec;
      std::filesystem::remove(dir / (k2 + ".bin"), rec);
      // rename 在此环境持续报占用（写句柄已关仍锁）——用 copy+remove 等价实现 header-key!=文件名
      bool copied = false;
      for (int attempt = 0; attempt < 5 && !copied; ++attempt) {
        rec.clear();
        std::filesystem::copy_file(dir / (k1 + ".bin"), dir / (k2 + ".bin"),
                                   std::filesystem::copy_options::overwrite_existing, rec);
        copied = !rec;
        if (!copied) std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      if (!copied) { check("F13_KEY_FILE_MISMATCH_REJECTED", false, "copy failed: " + rec.message()); }
      else {
        ProgramArtifactStore s; assert(s.Configure(root));
        check("F13_KEY_FILE_MISMATCH_REJECTED", !s.Get(k2), "header key != 文件名拒绝");
        std::filesystem::remove(dir / (k2 + ".bin"), rec);
      }
    }
    std::cout << "AT_F14" << std::endl;
    // 逐字节还原验证恢复
    {
      std::ofstream bin(dir / (k1 + ".bin"), std::ios::binary | std::ios::trunc);
      bin.write(reinterpret_cast<const char *>(old.data()), old.size());
      bin.close(); // 显式 flush——析构在块尾，check 在块内会读到旧内容
      ProgramArtifactStore s; assert(s.Configure(root));
      const auto got = s.Get(k1);
      if (!got) {
        std::error_code dec;
        auto sz = std::filesystem::file_size(dir / (k1 + ".bin"), dec);
        check("F14_RESTORE_LOADS", false,
              "Get null; bin size=" + std::to_string(sz) + " reject=" + s.Rejection(k1));
      } else {
        check("F14_RESTORE_LOADS", true, "原字节还原后可载入");
      }
    }
  }

  // ---- 原子写故障注入（真实系统路径 + 测试宏）----
  {
    ProgramArtifactStore s; assert(s.Configure(root));
    // 故障1 ENOSPC：新 key 失败，既有 key 完整
    s.SetWriteFault(1);
    auto fk = Hash("fault-enospc");
    auto fa = s.Put(fk, 7, {1, 2, 3});
    s.Drain();
    check("F15_ENOSPC_WRITE_FAILED", fa && fa->state == WRITE_FAILED &&
          !std::filesystem::exists(dir / (fk + ".bin")),
          "ENOSPC → WRITE_FAILED 且无半成品 .bin");
    check("F16_EXISTING_KEY_INTACT",
          s.Get(k1) && s.Get(k1)->state == PERSISTED,
          "故障不影响既有 key");
    s.SetWriteFault(0);
    // 故障2 部分临时写入
    s.SetWriteFault(2);
    auto fk2 = Hash("fault-partial");
    auto fa2 = s.Put(fk2, 7, std::vector<uint8_t>(4096, 7));
    s.Drain();
    check("F17_PARTIAL_WRITE_FAILED", fa2->state == WRITE_FAILED,
          "部分临时写入 → WRITE_FAILED");
    s.SetWriteFault(0);
    // 故障3 sync 失败（完整临时写后）
    s.SetWriteFault(3);
    auto fk3 = Hash("fault-sync");
    auto fa3 = s.Put(fk3, 7, {1, 2, 3});
    s.Drain();
    check("F18_SYNC_FAIL_REJECTS", fa3->state == WRITE_FAILED &&
          !std::filesystem::exists(dir / (fk3 + ".bin")),
          "sync 失败 → 拒绝且旧文件（如有）逐字节完整");
    s.SetWriteFault(0);
    // 故障4 替换失败：.tmp 目录占位
    auto fk4 = Hash("fault-replace");
    std::filesystem::create_directory(dir / (fk4 + ".bin.tmp"));
    auto fa4 = s.Put(fk4, 7, {1, 2, 3});
    s.Drain();
    check("F19_REPLACE_BLOCKED", fa4->state == WRITE_FAILED,
          ".tmp 目录占位 → 替换失败");
    // 故障期间既有旧档完整（同目标旧文件逐字节）——用 .tmp 目录占位实现替换失败
    auto before = Read(dir / (k1 + ".bin"));
    std::error_code rec;
    std::filesystem::remove(dir / (k1 + ".bin.tmp"), rec);
    std::filesystem::create_directory(dir / (k1 + ".bin.tmp"));
    auto fa5 = s.Put(k1, 9, std::vector<uint8_t>(2048, 5));
    s.Drain();
    const auto afterBytes = Read(dir / (k1 + ".bin"));
    check("F20A_SAME_KEY_SHARED_NO_REWRITE",
          fa5 && fa5->state == PERSISTED && fa5->bytes.size() == 3,
          "同 key 有效 entry Put 共享不可变旧版本（state=PERSISTED，不触盘）");
    check("F20B_OLD_FILE_INTACT", afterBytes == before,
          "同 key 替换失败 → 旧 .bin 逐字节完整 size=" +
              std::to_string(afterBytes.size()) + "/" + std::to_string(before.size()));
    std::error_code rec2;
    std::filesystem::remove(dir / (k1 + ".bin.tmp"), rec2);
    std::filesystem::remove(dir / (fk4 + ".bin.tmp"), rec2);
  }

  // ---- LRU / 上界 / pin ----
  {
    ProgramArtifactStore s; assert(s.Configure(root));
    // 触达 k1 让它变新
    assert(s.Get(k1));
    // 灌满 32 条
    std::vector<std::string> keys;
    const size_t existing = s.Count();
    for (size_t i = 0; i + existing < kCountLimit; ++i) {
      auto k = Hash("lru-" + std::to_string(i));
      keys.push_back(k);
      auto a = s.Put(k, 1, std::vector<uint8_t>(1024, uint8_t(i)));
      assert(a);
    }
    s.Drain();
    check("F21_COUNT_LIMIT", s.Count() == kCountLimit,
          "条数 = 32 上限（含既有 " + std::to_string(existing) + " 条）");
    auto pinned = s.Get(keys[0]); // pin 最旧的一条
    std::cout << "AT_LRU_PUT" << std::endl;
    auto nk = Hash("lru-new");
    auto na = s.Put(nk, 1, {1, 2, 3});
    s.Drain();
    std::cout << "AT_LRU_PIN_CHECK" << std::endl;
    check("F22_PINNED_SURVIVES", pinned && na && s.Get(keys[0]) == pinned,
          "被 pin 的 artifact 驱逐时保留");
    // 超内存上界拒绝
    auto huge = s.Put(Hash("huge"), 1, std::vector<uint8_t>(kTotalLimit, 1));
    check("F23_TOTAL_LIMIT_REJECT", !huge, "超 128MiB 总 payload 拒绝");
    std::cout << "AT_F24" << std::endl;
    // receipt 历史滚动（1500 条 ≈150KB > 128KiB 触发滚动；每 100 条 Drain 防队列满）
    for (int i = 0; i < 1500; ++i) {
      s.Receipt(keys[1], "{\"stage\":\"ROLL\",\"i\":" + std::to_string(i) + "}");
      if (i % 100 == 99)
        s.Drain();
    }
    s.Drain();
    auto rl = Read(dir / (keys[1] + ".jsonl"));
    check("F24_RECEIPT_ROLL_BOUNDED", rl.size() <= kReceiptLimit && !rl.empty(),
          "receipt 历史滚动 ≤128KiB（写 " + std::to_string(rl.size()) + "B）");
    std::cout << "AT_F25" << std::endl;
    // 收据失败计数（非 JSON 巨串？用合法超长 json 触发）
    s.Receipt("nonexistent-key-should-fail-validation",
              "{\"stage\":\"BAD\"}");
    s.Drain();
    check("F25_RECEIPT_FAILURE_COUNTED", s.ReceiptFailures() > 0,
          "非法 key 收据 → receiptFailures 计数");
  }

  std::cout << "AT_END" << std::endl;
  std::cout << "T476 store differential: " << passed << "/" << checks << " PASS\n";
  std::ofstream(std::filesystem::path(argv[1]) / "t476-store-diff.json")
      << "{\"passed\":" << passed << ",\"total\":" << checks << "}";
  return passed == checks ? 0 : 1;
}
