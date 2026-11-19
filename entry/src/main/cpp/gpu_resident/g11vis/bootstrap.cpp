#include "bootstrap.h"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <new>
#include <sstream>
namespace grs::g11vis {
namespace {
std::mutex witnessMutex;
uint64_t witnessSerial = 0, witnessTicket = 0, witnessContext = 0;
size_t witnessBytes = 0;
Digest RawHash(const uint8_t *p, size_t n) {
  p011gpu::Sha256 hash;
  hash.update(p, n);
  const auto b = hash.finish();
  Digest d{};
  for (size_t i = 0; i < 32; ++i)
    d[i / 4] |= uint32_t(b[i]) << (8 * (i % 4));
  return d;
}
} // namespace
bool WitnessBroker::Reserve(uint64_t owner, uint64_t context, size_t bytes,
                            uint64_t &ticket) {
  if (ticket || !owner || !context || (bytes != 3149824u && bytes != 4722688u))
    return false;
  std::lock_guard<std::mutex> lock(witnessMutex);
  if (witnessTicket || witnessSerial == UINT64_MAX)
    return false;
  witnessTicket = ticket = ++witnessSerial;
  witnessContext = context;
  witnessBytes = bytes;
  return true;
}
bool WitnessBroker::Release(uint64_t ticket, uint64_t context, bool retired,
                            bool released) {
  std::lock_guard<std::mutex> lock(witnessMutex);
  if (!ticket || ticket != witnessTicket || context != witnessContext ||
      !retired || !released)
    return false;
  witnessTicket = witnessContext = 0;
  witnessBytes = 0;
  return true;
}
void VisualBootstrap::Pair(size_t i, uint64_t n) {
  receipt_.words[i] = uint32_t(n);
  receipt_.words[i + 1] = uint32_t(n >> 32);
}
uint64_t VisualBootstrap::Pair(size_t i) const {
  return uint64_t(receipt_.words[i]) | (uint64_t(receipt_.words[i + 1]) << 32);
}
bool VisualBootstrap::Good() {
  const auto e = glGetError();
  if (e && !rawGl_)
    rawGl_ = e;
  return !rawGl_;
}
bool VisualBootstrap::Live(const BootstrapTicket &t, uint64_t now) const {
  return t.value && t.value == Pair(56) && !invalid_ && now >= Pair(82) &&
         now < Pair(84);
}
bool VisualBootstrap::ReserveInitial(const Scope &scope,
                                     const VisualResources &r, uint64_t now) {
  if (credit_ || native_ || gpu_ || now > UINT64_MAX - 60000000000ull)
    return false;
  const auto *limits = SelectCold(r.parent, r.k, r.v);
  if (!limits || !WitnessBroker::Reserve(scope.owner, scope.context,
                                         limits->witnessBytes, credit_))
    return false;
  receipt_ = {};
  invalid_ = false;
  rawGl_ = maps_ = 0;
  Pair(58, scope.owner);
  Pair(60, scope.context);
  Pair(82, now);
  Pair(84, now + 60000000000ull);
  capacity_ = limits->witnessBytes;
  native_.reset(new (std::nothrow) uint8_t[capacity_]());
  if (!native_) {
    receipt_.words[116] = 1;
    Retire(true, false);
    return false;
  }
  glGenBuffers(1, &gpu_);
  glBindBuffer(GL_COPY_WRITE_BUFFER, gpu_);
  glBufferData(GL_COPY_WRITE_BUFFER, GLsizeiptr(capacity_), nullptr,
               GL_DYNAMIC_COPY);
  return gpu_ && Good();
}
VisualBootstrap::Result VisualBootstrap::BeginVisualBootstrap(
    const TypedParentInitialToken &token, const Words64 &config,
    const VisualResources &r, GLuint work, uint64_t now, BootstrapTicket *out) {
  if (!out || out->value || !credit_ || !native_ || !gpu_ || !work ||
      !token.id_ || !token.parent_ ||
      !ValidateVisualConfig(config, *token.parent_) ||
      Pair(58) != token.scope_->owner || Pair(60) != token.scope_->context ||
      now < Pair(82) || now >= Pair(84))
    return Result::INVALID;
  const auto *limits = SelectCold(r.parent, r.k, r.v);
  if (!limits || capacity_ != limits->witnessBytes)
    return Result::INVALID;
  const auto created = Pair(82), deadline = Pair(84);
  receipt_ = {};
  auto &b = receipt_.words;
  invalid_ = false;
  rawGl_ = maps_ = 0;
  b[0] = 0x53423147u;
  b[1] = 1;
  b[2] = 1;
  b[3] = uint32_t(token.parent_->Kind());
  b[4] = config[2];
  b[5] = r.k;
  b[6] = r.v;
  std::copy(token.physical_->begin(), token.physical_->end(), b.begin() + 16);
  auto layout = HexDigest(kCompositeLayoutSHA),
       cfg = accept647::HashWords(config.data(), 64),
       recipe = HexDigest(kVisualRecipeSHA);
  std::copy(layout.begin(), layout.end(), b.begin() + 24);
  std::copy(token.pipeline_->begin(), token.pipeline_->end(), b.begin() + 32);
  std::copy(cfg.begin(), cfg.end(), b.begin() + 40);
  std::copy(recipe.begin(), recipe.end(), b.begin() + 48);
  const auto &id = *token.scope_;
  Pair(56, token.id_);
  Pair(58, id.owner);
  Pair(60, id.context);
  Pair(62, id.run);
  Pair(64, id.surface);
  Pair(66, id.pageSession);
  Pair(68, id.inputEpoch);
  Pair(70, 0);
  b[72] = 0;
  b[73] = 4;
  Pair(74, 0);
  Pair(78, credit_);
  Pair(80, credit_);
  Pair(82, created);
  Pair(84, deadline);
  Pair(88, token.id_);
  const auto segments = SeedSegments(r);
  size_t total = 0;
  for (size_t i = 0; i < 4; ++i) {
    const auto &s = segments[i];
    b[96 + i * 4] = s.source;
    b[97 + i * 4] = s.packed;
    b[98 + i * 4] = s.bytes;
    b[99 + i * 4] = s.kind;
    total += s.bytes;
  }
  Pair(112, total);
  for (const auto &s : segments) {
    glBindBuffer(GL_COPY_READ_BUFFER, work);
    glBindBuffer(GL_COPY_WRITE_BUFFER, gpu_);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
                        GLintptr(s.source), GLintptr(4096 + s.packed),
                        GLsizeiptr(s.bytes));
    ++b[117];
  }
  glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
  fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  glFlush();
  if (!gpu_ || !fence_ || !Good()) {
    b[116] = 2;
    invalid_ = true;
    return Result::GL_FAILED;
  }
  Pair(76, token.id_);
  out->value = token.id_;
  return Result::IN_PROGRESS;
}
VisualBootstrap::Result
VisualBootstrap::PollVisualBootstrap(const BootstrapTicket &t, uint64_t now) {
  if (!Live(t, now)) {
    invalid_ = true;
    return now > Pair(84) ? Result::EXPIRED : Result::INVALID;
  }
  auto &b = receipt_.words;
  if (b[2] >= 3)
    return b[2] == 4 ? Result::SEALED : Result::RAW_READY;
  const auto status = glClientWaitSync(fence_, 0, 0);
  if (status == GL_TIMEOUT_EXPIRED)
    return Result::IN_PROGRESS;
  if (status != GL_ALREADY_SIGNALED && status != GL_CONDITION_SATISFIED ||
      !Good()) {
    b[116] = 3;
    invalid_ = true;
    return Result::GL_FAILED;
  }
  if (b[2] == 1u)
    b[2] = 2u;
  const size_t total = size_t(Pair(112));
  size_t done = size_t(Pair(114));
  // One bounded mapping per poll; no second N-byte copy or whole-archive JS
  // value.
  const auto count = std::min<size_t>(4096, total - done);
  if (count) {
    glBindBuffer(GL_COPY_READ_BUFFER, gpu_);
    auto *p = glMapBufferRange(GL_COPY_READ_BUFFER, GLintptr(4096 + done),
                               GLsizeiptr(count), GL_MAP_READ_BIT);
    if (!p) {
      b[116] = 4;
      invalid_ = true;
      return Result::GL_FAILED;
    }
    std::memcpy(native_.get() + 4096 + done, p, count);
    ++maps_;
    const bool unmap = glUnmapBuffer(GL_COPY_READ_BUFFER) == GL_TRUE;
    if (!unmap || !Good()) {
      b[116] = 5;
      invalid_ = true;
      return Result::GL_FAILED;
    }
    done += count;
    Pair(114, done);
  }
  if (done != total)
    return Result::IN_PROGRESS;
  glDeleteSync(fence_);
  fence_ = nullptr;
  b[119] = 1;
  const auto *header = reinterpret_cast<const uint32_t *>(native_.get() + 4096);
  if (header[0] != 1110520135u || header[1] != 2 || header[2] != b[4] ||
      header[4] != b[5] || header[5] != b[6] || header[6] || header[7] ||
      header[8] || header[9] || header[12] || header[13] || header[14] ||
      header[15] || header[40] != 1 || header[42] ||
      header[127] != 3235778819u || accept647::Pair(header, 16) != Pair(58) ||
      accept647::Pair(header, 18) != Pair(60) ||
      accept647::Pair(header, 20) != Pair(62) ||
      accept647::Pair(header, 22) != Pair(64)) {
    b[116] = 6;
    invalid_ = true;
    return Result::INVALID;
  }
  for (size_t i = 0; i < 8; ++i)
    if (header[24 + i] != b[40 + i] || header[32 + i] != b[32 + i]) {
      b[116] = 7;
      invalid_ = true;
      return Result::INVALID;
    }
  for (size_t i = 48; i < 127; ++i)
    if (header[i]) {
      b[116] = 8;
      invalid_ = true;
      return Result::INVALID;
    }
  const auto hash = RawHash(native_.get() + 4096, total);
  std::copy(hash.begin(), hash.end(), b.begin() + 8);
  for (size_t i = 0; i < 4; ++i) {
    const auto d = RawHash(native_.get() + 4096 + b[97 + i * 4], b[98 + i * 4]);
    std::memcpy(native_.get() + i * 32, d.data(), 32);
  }
  b[2] = 3;
  Pair(90, now);
  return Result::RAW_READY;
}
VisualBootstrap::Result VisualBootstrap::ReadVisualBootstrap(
    const BootstrapTicket &t, uint32_t segment, uint32_t offset,
    uint8_t *caller, size_t cap, SeedPageMeta *meta, uint64_t now) {
  if (!Live(t, now))
    return Result::EXPIRED;
  if (receipt_.words[2] != 3 || !native_)
    return Result::WOULD_BLOCK;
  if (reader_ || segment >= 4 || !caller || !cap || cap > 4096 || offset % 4 ||
      Pair(86) == UINT64_MAX)
    return Result::INVALID;
  const auto &b = receipt_.words;
  const size_t n = b[98 + segment * 4], at = b[97 + segment * 4];
  if (offset >= n)
    return Result::INVALID;
  Pair(86, Pair(86) + 1);
  reader_ = true;
  receipt_.words[118] = 1;
  const size_t count = std::min(cap, n - offset);
  std::memcpy(caller, native_.get() + 4096 + at + offset, count);
  if (meta) {
    *meta = {segment, offset, uint32_t(count), uint32_t(n), {}};
    std::memcpy(meta->segmentSHA.data(), native_.get() + segment * 32, 32);
  }
  reader_ = false;
  receipt_.words[118] = 0;
  return Result::RAW_READY;
}
VisualBootstrap::Result
VisualBootstrap::SealVisualBootstrap(const BootstrapTicket &t,
                                     VisualSeedReceipt *output, uint64_t now) {
  if (!output || !Live(t, now) || receipt_.words[2] != 3 || reader_ ||
      !receipt_.words[119])
    return Result::WOULD_BLOCK;
  *output = receipt_;
  receipt_.words[2] = 4;
  output->words[2] = 4;
  return Result::SEALED;
}
VisualBootstrap::Result VisualBootstrap::End(const BootstrapTicket &t,
                                             uint64_t now) {
  if (t.value != Pair(56))
    return Result::INVALID;
  if (reader_)
    return Result::WOULD_BLOCK;
  if (receipt_.words[2] != 4 || !Live(t, now))
    return Result::INVALID;
  return Retire(true, false) ? Result::SEALED : Result::RETIRED_NOT_FREED;
}
bool VisualBootstrap::Retire(bool current, bool contextRetired) {
  if (reader_)
    return false;
  if (gpu_ && !fence_ && !receipt_.words[119] && !contextRetired)
    return false;
  if (fence_ && !contextRetired) {
    if (!current)
      return false;
    const auto s = glClientWaitSync(fence_, 0, 0);
    if (s != GL_ALREADY_SIGNALED && s != GL_CONDITION_SATISFIED)
      return false;
    glDeleteSync(fence_);
    fence_ = nullptr;
  }
  if (contextRetired)
    fence_ = nullptr;
  if (gpu_) {
    if (!current && !contextRetired)
      return false;
    if (current) {
      glDeleteBuffers(1, &gpu_);
      if (!Good())
        return false;
    }
    gpu_ = 0;
  }
  native_.reset();
  capacity_ = 0;
  receipt_.words[119] = 1;
  if (credit_) {
    if (!WitnessBroker::Release(credit_, Pair(60), true, !reader_))
      return false;
    credit_ = 0;
  }
  return true;
}
Digest VisualBootstrap::SeedHash() const {
  Digest out{};
  std::copy_n(receipt_.words.begin() + 8, 8, out.begin());
  return out;
}
std::string VisualBootstrap::Manifest() const {
  const auto &b = receipt_.words;
  std::ostringstream o;
  o << "{\"schema\":\"G11_VIS_SEED_RAW_V1\",\"id\":\"" << Pair(56)
    << "\",\"state\":" << b[2] << ",\"rawBytes\":\"" << Pair(112)
    << "\",\"readBytes\":\"" << Pair(114) << "\",\"actualVisualSeedSHA\":\""
    << HexDigest(SeedHash()) << "\",\"copies\":" << b[117]
    << ",\"maps\":" << maps_ << ",\"rawGl\":" << rawGl_ << ",\"segments\":[";
  for (size_t i = 0; i < 4; ++i) {
    if (i)
      o << ',';
    o << "{\"kind\":" << b[99 + i * 4] << ",\"gpuByteOffset\":" << b[96 + i * 4]
      << ",\"preimageOffset\":" << b[97 + i * 4]
      << ",\"bytes\":" << b[98 + i * 4];
    if (native_ && b[2] >= 3) {
      Digest h{};
      std::memcpy(h.data(), native_.get() + i * 32, 32);
      o << ",\"sha256\":\"" << HexDigest(h) << "\"";
    }
    o << '}';
  }
  o << "],\"receiptWords\":[";
  for (size_t i = 0; i < 128; ++i) {
    if (i)
      o << ',';
    o << b[i];
  }
  return o.str() + "]}";
}
} // namespace grs::g11vis
