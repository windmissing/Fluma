#pragma once
#include "contract_generated.h"
#include "receipt_clock.h"
#include "receipt_recipe.h"
#include "rx_storage.h"
#include "wire.h"
#include <algorithm>
#include <cstdio>
namespace grs::g9rx {
class RxProtocol {
public:
  RxProtocolPayload payload{};
  RxSync sync{};
  enum ControlWord : uint32_t {
    ACTIVE = 0,
    STROKE = 1,
    POINTER = 2,
    MOVES = 3,
    HEAD = 4,
    SIZE = 5,
    CLAIMED = 6,
    CLAIM_SLOT = 7,
    COMMAND_SEQ = 8,
    CLOCK_GEN = 10,
    LIFECYCLE_GEN = 12,
    INGRESS_SEQ = 14,
    LAST_INGRESS = 16,
    LAST_OWNER = 18,
    STROKE_CANCEL = 20,
    JOURNAL_SEQ = 22,
    FIRST_ERROR = 24,
    ERROR_SEQ = 26,
    HAS_LAST = 28,
    BASE_CAPTURE = 29,
    ROOT_LIVE = 30
  };
  uint64_t Get(size_t at) const { return Pair(payload.control.data(), at); }
  void Set(size_t at, uint64_t value) {
    Put(payload.control.data(), at, value);
  }
  uint32_t *Raw(size_t n) { return payload.bridge.data() + 64 * n; }
  uint32_t *MappingScratch(size_t n) {
    return payload.bridge.data() + 976 + 64 * n;
  }
  uint32_t *FrameCommit(size_t slot) {
    return payload.bridge.data() + 896 + 4 * slot;
  }
  const Words64 &Capability() const { return payload.metadata[2]; }
  const Words64 &Mapping() const { return payload.metadata[3]; }
  void Error(uint32_t code) {
    if (!payload.control[FIRST_ERROR])
      payload.control[FIRST_ERROR] = code;
    uint64_t n;
    if (Add(Get(ERROR_SEQ), 1, n))
      Set(ERROR_SEQ, n);
    else {
      payload.control[ACTIVE] = payload.control[STROKE] = 0;
      sync.ownerActive.store(false);
      payload.metadata[2][3] = payload.metadata[3][2] = 0;
    }
  }
  void Revoke(uint32_t code) {
    if (!payload.control[77] && firstClockApiStage) {
      payload.control[76] = uint32_t(firstClockApiError);
      payload.control[77] = firstClockApiStage;
    }
    payload.control[ACTIVE] = payload.control[STROKE] = 0;
    payload.metadata[2][3] = 0;
    payload.metadata[3][2] = 0;
    sync.ownerActive.store(false);
    sync.Bump(sync.lifecycleInvalidation);
    if (!payload.control[ROOT_LIVE]) {
      payload.control[SIZE] = payload.control[HEAD] = payload.control[CLAIMED] =
          0;
    }
    Error(code);
  }
  void Journal(uint32_t kind, uint32_t status, uint64_t now, uint64_t seq = 0,
               uint64_t nonce = 0, uint64_t receipt = 0, uint64_t commit = 0,
               uint64_t publication = 0) {
    uint64_t ordinal = Get(JOURNAL_SEQ);
    if (ordinal == UINT64_MAX) {
      Revoke(4);
      return;
    }
    auto &row = payload.journal[ordinal % 16];
    row = {};
    row[0] = kind;
    row[1] = status;
    Put(row.data(), 2, Get(CLOCK_GEN));
    Put(row.data(), 4, seq);
    Put(row.data(), 6, nonce);
    Put(row.data(), 8, now);
    Put(row.data(), 10, receipt);
    Put(row.data(), 12, commit);
    Put(row.data(), 14, publication);
    p011gpu::Sha256 rolling;
    for (size_t i = 64; i < 72; ++i)
      HashWord(rolling, payload.control[i]);
    for (auto w : row)
      HashWord(rolling, w);
    auto digest = Finish(rolling);
    std::copy(digest.begin(), digest.end(), payload.control.begin() + 64);
    Set(JOURNAL_SEQ, ordinal + 1);
  }
  std::string Reply(const char *state, uint32_t code, uint64_t receipt = 0,
                    uint64_t elapsed = 0) {
    char *out = reinterpret_cast<char *>(payload.bridge.data() + 128);
    int n = std::snprintf(
        out, 2048,
        "{\"revision\":\"G9_RX_REPLY_V1\",\"boot\":\"%llu\",\"attempt\":\"%"
        "llu\",\"scopeRun\":\"%llu\",\"scopeEpoch\":\"%llu\",\"profile\":"
        "\"GNR1\",\"domain\":\"NATIVE_RECEIPT_SPATIAL_CONTROL\",\"state\":\"%"
        "s\",\"code\":%u,\"clockGeneration\":\"%llu\",\"lifecycleGeneration\":"
        "\"%llu\",\"ingressSeq\":\"%llu\",\"receiptNs\":\"%llu\",\"elapsedNs\":"
        "\"%llu\",\"sampleAge\":\"UNKNOWN\",\"queue\":%u,\"firstFailure\":%u}",
        (unsigned long long)sync.boot, (unsigned long long)Get(INGRESS_SEQ),
        (unsigned long long)Pair(Capability().data(), 10),
        (unsigned long long)Pair(Capability().data(), 16), state, code,
        (unsigned long long)Get(CLOCK_GEN),
        (unsigned long long)Get(LIFECYCLE_GEN),
        (unsigned long long)Get(INGRESS_SEQ), (unsigned long long)receipt,
        (unsigned long long)elapsed, payload.control[SIZE],
        payload.control[FIRST_ERROR]);
    if (n <= 0 || n >= 2048)
      return "{\"state\":\"REPLY_LIMIT\"}";
    std::string reply(out, n);
    reply.pop_back();
    reply += ",\"scope\":[";
    for (size_t i = 0; i < 9; ++i) {
      if (i)
        reply += ',';
      reply +=
          '"' + std::to_string(Pair(Capability().data(), 10 + 2 * i)) + '"';
    }
    reply += "],\"commandSeq\":\"" + std::to_string(Get(COMMAND_SEQ)) +
             "\",\"lastFrozenNonce\":\"" +
             std::to_string(Pair(payload.witnesses[17].data(), 140)) + "\"}";
    return reply.size() < 2048 ? reply : "{\"state\":\"REPLY_LIMIT\"}";
  }
  bool SnapshotMappingBeforeParse(CheckedTime time) {
    std::unique_lock<std::mutex> lock(sync.protocol, std::try_to_lock);
    if (!lock.owns_lock())
      return false;
    if (!RoundingQualified()) {
      Revoke(43);
      return false;
    }
    if (!time.ok) {
      Revoke(1);
      return false;
    }
    if (sync.mappingPending.load() || (payload.metadata[2][3] & 31) != 31 ||
        payload.metadata[3][2] != 7 || !payload.control[ACTIVE] ||
        !sync.ownerActive.load())
      return false;
    uint64_t present = Pair(payload.scratch.data(), 252);
    if (time.ns < present) {
      Error(2);
      return false;
    }
    std::copy(payload.metadata[3].begin(), payload.metadata[3].end(),
              MappingScratch(0));
    std::copy(payload.metadata[2].begin(), payload.metadata[2].end(),
              MappingScratch(1));
    return true;
  }
  bool ScopeEcho(const uint32_t *raw) const {
    return Pair(raw, 38) == Pair(Capability().data(), 16) &&
           Pair(raw, 40) == Pair(Capability().data(), 18) &&
           Pair(raw, 42) == Get(CLOCK_GEN) &&
           Pair(raw, 44) == Get(LIFECYCLE_GEN) &&
           std::equal(raw + 48, raw + 56, Mapping().data() + 48);
  }
  bool CurrentAge(uint64_t now, uint64_t receipt) const {
    return now >= receipt && now - receipt <= 250000000;
  }
  void MoveHash(uint32_t domain, const uint32_t *raw, uint64_t seq,
                uint64_t receipt, bool first, bool last) {
    auto &record = payload.witnesses[16];
    p011gpu::Sha256 h;
    HashWord(h, domain);
    HashWord(h, 1);
    if (first) {
      HashWord(h, 827477575);
      HashWord(h, uint32_t(seq));
      HashWord(h, uint32_t(seq >> 32));
      HashWord(h, uint32_t(Get(CLOCK_GEN)));
      HashWord(h, uint32_t(Get(CLOCK_GEN) >> 32));
    } else {
      for (size_t i = 208; i < 216; ++i)
        HashWord(h, record[i]);
      HashWord(h, uint32_t(seq));
      HashWord(h, uint32_t(seq >> 32));
      HashWord(h, uint32_t(receipt));
      HashWord(h, uint32_t(receipt >> 32));
      if (last)
        HashWord(h, payload.control[MOVES]);
    }
    for (size_t i = 0; i < 64; ++i)
      HashWord(h, raw[i]);
    auto hash = Finish(h);
    std::copy(hash.begin(), hash.end(), record.begin() + 208);
  }
  std::string ValidateStroke(const uint32_t *raw, CheckedTime receipt) {
    auto ready = CheckedClockRead();
    std::unique_lock<std::mutex> lock(sync.protocol, std::try_to_lock);
    if (!lock.owns_lock())
      return "{\"state\":\"INGRESS_BUSY\"}";
    if (!ScopeEcho(raw))
      return Reply("STALE_SCOPE", 3, receipt.ns);
    if (!RoundingQualified()) {
      Revoke(43);
      return Reply("ROUNDING_MODE_UNSUPPORTED", 43, receipt.ns);
    }
    if (!ready.ok || !receipt.ok || ready.ns < receipt.ns) {
      Revoke(1);
      return Reply("CLOCK_READ_FAILED_OR_REVERSED", 1, receipt.ns);
    }
    if (receipt.ns < Get(LAST_INGRESS)) {
      Revoke(1);
      return Reply("CLOCK_REVERSED", 1, receipt.ns);
    }
    Set(LAST_INGRESS, receipt.ns);
    uint64_t seq;
    if (!Add(Get(INGRESS_SEQ), 1, seq)) {
      Revoke(4);
      return Reply("INGRESS_ID_EXHAUSTED", 4, receipt.ns);
    }
    Set(INGRESS_SEQ, seq);
    auto reject = [&](const char *why, uint32_t code) {
      payload.control[STROKE] = 0;
      Error(code);
      return Reply(why, code, receipt.ns, ready.ns - receipt.ns);
    };
    if (ready.ns - receipt.ns > 8000000)
      return reject("VALIDATION_DEADLINE", 5);
    if (raw[0] != 1347899975 || raw[1] != 1 || raw[2] != 64 || raw[7] != 1 ||
        raw[3] > 3 || raw[11] || raw[10] > 1 ||
        (!raw[10] && (raw[8] || raw[9])))
      return reject("BAD_RAW64", 6);
    for (size_t i = 56; i < 64; ++i)
      if (raw[i])
        return reject("RESERVED_WORD", 6);
    if (raw[3] == 3) {
      for (size_t i = 4; i < 30; ++i)
        if (i != 7 && raw[i])
          return reject("BAD_CANCEL", 6);
      payload.control[STROKE] = 0;
      return Reply("CANCELLED", 0, receipt.ns);
    }
    if (raw[5] != 1 || raw[6] > 1 || (raw[3] != 2 && raw[6] != 1))
      return reject("MULTITOUCH_OR_CHANGED_POINT", 7);
    const auto *map = MappingScratch(0);
    if (!std::equal(raw + 30, raw + 38, map + 10) ||
        Pair(raw, 46) != Pair(map, 8) || map[2] != 7 ||
        payload.metadata[2][3] != 31 || sync.mappingPending.load() ||
        Mapping()[2] != 7 || Pair(map, 8) != Pair(Mapping().data(), 8) ||
        !std::equal(map + 10, map + 18, Mapping().data() + 10))
      return reject("GEOMETRY_MISMATCH", 8);
    double x, y;
    if (!Map(raw, map, x, y))
      return reject("COORDINATE_POLICY", 9);
    auto &draft = payload.witnesses[16];
    if (raw[3] == 0) {
      if (payload.control[STROKE])
        return reject("DUPLICATE_DOWN", 10);
      draft = {};
      std::copy(Capability().begin() + 10, Capability().begin() + 28,
                draft.begin() + 8);
      payload.control[STROKE] = 1;
      payload.control[POINTER] = raw[4];
      payload.control[MOVES] = 0;
      Set(STROKE_CANCEL, sync.strokeCancelGeneration.load());
      Put(draft.data(), 30, seq);
      Put(draft.data(), 34, receipt.ns);
      Put(draft.data(), 40, ready.ns);
      std::copy(raw + 12, raw + 30, draft.begin() + 64);
      std::copy(raw + 8, raw + 10, draft.begin() + 120);
      draft[124] = raw[10];
      RealPut(draft.data(), 108, x);
      RealPut(draft.data(), 110, y);
      std::copy(map + 4, map + 8, draft.begin() + 56);
      Put(draft.data(), 142, Pair(map, 8));
      draft[238] = map[44];
      std::copy(map + 30, map + 38, draft.begin() + 100);
      Put(draft.data(), 60, Pair(map, 28));
      std::copy(map + 18, map + 26, draft.begin() + 192);
      Put(FrameCommit(16), 0, Pair(map, 26));
      std::copy(map + 48, map + 64, draft.begin() + 144);
      MoveHash(0x444d5247, raw, seq, receipt.ns, true, false);
      auto end = CheckedClockRead();
      if (!end.ok || end.ns < ready.ns) {
        Revoke(1);
        return Reply("CLOCK_REVERSED", 1, receipt.ns);
      }
      if (end.ns - receipt.ns > 8000000)
        return reject("VALIDATION_DEADLINE", 5);
      Put(draft.data(), 40, end.ns);
      return Reply("DOWN", 0, receipt.ns, ready.ns - receipt.ns);
    }
    if (!payload.control[STROKE])
      return Reply("IDLE", 0, receipt.ns);
    if (raw[4] != payload.control[POINTER] ||
        Get(STROKE_CANCEL) != sync.strokeCancelGeneration.load() ||
        !sync.ownerActive.load())
      return reject("POINTER_OR_CONCURRENT_CANCEL", 11);
    if (raw[3] == 1) {
      if (payload.control[MOVES] >= 256)
        return reject("MOVE_LIMIT", 12);
      ++payload.control[MOVES];
      MoveHash(0x4d4d5247, raw, seq, receipt.ns, false, false);
      auto end = CheckedClockRead();
      if (!end.ok || end.ns < ready.ns) {
        Revoke(1);
        return Reply("CLOCK_REVERSED", 1, receipt.ns);
      }
      if (end.ns - receipt.ns > 8000000)
        return reject("VALIDATION_DEADLINE", 5);
      return Reply("MOVE_OBSERVED_NO_FORCE", 0, receipt.ns,
                   ready.ns - receipt.ns);
    }
    payload.control[STROKE] = 0;
    uint64_t down = Pair(draft.data(), 34);
    if (receipt.ns < down || receipt.ns - down < 8333334 ||
        receipt.ns - down > 1000000000)
      return reject("RECEIPT_DURATION", 13);
    if (Pair(draft.data(), 142) != Pair(map, 8) || draft[238] != map[44] ||
        !std::equal(draft.begin() + 56, draft.begin() + 60, map + 4) ||
        !std::equal(draft.begin() + 144, draft.begin() + 160, map + 48))
      return reject("GEOMETRY_CHANGED", 8);
    if (payload.control[SIZE] >= 16)
      return reject("FIFO_FULL", 14);
    SpatialRecipe recipe;
    if (!Recipe(x - Real(draft.data(), 108), y - Real(draft.data(), 110),
                recipe))
      return Reply("NO_IMPULSE", 0, receipt.ns);
    uint64_t command;
    if (!Add(Get(COMMAND_SEQ), 1, command)) {
      Revoke(4);
      return Reply("COMMAND_ID_EXHAUSTED", 4, receipt.ns);
    }
    Set(COMMAND_SEQ, command);
    Put(draft.data(), 26, command);
    draft[28] = raw[4];
    draft[29] = 1;
    Put(draft.data(), 32, seq);
    Put(draft.data(), 36, receipt.ns);
    Put(draft.data(), 38, receipt.ns - down);
    Put(draft.data(), 42, ready.ns);
    Put(draft.data(), 44, ready.ns);
    Put(draft.data(), 52, Get(CLOCK_GEN));
    Put(draft.data(), 54, Get(LIFECYCLE_GEN));
    Put(draft.data(), 62, Pair(map, 28));
    std::copy(raw + 12, raw + 30, draft.begin() + 82);
    RealPut(draft.data(), 112, x);
    RealPut(draft.data(), 114, y);
    RealPut(draft.data(), 116, x - Real(draft.data(), 108));
    RealPut(draft.data(), 118, y - Real(draft.data(), 110));
    std::copy(raw + 8, raw + 10, draft.begin() + 122);
    draft[124] |= raw[10] << 1;
    draft[125] = 1;
    Put(draft.data(), 126, 125000000);
    Put(draft.data(), 128, uint64_t(recipe.jx));
    Put(draft.data(), 130, recipe.jy);
    for (size_t i = 0; i < 4; ++i)
      draft[132 + i] = recipe.cap >> (32 * i);
    draft[136] = recipe.flags & 3;
    draft[137] = payload.control[MOVES];
    draft[139] = Capability()[5];
    Put(draft.data(), 142, Pair(map, 8));
    std::copy(map + 18, map + 26, draft.begin() + 200);
    Put(FrameCommit(16), 2, Pair(map, 26));
    auto clockSha = FromHex(kClockSpecSHA),
         mappingSha = FromHex(kMappingSpecSHA),
         policySha = FromHex(kInputPolicySHA);
    std::copy(clockSha.begin(), clockSha.end(), draft.begin() + 160);
    std::copy(mappingSha.begin(), mappingSha.end(), draft.begin() + 168);
    std::copy(policySha.begin(), policySha.end(), draft.begin() + 216);
    std::copy(Capability().begin() + 28, Capability().begin() + 36,
              draft.begin() + 224);
    draft[238] = map[44];
    MoveHash(0x554d5247, raw, seq, receipt.ns, false, true);
    uint32_t at = (payload.control[HEAD] + payload.control[SIZE]) % 16;
    auto &input = payload.inputs[at];
    input = {};
    input[0] = 1481788999;
    input[1] = 3;
    input[2] = 827477575;
    Put(input.data(), 4, Pair(draft.data(), 8));
    Put(input.data(), 12, Pair(draft.data(), 14));
    std::copy(draft.begin() + 144, draft.begin() + 152, input.begin() + 26);
    input[40] = 2;
    Put(input.data(), 41, command);
    Put(input.data(), 43, receipt.ns);
    input[45] = recipe.flags;
    RealPut(input.data(), 46, Real(draft.data(), 108));
    RealPut(input.data(), 48, 4);
    std::copy(draft.begin() + 116, draft.begin() + 120, input.begin() + 50);
    Put(input.data(), 54, 125000000);
    Put(input.data(), 56, uint64_t(recipe.jx));
    Put(input.data(), 58, recipe.jy);
    std::copy(draft.begin() + 132, draft.begin() + 136, input.begin() + 60);
    input[77] = 63701;
    p011gpu::Sha256 stable;
    for (uint32_t w :
         {0x43535247u, 1u, input[2], input[4], input[5], input[12], input[13]})
      HashWord(stable, w);
    for (size_t i = 26; i < 34; ++i)
      HashWord(stable, input[i]);
    for (size_t i = 40; i < 128; ++i)
      HashWord(stable, input[i]);
    auto hash = Finish(stable);
    std::copy(hash.begin(), hash.end(), draft.begin() + 184);
    Put(draft.data(), 234, ready.ns - receipt.ns);
    Put(draft.data(), 236, ready.ns - receipt.ns);
    auto end = CheckedClockRead();
    if (!end.ok || end.ns < ready.ns) {
      Revoke(1);
      return Reply("CLOCK_REVERSED", 1, receipt.ns);
    }
    if (end.ns - receipt.ns > 8000000)
      return reject("VALIDATION_DEADLINE", 5);
    if (Get(STROKE_CANCEL) != sync.strokeCancelGeneration.load() ||
        !sync.ownerActive.load())
      return reject("CONCURRENT_CANCEL", 11);
    ready = end;
    Put(draft.data(), 42, ready.ns);
    Put(draft.data(), 44, ready.ns);
    Put(draft.data(), 234, ready.ns - receipt.ns);
    Put(draft.data(), 236, ready.ns - receipt.ns);
    payload.witnesses[at] = draft;
    std::copy(FrameCommit(16), FrameCommit(16) + 4, FrameCommit(at));
    ++payload.control[SIZE];
    Journal(1, 0, ready.ns, command, 0, receipt.ns);
    return Reply("QUEUED", 0, receipt.ns, ready.ns - receipt.ns);
  }
};
} // namespace grs::g9rx
