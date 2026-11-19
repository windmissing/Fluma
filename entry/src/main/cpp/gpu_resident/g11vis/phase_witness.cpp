#include "phase_witness.h"
#include <cstring>
#include <sstream>
namespace grs::g11vis {
bool PhaseWitness::Good() {
  auto e = glGetError();
  if (e && !rawGl_)
    rawGl_ = e;
  return !rawGl_;
}
bool PhaseWitness::Copy(GLuint work, size_t from, size_t to, size_t bytes) {
  if (from > PTRDIFF_MAX || to > bytes_ || bytes > bytes_ - to)
    return false;
  glBindBuffer(GL_COPY_READ_BUFFER, work);
  glBindBuffer(GL_COPY_WRITE_BUFFER, gpu_);
  glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, from, to,
                      bytes);
  ++copies_;
  glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
  return Good();
}
bool PhaseWitness::Arm(const Scope &scope, const ParentAdapter &parent,
                       const Words64 &config, const Digest &pipeline,
                       const VisualSeedReceipt &seed, uint64_t context,
                       uint64_t nonce, uint32_t micro, uint32_t phase,
                       uint32_t role, const VisualResources &v, uint64_t now) {
  if (ticket_ || !nonce || (micro > 31 && micro != UINT32_MAX) || phase < 1 ||
      phase > 6 || (role != 2 && role != 4))
    return false;
  const auto bytes = 4096u + 256u * size_t(v.v) + 2097152u + 524288u;
  uint64_t ticket = 0;
  if (!WitnessBroker::Reserve(scope.owner, context, bytes, ticket))
    return false;
  ticket_ = ticket;
  context_ = context;
  nonce_ = nonce;
  micro_ = micro;
  phase_ = phase;
  role_ = role;
  created_ = now;
  bytes_ = bytes;
  done_ = 0;
  old_ = captured_ = retired_ = reader_ = invalid_ = false;
  copies_ = maps_ = rawGl_ = 0;
  hash_ = p011gpu::Sha256{};
  digest_ = {};
  native_.reset(new (std::nothrow) uint8_t[bytes_]());
  if (!native_) {
    retired_ = true;
    End(true);
    return false;
  }
  auto *h = reinterpret_cast<uint32_t *>(native_.get());
  h[0] = 0x57313147;
  h[1] = 2;
  h[2] = v.k;
  h[3] = v.v;
  auto put = [&](size_t n, uint64_t x) {
    h[n] = uint32_t(x);
    h[n + 1] = uint32_t(x >> 32);
  };
  put(4, nonce_);
  h[6] = micro_;
  h[7] = phase_;
  h[8] = role_;
  put(10, created_);
  put(12, ticket_);
  h[9] = uint32_t(parent.Kind());
  const uint64_t ids[] = {scope.run,        scope.owner,       scope.context,
                          scope.inputEpoch, scope.pageSession, scope.permission,
                          scope.window,     scope.config,      scope.surface};
  for (size_t i = 0; i < 9; ++i)
    put(32 + 2 * i, ids[i]);
  const Digest hashes[] = {parent.ParameterHash(),
                           parent.PhysicalLayoutHash(),
                           pipeline,
                           accept647::HashWords(config.data(), 64),
                           HexDigest(kVisualRecipeSHA),
                           HexDigest(kCompositeLayoutSHA)};
  for (size_t i = 0; i < 6; ++i)
    std::copy(hashes[i].begin(), hashes[i].end(), h + 64 + 8 * i);
  std::copy(seed.words.begin(), seed.words.end(), h + 128);
  glGenBuffers(1, &gpu_);
  glBindBuffer(GL_COPY_WRITE_BUFFER, gpu_);
  glBufferData(GL_COPY_WRITE_BUFFER, bytes_, nullptr, GL_DYNAMIC_COPY);
  if (!gpu_ || !Good()) {
    state_ = "GL_FAILED";
    invalid_ = true;
    return false;
  }
  state_ = "ARMED";
  return true;
}
bool PhaseWitness::After(const G11Pass &p, uint64_t nonce, GLuint work,
                         const VisualResources &v, uint32_t bank) {
  if (!ticket_ || invalid_ || retired_ || nonce != nonce_ || !p.vis ||
      p.role != role_ || uint32_t(p.micro) != micro_ || p.phase != phase_)
    return true;
  if (p.op == 513u && !old_) {
    old_ = Copy(work, v.visual[bank], 4096, size_t(v.v) * 128);
    state_ = old_ ? "OLD_CAPTURED" : "GL_FAILED";
    return old_;
  }
  if (p.op != 514u || captured_)
    return true;
  if (!old_) {
    invalid_ = true;
    state_ = "MISSING_OLD";
    return false;
  }
  auto at = 4096 + size_t(v.v) * 128;
  captured_ =
      Copy(work, v.visual[bank], at, size_t(v.v) * 128) &&
      Copy(work, v.visual[kSegMaterialPlan], at + size_t(v.v) * 128, 2097152) &&
      Copy(work, v.visual[kSegFourMaps], at + size_t(v.v) * 128 + 2097152,
           524288);
  // Preserve the target batch's actual bank/plan control header before later
  // batches overwrite it.
  captured_ = captured_ &&
              Copy(work, v.visual[kSegControl] + bank * 512, 1024, 512) &&
              Copy(work, v.visual[kSegControl] + kControlPlanHeader.byteStart,
                   1536, kControlPlanHeader.bytes);
  state_ = captured_ ? "CAPTURED_WAIT_ROOT" : "GL_FAILED";
  return captured_;
}
void PhaseWitness::RootRetired(uint64_t nonce, const accept647::Tail *tail,
                               bool accepted) {
  if (!ticket_ || nonce != nonce_ || retired_)
    return;
  retired_ = true;
  auto *h = reinterpret_cast<uint32_t *>(native_.get());
  if (h) {
    h[16] = accepted;
    h[17] = tail ? (*tail)[6] : 0;
    h[18] = copies_;
    if (tail)
      std::memcpy(h + 512, tail->data(), 1024);
  }
  state_ = invalid_ ? "INVALIDATED" : captured_ ? "READING" : "NOT_CAPTURED";
}
bool PhaseWitness::Pump(uint64_t now) {
  if (!ticket_ || !retired_ || !captured_)
    return false;
  if (now < created_ || now - created_ >= 60000000000ull) {
    invalid_ = true;
    if (End(true))
      state_ = "EXPIRED_FREED";
    return false;
  }
  if (invalid_) {
    state_ = "INVALIDATED";
    return false;
  }
  if (done_ == bytes_)
    return true;
  // GPU header patches were copied at the batch cut; native identity/outcome
  // stays native.
  const size_t begin = done_, n = std::min<size_t>(4096, bytes_ - done_);
  glBindBuffer(GL_COPY_READ_BUFFER, gpu_);
  auto *raw = glMapBufferRange(GL_COPY_READ_BUFFER, begin, n, GL_MAP_READ_BIT);
  ++maps_;
  if (!raw || !Good()) {
    state_ = "GL_FAILED";
    invalid_ = true;
    return false;
  }
  if (begin == 0) {
    std::memcpy(native_.get() + 1024, static_cast<const uint8_t *>(raw) + 1024,
                1024);
  } else
    std::memcpy(native_.get() + begin, raw, n);
  const bool ok = glUnmapBuffer(GL_COPY_READ_BUFFER) == GL_TRUE && Good();
  if (!ok) {
    invalid_ = true;
    state_ = "GL_FAILED";
    return false;
  }
  hash_.update(native_.get() + begin, n);
  done_ += n;
  if (done_ == bytes_) {
    const auto d = hash_.finish();
    for (size_t i = 0; i < 8; ++i)
      std::memcpy(&digest_[i], d.data() + 4 * i, 4);
    state_ = "READY";
  }
  return done_ == bytes_;
}
std::string PhaseWitness::State() const {
  std::ostringstream o;
  o << "{\"revision\":\"G11_PHASE_WITNESS_V2\",\"state\":\"" << state_
    << "\",\"id\":\"" << ticket_ << "\",\"rootNonce\":\"" << nonce_
    << "\",\"micro\":" << micro_ << ",\"phase\":" << phase_
    << ",\"role\":" << role_ << ",\"bytes\":" << bytes_
    << ",\"completed\":" << done_ << ",\"copies\":" << copies_
    << ",\"maps\":" << maps_ << ",\"copyBarriers\":" << copies_
    << ",\"firstGl\":" << rawGl_ << ",\"sha256\":\"" << HexDigest(digest_)
    << "\"}";
  return o.str();
}
std::string PhaseWitness::Page(uint32_t offset, uint64_t now) {
  if (!ticket_ || !native_ || invalid_ || !retired_ || done_ != bytes_ ||
      reader_ || now < created_ || now - created_ >= 60000000000ull ||
      offset >= bytes_ / 4)
    return "{\"state\":\"NOT_READABLE\"}";
  reader_ = true;
  struct EndRead {
    bool &b;
    ~EndRead() { b = false; }
  } end{reader_};
  const auto count = std::min<size_t>(1024, bytes_ / 4 - offset);
  std::ostringstream o;
  o << "{\"id\":\"" << ticket_ << "\",\"wordOffset\":" << offset
    << ",\"words\":[";
  const auto *words = reinterpret_cast<const uint32_t *>(native_.get());
  for (size_t i = 0; i < count; ++i) {
    if (i)
      o << ',';
    o << words[offset + i];
  }
  o << "]}";
  auto text = o.str();
  return text.size() <= 16384 ? text : "{\"state\":\"PAGE_UTF8_LIMIT\"}";
}
bool PhaseWitness::End(bool current, bool gone) {
  if (!ticket_)
    return true;
  CancelUnsubmitted();
  if (reader_ || !retired_ && !gone || !current && !gone)
    return false;
  if (current && gpu_)
    glDeleteBuffers(1, &gpu_);
  gpu_ = 0;
  native_.reset();
  if (!WitnessBroker::Release(ticket_, context_, true, true))
    return false;
  ticket_ = 0;
  state_ = "FREED";
  return true;
}
} // namespace grs::g11vis
