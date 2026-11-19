#include "cold.h"
namespace grs::g13 {
Cold::Slot *Cold::Find(uint64_t id) {
  for (auto &s : slots_)
    if (s.id == id && s.words)
      return &s;
  return nullptr;
}
void Cold::Trim(uint64_t now) {
  for (auto &s : slots_)
    if (s.words && now >= s.created && now - s.created >= 60000000000ull) {
      s.words.reset();
      s.credit.Reset();
      s.id = 0;
      s.count = 0;
      s.ready = false;
    }
}
bool Cold::Receipt(uint32_t *out, size_t words, uint32_t tag,
                   const std::string &json) {
  if (words < 16 || json.size() > (words - 16) * 4)
    return false;
  out[0] = tag;
  out[1] = 1;
  out[2] = 1;
  out[3] = uint32_t(words);
  out[4] = uint32_t(json.size());
  auto hash = Hash(json.data(), json.size());
  std::copy(hash.begin(), hash.end(), out + 6);
  out[15] = kSeal;
  std::memcpy(out + 16, json.data(), json.size());
  return true;
}
std::string Cold::Begin(Controller &controller, const std::string &token,
                        bool full, const std::string &scene) {
  (void)scene;
  Trim(Now());
  if (Pending())
    return Result("g13ColdBegin", "WOULD_BLOCK", 34);
  if (token != controller.ColdToken())
    return Result("g13ColdBegin", "STALE_TOKEN", 31);
  if (controller.Gpu().Ticket()[127] != kSeal)
    return Result("g13ColdBegin", "NO_ROOT", 28);
  auto p = controller.Gpu().Layout();
  if (!p)
    return Result("g13ColdBegin", "NOT_READY", 28);
  int index = -1;
  for (size_t i = 0; i < slots_.size(); i++)
    if (!slots_[i].words) {
      index = int(i);
      break;
    }
  if (index < 0)
    return Result("g13ColdBegin", "WOULD_BLOCK", 34);
  auto &slot = slots_[size_t(index)];
  size_t total =
      16384 + 2 * p->sw + 18688 + (full ? p->sw + p->ww + 2 * p->pw : 0);
  uint64_t request = 0, id = 0;
  if (!slot.credit.Reserve(total * 4, 11510272, 13, request, id))
    return Result("g13ColdBegin", "WOULD_BLOCK", 34);
  slot.words.reset(new (std::nothrow) uint32_t[total]{});
  if (!slot.words) {
    slot.credit.Reset();
    return Result("g13ColdBegin", "ALLOCATION_FAILED", 30);
  }
  slot.count = total;
  slot.id = id;
  slot.request = request;
  slot.created = Now();
  slot.full = full;
  slot.ready = false;
  slot.token = token;
  pending_ = index;
  pendingOwner_ = &controller;
  controller.HoldCold(true);
  std::ostringstream out;
  out << "{\"route\":13,\"schema\":1,\"state\":\"REQUESTED\",\"id\":\"" << id
      << "\",\"request\":\"" << request << "\",\"rawBytes\":\"" << total * 4
      << "\",\"segments\":" << (full ? 8 : 4) << ",\"createdNativeNs\":\""
      << slot.created << "\"}";
  return reply_ = out.str();
}
void Cold::ReleasePending() {
  if (pendingOwner_)
    pendingOwner_->HoldCold(false);
  if (pending_ >= 0) {
    auto &slot = slots_[size_t(pending_)];
    slot.words.reset();
    slot.credit.Reset();
    slot.id = 0;
    slot.count = 0;
    slot.ready = false;
  }
  pending_ = -1;
  pendingOwner_ = nullptr;
}
void Cold::CancelPending(uint32_t error, const char *state) {
  if (!Pending())
    return;
  uint64_t id = slots_[size_t(pending_)].id;
  std::ostringstream out;
  out << "{\"route\":13,\"schema\":1,\"state\":" << Quote(state) << ",\"id\":\""
      << id << "\",\"error\":" << error << '}';
  reply_ = out.str();
  ReleasePending();
}
std::string Cold::Service(Controller &controller, const std::string &scene) {
  Trim(Now());
  if (!Pending())
    return {};
  auto &slot = slots_[size_t(pending_)];
  uint64_t now = Now();
  if (now < slot.created || now - slot.created >= 60000000000ull) {
    CancelPending(33, "EXPIRED");
    return reply_;
  }
  if (slot.token != controller.ColdToken()) {
    CancelPending(31, "STALE_TOKEN");
    return reply_;
  }
  if (!controller.RootIdle() || !controller.PinCold())
    return {};
  const uint64_t requestId = slot.id;
  std::string result;
  try {
    result = Build(controller, slot.full, scene);
  } catch (const std::exception &) {
    result = Result("g13ColdBegin", "BUILD_EXCEPTION", 30);
  }
  controller.UnpinCold();
  controller.HoldCold(false);
  pendingOwner_ = nullptr;
  if (slot.words && slot.ready) {
    pending_ = -1;
    reply_ = result;
  } else {
    std::ostringstream error;
    error << "{\"route\":13,\"schema\":1,\"state\":\"FAILED\",\"id\":\""
          << requestId << "\",\"detail\":" << result << '}';
    reply_ = error.str();
    ReleasePending();
  }
  return reply_;
}
std::string Cold::Build(Controller &controller, bool full,
                        const std::string &scene) {
  auto *slot = &slots_[size_t(pending_)];
  auto p = controller.Gpu().Layout();
  uint64_t now = slot->created, id = slot->id, request = slot->request;
  size_t total = slot->count;
  auto raw = std::move(slot->words);
  size_t written = 0;
  auto err =
      controller.CaptureSpan(full, raw.get() + 16384, total - 16384, written);
  controller.UnpinCold();
  if (err || written != total - 16384) {
    raw.reset();
    slot->credit.Reset();
    std::ostringstream failed;
    failed << "{\"route\":13,\"schema\":1,\"api\":\"g13ColdBegin\",\"state\":"
              "\"GPU_CAPTURE_FAILED\",\"error\":"
           << (err ? err : 28)
           << ",\"stage\":" << Quote(controller.Gpu().CaptureStage())
           << ",\"rawGL\":" << controller.Gpu().RawGl() << "}";
    return failed.str();
  }
  auto *h = raw.get();
  const auto &config = controller.Gpu().Configuration();
  const auto &tail = controller.LastCompletion();
  const auto &permit = controller.Gpu().Ticket();
  uint32_t count = full ? 8 : 4;
  h[0] = kMagic_cold;
  h[1] = 1;
  h[2] = kMagic_profile;
  h[3] = 256;
  h[4] = full ? 2 : 1;
  h[5] = tail[5] == 1 ? 0 : 1;
  h[6] = count;
  h[7] = uint32_t((total + 1023) / 1024);
  Put(h + 8, id);
  Put(h + 10, request);
  Put(h + 12, now);
  Put(h + 14, U64(permit.data() + 12));
  Put(h + 16, U64(permit.data() + 16));
  Put(h + 18, U64(permit.data() + 18));
  Put(h + 20, U64(permit.data() + 20));
  Put(h + 22, controller.Commit());
  Put(h + 24, U64(permit.data() + 8));
  Put(h + 26, total * 4);
  for (auto pair : std::array<std::pair<size_t, size_t>, 6>{
           {{32, 36}, {40, 44}, {48, 52}, {64, 76}, {72, 60}, {88, 68}}})
    std::copy_n(config.data() + pair.second, 8, h + pair.first);
  auto math = FromHex(kHash_mathSHA);
  std::copy(math.begin(), math.end(), h + 56);
  std::copy_n(tail.data() + 88, 8, h + 80);
  h[96] = p->nx;
  h[97] = p->ny;
  h[98] = config[6];
  h[99] = tail[4];
  h[100] = tail[5];
  h[101] = tail[7];
  h[102] = 1024;
  h[103] = tail[126];
  std::copy(config.begin(), config.end(), h + 1280);
  std::array<size_t, 8> lengths{};
  lengths[0] = 16384;
  lengths[1] = p->sw;
  lengths[2] = p->sw;
  if (full) {
    lengths[3] = p->sw;
    lengths[4] = p->ww;
    lengths[5] = 18688;
    lengths[6] = p->pw;
    lengths[7] = p->pw;
  } else
    lengths[3] = 18688;
  size_t offset = 0, ctrlOffset = 0;
  for (uint32_t i = 0; i < count; i++) {
    auto d = h + 256 + 32 * i;
    d[0] = i + 1;
    d[1] = 1;
    d[2] = i;
    d[4] = i == 1           ? controller.Gpu().CapturedBaseBank()
           : i == 2         ? controller.Gpu().CapturedCandidateBank()
           : full && i == 3 ? controller.Gpu().ReplayBank()
           : full && i >= 6 ? i - 6
                            : 0;
    d[5] = 1;
    Put(d + 6, offset * 4);
    Put(d + 8, lengths[i] * 4);
    d[10] = d[11] = d[12] = uint32_t(lengths[i]);
    Put(d + 13, controller.Commit());
    Put(d + 15, U64(permit.data() + 8));
    d[26] = 1;
    d[31] = kSeal;
    if (i) {
      auto hash = Hash(h + offset, lengths[i] * 4);
      std::copy(hash.begin(), hash.end(), d + 17);
    }
    if (i == (full ? 5u : 3u))
      ctrlOffset = offset;
    offset += lengths[i];
  }
  const auto *rawTail = h + ctrlOffset + 18432;
  const bool sealed = rawTail[0] == kMagic_tail && rawTail[255] == kSeal &&
                      U64(rawTail + 8) == U64(permit.data() + 8);
  const bool published = sealed && rawTail[5] == 1 &&
                         controller.Commit() == U64(rawTail + 26) &&
                         controller.AcceptedCompletion()[8] == rawTail[8] &&
                         controller.AcceptedCompletion()[9] == rawTail[9];
  const bool diagnostic =
      sealed && rawTail[5] == 7 && (rawTail[126] == 1 || rawTail[126] == 3);
  h[5] = (published || diagnostic) ? 0u : 1u;
  h[99] = permit[1];
  h[100] = sealed ? rawTail[5] : 0u;
  h[101] = sealed ? rawTail[7] : 0u;
  h[102] = sealed ? 1024u : 0u;
  h[103] = permit[92];
  std::copy_n(permit.data() + 72, 8, h + 80);
  for (uint32_t i = 1; i < count; i++) {
    auto *d = h + 256 + 32 * i;
    const size_t start = size_t(U64(d + 6)) / 4;
    if (i <= 2u || (full && i == 3u))
      Put(d + 13, U64(h + start + 16));
    else if (full && i >= 6u)
      Put(d + 13, U64(h + start + 16));
  }
  std::copy_n(h + ctrlOffset + 1024, 1024, h + 1408);
  std::copy_n(h + ctrlOffset + 256, 256, h + 4736);
  std::copy(permit.begin(), permit.end(), h + 5760);
  bool baseReceipt = Receipt(h + 5888, 128, 7,
                             std::string("{\"acceptedBaseToken\":") +
                                 Quote(controller.BaseToken()) + "}");
  auto diagnosticJson = controller.RootReceipt();
  diagnosticJson.pop_back();
  const auto *global = h + ctrlOffset;
  const uint32_t firstMath = global[750], aggregateMath = global[751],
                 firstSeal = global[766];
  const bool firstObserved =
      sealed && controller.GpuTailObserved() && global[513] < 64u &&
      firstSeal == 0xc0de1311u && U64(global + 733) == U64(permit.data() + 8) &&
      global[514] == rawTail[7] && global[513] == rawTail[6] &&
      global[515] == rawTail[122] && global[516] == rawTail[123] &&
      global[517] == rawTail[124];
  const char *reason = controller.RootError() == 17u
                           ? (!firstObserved      ? "MATH_DETAIL_NOT_OBSERVED"
                              : (firstMath & ~7u) ? "UNSUPPORTED_MATH_STATUS"
                              : firstMath         ? "NUMERIC_DOMAIN"
                                                  : "NUMERIC_OVERFLOW")
                       : controller.RootError() ? "NON_MATH_FIRST_ERROR"
                                                : "NONE";
  diagnosticJson +=
      ",\"firstMathStatus\":" +
      (firstObserved ? std::to_string(firstMath) : "null") +
      ",\"aggregateMathStatus\":" + std::to_string(aggregateMath) +
      ",\"firstFaultDetailSeal\":" + std::to_string(firstSeal) +
      ",\"firstFaultObserved\":" + (firstObserved ? "true" : "false") +
      ",\"firstCause\":" + Quote(reason) + ",\"firstFaultTuple\":[";
  for (size_t i = 0; i < 7; i++) {
    if (i)
      diagnosticJson += ',';
    diagnosticJson += std::to_string(i < 2    ? global[733 + i]
                                     : i == 2 ? global[513]
                                     : i < 6  ? global[512 + i]
                                              : global[514]);
  }
  diagnosticJson += "],\"aliasInjection\":[";
  for (size_t i = 0; i < 8; i++) {
    if (i)
      diagnosticJson += ',';
    diagnosticJson += std::to_string(global[828 + i]);
  }
  diagnosticJson += "]}";
  bool diagnosticReceipt = Receipt(h + 5504, 256, 6, diagnosticJson);
  bool receipts =
      Receipt(h + 2432, 512, 1, controller.Gpu().Capabilities()) &&
      Receipt(h + 2944, 512, 2,
              std::string("{\"backend\":\"SOFT_BINARY128_CONSTANT_RANGE_"
                          "REDUCTION\",\"mathSHA\":\"") +
                  kHash_mathSHA + "\",\"qualification\":\"NOT_MEASURED\"}") &&
      Receipt(h + 3456, 1024, 3, controller.Gpu().CompileReceipt()) &&
      Receipt(h + 4480, 256, 4, scene) &&
      Receipt(
          h + 4992, 512, 5,
          std::string("{\"sourceSHA\":\"") + PipelineSha() +
              "\",\"testHooks\":" + (PipelineTestHooks() ? "true" : "false") +
              ",\"resources\":" + controller.Gpu().Resources() + "}");
  if (!receipts || !baseReceipt || !diagnosticReceipt) {
    raw.reset();
    slot->credit.Reset();
    return Result("g13ColdBegin", "RECEIPT_CAPACITY", 30);
  } // The qualification sample block is copied verbatim from the actual GPU
    // root trace.
  std::ostringstream mathReceipt;
  mathReceipt << "{\"backend\":\"SOFT_BINARY128\",\"scope\":\"BOOT_SAMPLES_"
                 "ONLY\",\"qualificationWords\":[";
  for (size_t i = 0; i < 128; i++) {
    if (i)
      mathReceipt << ',';
    mathReceipt << h[ctrlOffset + 2048 + 768 + i];
  }
  mathReceipt << "],\"firstFaultScalarObserved\":"
              << (firstObserved && firstMath ? "true" : "false")
              << ",\"firstFaultScalarWords\":[";
  if (firstObserved && firstMath)
    for (size_t i = 0; i < 17; i++) {
      if (i)
        mathReceipt << ',';
      mathReceipt << global[733 + i];
    }
  mathReceipt
      << "],\"argumentOrigin\":\"OBSERVED_IF_FLAGGED_ELSE_DERIVED_FROM_CONFIG_"
         "AND_ACCEPTED_TICK\",\"globalPrecisionProof\":\"NOT_MEASURED\"}";
  if (!Receipt(h + 2944, 512, 2, mathReceipt.str())) {
    raw.reset();
    slot->credit.Reset();
    return Result("g13ColdBegin", "MATH_RECEIPT_CAPACITY", 30);
  }
  auto hash = Hash(h, 65536);
  std::copy(hash.begin(), hash.end(), h + 248);
  slot->words = std::move(raw);
  slot->count = total;
  slot->id = id;
  slot->request = request;
  slot->created = now;
  slot->segments = count;
  slot->nextSegment = 0;
  slot->nextWord = 0;
  slot->ready = true;
  slot->credit.BuilderDone();
  std::ostringstream out;
  out << "{\"route\":13,\"schema\":1,\"state\":\"READY\",\"id\":\"" << id
      << "\",\"request\":\"" << request << "\",\"mode\":\""
      << (full ? "FULL" : "ROOT") << "\",\"rawBytes\":\"" << total * 4
      << "\",\"segments\":" << count << ",\"createdNativeNs\":\"" << now
      << "\",\"ttlNs\":\"60000000000\"}";
  return out.str();
}
std::string Cold::Page(uint64_t id, uint32_t segment, size_t wordOffset) {
  Trim(Now());
  auto s = Find(id);
  if (!s)
    return Result("g13ColdPage", "EXPIRED", 33);
  if (!s->ready)
    return Result("g13ColdPage", "BUILDING", 34);
  if (segment != s->nextSegment || wordOffset != s->nextWord ||
      segment >= s->segments)
    return Result("g13ColdPage", "OUT_OF_ORDER", 5);
  if (!s->credit.ReadBegin())
    return Result("g13ColdPage", "READER_BUSY", 34);
  struct ReaderPin {
    coldbroker::Lease &lease;
    ~ReaderPin() { lease.ReadEnd(); }
  } reader{s->credit};
  auto d = s->words.get() + 256 + 32 * segment;
  size_t start = size_t(U64(d + 6)) / 4, length = size_t(U64(d + 8)) / 4;
  if (wordOffset >= length) {
    return Result("g13ColdPage", "OUT_OF_RANGE", 5);
  }
  size_t count = std::min(size_t(1024), length - wordOffset);
  std::ostringstream out;
  out << "{\"route\":13,\"schema\":1,\"state\":\"PAGE\",\"id\":\"" << id
      << "\",\"segment\":" << segment << ",\"wordOffset\":\"" << wordOffset
      << "\",\"rawByteOffset\":\"" << (start + wordOffset) * 4
      << "\",\"words\":[";
  for (size_t i = 0; i < count; i++) {
    if (i)
      out << ',';
    out << s->words[start + wordOffset + i];
  }
  out << "]}";
  std::string result = out.str();
  if (result.size() > 16384)
    return Result("g13ColdPage", "PAGE_UTF8_LIMIT", 30);
  s->nextWord += count;
  if (s->nextWord == length) {
    ++s->nextSegment;
    s->nextWord = 0;
  }
  return result;
}
std::string Cold::End(uint64_t id) {
  if (Pending() && slots_[size_t(pending_)].id == id) {
    CancelPending(11, "CANCELLED");
    return reply_;
  }
  auto s = Find(id);
  if (!s)
    return Result("g13ColdEnd", "EXPIRED", 33);
  bool complete = s->nextSegment == s->segments;
  s->words.reset();
  s->credit.Reset();
  s->id = 0;
  s->count = 0;
  return Result("g13ColdEnd", complete ? "CLOSED" : "INCOMPLETE",
                complete ? 0 : 28);
}
} // namespace grs::g13
