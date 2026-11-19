#include "cold_generated.h"
#include "transport.h"
#include <cstring>
namespace grs::g9rx {
bool Transport::CaptureSpan(uint32_t *out, size_t words, bool full,
                            const uint32_t *quota, uint64_t request,
                            uint64_t trace, uint64_t created) {
  if (!ready_ || busy_ || !Current() || !profile_ || !out || !quota ||
      full && !config_.fullTrialTrace || tail_[0] != kCompletionMagic ||
      tail_[6] == 5)
    return false;
  const ColdSegment *segments =
      profile_->k == 192 ? (full ? kCold192FULL.data() : kCold192ROOT.data())
                         : (full ? kCold512FULL.data() : kCold512ROOT.data());
  size_t count = full ? 10 : 4,
         bytes = segments[count - 1].byteStart + segments[count - 1].bytes;
  if (words != bytes / 4)
    return false;
  std::fill(out, out + words, 0u);
  out[0] = 1129861703;
  out[1] = 1;
  out[2] = 8192;
  out[3] = 3235799921u;
  out[4] = kProfileCode;
  out[5] = full;
  out[6] = count;
  out[7] = profile_->k;
  Put(out, 8, bytes);
  Put(out, 10, request);
  Put(out, 12, trace);
  Put(out, 14, created);
  Put(out, 16, permit_.identity.run);
  Put(out, 18, permit_.identity.context);
  Put(out, 20, permit_.identity.surface);
  Put(out, 22, permit_.identity.pageSession);
  Put(out, 24, permit_.identity.inputEpoch);
  Put(out, 26, permit_.nonce);
  out[28] = tail_[6];
  out[29] = tail_[37];
  out[30] = tail_[44];
  out[31] = tail_[46];
  std::copy(tail_.begin(), tail_.end(), out + 256);
  std::copy(lastInput_.begin(), lastInput_.end(), out + 512);
  std::copy(lastConfig_.begin(), lastConfig_.end(), out + 640);
  std::copy(quota, quota + 256, out + 768);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
  glBindBuffer(GL_COPY_READ_BUFFER, buffers_[5]);
  const auto *summary = static_cast<const uint32_t *>(glMapBufferRange(
      GL_COPY_READ_BUFFER, profile_->work[6].begin, 1536, GL_MAP_READ_BIT));
  if (!summary)
    return false;
  std::memcpy(out + 1024, summary, 1536);
  if (glUnmapBuffer(GL_COPY_READ_BUFFER) != GL_TRUE || !Good())
    return false;
  for (size_t i = 0; i < count; ++i) {
    const auto &seg = segments[i];
    bool executed =
        i < 2 || (i < 4 ? tail_[6] != 4 : out[1024 + (i - 4) * 64] != 0);
    if (executed) {
      uint32_t kind = i < 2   ? 0
                      : i < 4 ? 1
                              : 2,
               index = i == 0   ? 1
                       : i == 1 ? current_
                       : i == 2 ? 1
                       : i == 3 ? 2 + permit_.arena
                                : i - 4;
      if (!CaptureBuffer(out + seg.byteStart / 4, seg.bytes / 4, kind, index))
        return false;
    }
    auto *d = out + 2048 + i * 32;
    d[0] = i + 1;
    d[1] = i >= 4 ? i - 4 : 0;
    d[2] = i == 2 ? 1 : i == 3 ? 2 : i >= 4 ? 1 : 0;
    d[3] = executed ? 2 : 1;
    d[4] = i >= 4 ? out[1024 + (i - 4) * 64] : tail_[37];
    d[5] = !executed ? 0 : i >= 4 ? out[1024 + (i - 4) * 64 + 7] : tail_[44];
    d[6] = !executed ? 0 : i >= 4 ? out[1024 + (i - 4) * 64 + 8] : tail_[46];
    Put(d, 8, seg.byteStart / 4);
    Put(d, 10, seg.bytes / 4);
    d[12] = 1;
    auto hash = HashWords(out + seg.byteStart / 4, seg.bytes / 4);
    std::copy(hash.begin(), hash.end(), d + 23);
    d[31] = kSeal;
  }
  return Good();
}
} // namespace grs::g9rx
