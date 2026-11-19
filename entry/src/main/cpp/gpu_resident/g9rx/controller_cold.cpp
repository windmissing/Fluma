#include "controller.h"
#include "implementation_generated.h"
namespace grs::g9rx {
namespace {
void H(uint32_t *at, const Digest &hash) {
  std::copy(hash.begin(), hash.end(), at);
}
Digest GeometryHash(const uint32_t *record) {
  if (!record[0])
    return {};
  p011gpu::Sha256 h;
  for (uint32_t w : {0x4b475247u, 1u, kProfileCode, 1u})
    HashWord(h, w);
  for (size_t i = 8; i < 26; ++i)
    HashWord(h, record[i]);
  for (size_t i = 56; i < 60; ++i)
    HashWord(h, record[i]);
  for (size_t i = 142; i < 144; ++i)
    HashWord(h, record[i]);
  for (size_t i = 100; i < 108; ++i)
    HashWord(h, record[i]);
  for (size_t i = 76; i < 82; ++i)
    HashWord(h, record[i]);
  HashWord(h, record[238]);
  for (size_t i = 144; i < 160; ++i)
    HashWord(h, record[i]);
  for (size_t i = 168; i < 176; ++i)
    HashWord(h, record[i]);
  return Finish(h);
}
} // namespace
bool Controller::CaptureCold(uint32_t *out, size_t words, bool full,
                             uint64_t request, uint64_t trace, uint64_t now,
                             uint32_t *journal) {
  uint64_t epoch = 0;
  QuotaSnapshotV3 quota{};
  {
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    if (!configured_ || !tail_[0] ||
        protocol_.payload.control[RxProtocol::ROOT_LIVE] || transport_.Busy() ||
        lease_ || sceneFence_ || coldPin_)
      return false;
    coldPin_ = true;
    epoch = protocol_.Get(RxProtocol::LIFECYCLE_GEN);
    quota = clock_.SnapshotLocked(
        now,
        protocol_.payload.control[RxProtocol::SIZE] &&
            protocol_.payload
                    .inputs[protocol_.payload.control[RxProtocol::HEAD]][40] ==
                1,
        run_);
  }
  bool ok = transport_.CaptureSpan(out, words, full, quota.data(), request,
                                   trace, now);
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  coldPin_ = false;
  if (!ok || protocol_.Get(RxProtocol::LIFECYCLE_GEN) != epoch ||
      protocol_.sync.lifecycleInvalidation.load() != epoch)
    return false;
  auto &payload = protocol_.payload;
  if (!journal)
    return false;
  for (size_t row = 0; row < 16; ++row)
    std::copy(payload.journal[row].begin(), payload.journal[row].end(),
              journal + row * 16);
  std::copy(protocol_.FrameCommit(17), protocol_.FrameCommit(17) + 4, out + 32);
  std::copy(protocol_.FrameCommit(18), protocol_.FrameCommit(18) + 4, out + 36);
  std::copy(payload.metadata[2].begin(), payload.metadata[2].end(), out + 40);
  H(out + 104, GeometryHash(payload.witnesses[17].data()));
  H(out + 112, GeometryHash(payload.witnesses[18].data()));
  H(out + 120, FromHex(kPipelineSHA));
  H(out + 128, FromHex(kLayoutSHA));
  H(out + 136,
    FromHex(
        "b17060c0441e481ec99f3279c76c8b8c94a1013b0fd9045d6b01b6cb827dac77"));
  H(out + 144, config_.parameterSha);
  H(out + 152, config_.seedSha);
  H(out + 160, FromHex(kClockCodeSHA));
  H(out + 168, FromHex(kMappingCodeSHA));
  p011gpu::Sha256 loaded;
  loaded.update(reinterpret_cast<const uint8_t *>(loaded_.receipt),
                std::strlen(loaded_.receipt));
  H(out + 176, Finish(loaded));
  H(out + 184, FromHex(kMappingSpecSHA));
  H(out + 192, FromHex(kInputPolicySHA));
  H(out + 200, FromHex(kClockSpecSHA));
  std::copy(config_.capsule64.begin() + 48, config_.capsule64.begin() + 56,
            out + 208);
  H(out + 216, loaded_.sha);
  out[224] = (payload.witnesses[17][0] ? 1u : 0u) |
             (payload.witnesses[18][0] ? 2u : 0u);
  out[225] = payload.witnesses[18][0] ? 3 : 0;
  uint64_t total = protocol_.Get(RxProtocol::JOURNAL_SEQ);
  Put(out, 226, total);
  out[228] = std::min<uint64_t>(16, total);
  out[229] = total % 16;
  Put(out, 230, total > 16 ? total - 16 : 0);
  std::copy(payload.control.begin() + 64, payload.control.begin() + 72,
            out + 232);
  H(out + 240, FromHex(kRxImplementationSHA));
  std::copy(payload.witnesses[17].begin(), payload.witnesses[17].end(),
            out + 3072);
  std::copy(payload.witnesses[18].begin(), payload.witnesses[18].end(),
            out + 3328);
  std::copy(config_.capsule64.begin(), config_.capsule64.end(), out + 3584);
  auto *seed = out + 3648;
  seed[0] = 1297306183;
  seed[1] = 1;
  seed[2] = 64;
  seed[3] = 3235799922u;
  std::copy(config_.capsule64.begin() + 48, config_.capsule64.begin() + 56,
            seed + 4);
  H(seed + 12, config_.seedSha);
  H(seed + 20, config_.parameterSha);
  Put(seed, 28, 5368709120ULL);
  Put(seed, 30, 1);
  seed[32] = 1;
  seed[33] = 1;
  Put(seed, 34, transport_.StateBytes());
  seed[36] = 1;
  return true;
}
} // namespace grs::g9rx
