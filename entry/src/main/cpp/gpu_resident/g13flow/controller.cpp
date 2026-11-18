#include "controller.h"
#include "../resource_gl.h"
namespace grs::g13 {
uint32_t Controller::Guard(void *context, uint64_t now) {
  auto &self = *static_cast<Controller *>(context);
  if (!self.live_ || self.cancelled_ ||
      (self.authority_ && !self.authority_(self.authorityContext_)))
    return 11;
  if (now > U64(self.permit_.data() + 38))
    return 9;
  if (!self.Fresh(now))
    return 7;
  return 0;
}
bool Controller::Fresh(uint64_t now) const {
  return (!authority_ || authority_(authorityContext_)) && active_ &&
         lastPulse_ && now >= lastPulse_ && now - lastPulse_ <= 33333333ull;
}
uint32_t Controller::Configure(const Config &c, const Seed &seed,
                               const Identity &id) {
  if (live_ || sceneLease_ || coldPin_ || coldHold_ || !RetireSceneFence())
    return 34;
  if (!id.run || !id.owner || !id.context || !id.geometry || !id.config ||
      !id.session || !id.permission || !id.inputEpoch || !id.control ||
      (!seed.nativeControl && !id.surface))
    return 2;
  if ((seed.testHook || seed.diagnostic == 3) && !PipelineTestHooks())
    return 4;
  if (seed.testHook > 5 || seed.diagnostic > 3 ||
      (!seed.diagnostic && seed.forcedJ) ||
      (seed.forcedJ != 0 && seed.forcedJ != 1 && seed.forcedJ != 2 &&
       seed.forcedJ != 4) ||
      (seed.diagnostic == 1 && seed.forcedJ > 1))
    return 5;
  transport_.SetGuard(this, &Controller::Guard);
  auto e = transport_.Configure(c, seed, id);
  if (e)
    return failure_ = e;
  config_ = c;
  identity_ = id;
  acceptedIdentity_ = id;
  acceptedNonce_ = 0;
  issuedNonce_ = issuedPlan_ = 0;
  baseToken_.clear();
  diagnostic_ = seed.diagnostic;
  forcedJ_ = seed.forcedJ;
  nativeControl_ = seed.nativeControl;
  maxUiGap_ = maxNativeGap_ = pulses_ = 0;
  configured_ = true;
  active_ = false;
  cancelled_ = false;
  failure_ = 0;
  runFault_ = false;
  acceptedTail_ = {};
  lastRootPoll_ = 0;
  lastRootError_ = lastRootOutcome_ = 0;
  lastRootGpuTail_ = false;
  rebind_ = false;
  lastSource_ = 0;
  lastPulse_ = pulseSeq_ = lastUiNs_ = 0;
  overload_ = revokedQuota_ = newQuantumFrames_ = sourceFrames_ =
      repeatFrames_ = 0;
  commit_ = quantum_ = earned_ = reserved_ = consumed_ = discarded_ =
      clearedCarry_ += clockRemainder_;
  clockRemainder_ = 0;
  head_ = count_ = 0;
  quantum_ = U64(transport_.InitialHeader().data() + 18);
  lastSource_ = U64(transport_.InitialHeader().data() + 192);
  lastSourceIntent_ = U64(transport_.InitialHeader().data() + 196);
  clearedCarry_ = staleFrames_ = clockFault_ = 0;
  tail_ = {};
  return 0;
}
uint32_t Controller::Pulse(uint64_t session, uint64_t sequence, uint64_t uiNs,
                           uint64_t now) {
  if (nativeControl_)
    return 4;
  return AcceptPulse(session, sequence, uiNs, now, true);
}
uint32_t Controller::NativePulse(uint64_t sequence, uint64_t now) {
  if (!nativeControl_)
    return 4;
  return AcceptPulse(identity_.session, sequence, 0, now, false);
}
uint32_t Controller::AcceptPulse(uint64_t session, uint64_t sequence,
                                 uint64_t uiNs, uint64_t now, bool ui) {
  if (session != identity_.session || !active_)
    return 31;
  if (sequence <= pulseSeq_ || (ui && uiNs <= lastUiNs_))
    return 8;
  bool reversed = lastPulse_ && now < lastPulse_;
  uint64_t nativeGap = lastPulse_ && !reversed ? now - lastPulse_ : 0,
           uiGap = ui && lastUiNs_ ? uiNs - lastUiNs_ : 0;
  maxNativeGap_ = std::max(maxNativeGap_, nativeGap);
  maxUiGap_ = std::max(maxUiGap_, uiGap);
  if (reversed || nativeGap > 33333333ull || uiGap > 33333333ull) {
    uint64_t removed = earned_ - consumed_ - reserved_ - discarded_;
    discarded_ += removed;
    revokedQuota_ += removed;
    clearedCarry_ += clockRemainder_;
    clockRemainder_ = 0;
    lastPulse_ = 0;
    failure_ = reversed ? 8 : 7;
    clockFault_ = failure_;
    cancelled_ = live_;
    return failure_;
  }
  lastPulse_ = now;
  pulseSeq_ = sequence;
  lastUiNs_ = ui ? uiNs : 0;
  ++pulses_;
  clockRemainder_ += nativeGap * 120ull;
  uint64_t newly = clockRemainder_ / 1000000000ull;
  clockRemainder_ %= 1000000000ull;
  earned_ += newly;
  uint64_t available = earned_ - consumed_ - reserved_ - discarded_;
  if (available > 4) {
    discarded_ += available - 4;
    overload_ += available - 4;
  }
  return 0;
}
uint32_t Controller::Offer(const Input &input, uint64_t now) {
  if (!configured_ || !Fresh(now))
    return 7;
  if (diagnostic_ == 1)
    return 4;
  if (input[0] != kMagic_input || input[1] != 1 || input[2] != 128 ||
      input[127] != kSeal || input[3] != 50)
    return 1;
  for (size_t i = 108; i < 127; i++)
    if (input[i])
      return 5;
  if (input[30] || input[31] || input[103] != 1)
    return 4;
  if (!SelfHash(input, 80))
    return 3;
  for (auto at : {90, 91, 92, 93, 94, 96, 101, 102})
    if (!std::isfinite(F32(input[size_t(at)])))
      return 5;
  if (input[88] < 1 || input[88] > 3 || input[101] != config_[32] ||
      input[102] != config_[33] ||
      std::hypot(F32(input[93]), F32(input[94])) > 64 ||
      std::abs(F32(input[91])) > 2 || std::abs(F32(input[96])) > 8)
    return 5;
  if (input[88] == 2 && (input[89] != 2 || F32(input[92]) < 2))
    return 5;
  if (input[88] == 3 &&
      (input[95] < 1 || input[95] > 3 || (input[95] != 1 && !diagnostic_) ||
       U64(input.data() + 99) <= U64(input.data() + 97) ||
       U64(input.data() + 97) < quantum_))
    return 5;
  for (auto pair :
       std::array<std::pair<size_t, uint64_t>, 8>{{{4, identity_.run},
                                                   {6, identity_.context},
                                                   {8, identity_.surface},
                                                   {10, identity_.geometry},
                                                   {12, identity_.config},
                                                   {14, identity_.session},
                                                   {16, identity_.permission},
                                                   {18, identity_.control}}})
    if (U64(input.data() + pair.first) != pair.second)
      return 2;
  for (auto pair : std::array<std::pair<size_t, size_t>, 6>{
           {{32, 76}, {40, 36}, {48, 44}, {56, 52}, {64, 60}, {72, 68}}})
    for (size_t i = 0; i < 8; i++)
      if (input[pair.first + i] != config_[pair.second + i])
        return 3;
  if (commandSerial_ == UINT64_MAX)
    return 36;
  uint64_t seq = U64(input.data() + 20);
  if (seq <= lastSource_)
    return 31;
  if (count_ &&
      seq <= U64(fifo_[(head_ + count_ - 1) % 16].original.data() + 20))
    return 31;
  for (size_t i = 0; i < count_; i++)
    if (U64(fifo_[(head_ + i) % 16].original.data() + 20) == seq)
      return 31;
  if (count_ == 16)
    return 30;
  auto &q = fifo_[(head_ + count_) % 16];
  q.original = input;
  q.canonical = input;
  q.serial = ++commandSerial_;
  Put(q.canonical.data() + 22, now);
  Put(q.canonical.data() + 106, q.serial);
  SealHash(q.canonical, 80);
  ++count_;
  return 0;
}
uint32_t Controller::BeginRoot(const Permit &p) {
  uint64_t now = Now();
  if (!RetireSceneFence())
    return 34;
  if (runFault_)
    return failure_ ? failure_ : 18;
  if (failure_ && failure_ != 12 && failure_ != 13 && failure_ != 14 &&
      failure_ != 15 && failure_ != 23)
    return failure_;
  if (!Ready() || live_ || coldHold_ || coldPin_ || sceneLease_ || sceneFence_)
    return 34;
  if (!Fresh(now))
    return 7;
  if (p[0] != 1 || p[127] != kSeal || p[1] != 0 && p[1] != 50 ||
      p[2] != (nativeControl_ ? 1u : 0u) ||
      p[3] != transport_.CandidateBank() || p[4] != transport_.ReplayBank() ||
      p[5] != transport_.PendingPack() ||
      p[7] != RequiredAudits(p[1] == 50, diagnostic_) || p[92] != diagnostic_ ||
      p[93] != forcedJ_ || p[94] != (rebind_ ? 1u : 0u) || p[95] ||
      U64(p.data() + 8) <= issuedNonce_ || U64(p.data() + 10) <= issuedPlan_ ||
      U64(p.data() + 36) > now ||
      U64(p.data() + 38) != U64(p.data() + 36) + 250000000ull ||
      now > U64(p.data() + 38))
    return 2;
  for (auto pair :
       std::array<std::pair<size_t, uint64_t>, 12>{{{12, identity_.run},
                                                    {14, identity_.owner},
                                                    {16, identity_.context},
                                                    {18, identity_.surface},
                                                    {20, identity_.geometry},
                                                    {22, identity_.config},
                                                    {24, identity_.session},
                                                    {26, identity_.permission},
                                                    {28, identity_.inputEpoch},
                                                    {30, identity_.control},
                                                    {32, commit_},
                                                    {88, quantum_}}})
    if (U64(p.data() + pair.first) != pair.second)
      return 2;
  for (auto pair : std::array<std::pair<size_t, size_t>, 4>{
           {{40, 76}, {48, 36}, {56, 44}, {64, 52}}})
    for (size_t i = 0; i < 8; i++)
      if (p[pair.first + i] != config_[pair.second + i])
        return 3;
  if (U64(p.data() + 34) != commit_ + 1 ||
      U64(p.data() + 90) != (p[1] == 50 && count_
                                 ? U64(fifo_[head_].canonical.data() + 20)
                                 : lastSource_))
    return 2;
  bool source = p[1] >= 50;
  for (size_t i = 96; i < 127; i++)
    if (p[i])
      return 2;
  if (p[6] != (source ? head_ : 0))
    return 2;
  if (source && !count_)
    return 31;
  if (!source && earned_ - consumed_ - reserved_ - discarded_ == 0)
    return 34;
  Input original{}, canonical{};
  if (source) {
    original = fifo_[head_].original;
    canonical = fifo_[head_].canonical;
  } else {
    canonical[0] = kMagic_input;
    canonical[1] = 1;
    canonical[2] = 128;
    canonical[3] = p[1];
    for (auto pair : std::array<std::pair<size_t, size_t>, 8>{{{4, 12},
                                                               {6, 16},
                                                               {8, 18},
                                                               {10, 20},
                                                               {12, 22},
                                                               {14, 24},
                                                               {16, 26},
                                                               {18, 30}}})
      std::copy_n(p.data() + pair.second, 2, canonical.data() + pair.first);
    Put(canonical.data() + 20, U64(p.data() + 8));
    Put(canonical.data() + 22, now);
    Put(canonical.data() + 24, commit_);
    Put(canonical.data() + 26, quantum_);
    canonical[30] = p[92];
    canonical[31] = p[93];
    for (auto pair : std::array<std::pair<size_t, size_t>, 6>{
             {{32, 76}, {40, 36}, {48, 44}, {56, 52}, {64, 60}, {72, 68}}})
      std::copy_n(config_.data() + pair.second, 8,
                  canonical.data() + pair.first);
    canonical[127] = kSeal;
    SealHash(canonical, 80);
    original = canonical;
  }
  baseToken_ = FieldToken();
  Permit actual = p;
  auto baseHash = Hash(baseToken_.data(), baseToken_.size());
  std::copy(baseHash.begin(), baseHash.end(), actual.begin() + 80);
  std::copy_n(canonical.data() + 80, 8, actual.data() + 72);
  SnapshotQuota(actual, now);
  auto e = transport_.BeginRoot(actual, original, canonical, quota_);
  if (e)
    return e;
  permit_ = actual;
  issuedNonce_ = U64(actual.data() + 8);
  issuedPlan_ = U64(actual.data() + 10);
  nonce_ = std::max(nonce_, issuedNonce_);
  plan_ = std::max(plan_, U64(actual.data() + 10));
  failure_ = 0;
  live_ = true;
  cancelled_ = false;
  if (!source)
    ++reserved_;
  return 0;
}
void Controller::SnapshotQuota(const Permit &p, uint64_t now) {
  quota_ = {};
  Put(quota_.data(), identity_.session);
  Put(quota_.data() + 2, pulseSeq_);
  Put(quota_.data() + 4, lastUiNs_);
  Put(quota_.data() + 6, lastPulse_);
  Put(quota_.data() + 8, pulses_);
  Put(quota_.data() + 10, maxUiGap_);
  Put(quota_.data() + 12, maxNativeGap_);
  Put(quota_.data() + 14, earned_);
  Put(quota_.data() + 16, consumed_);
  Put(quota_.data() + 18, overload_);
  Put(quota_.data() + 20, revokedQuota_);
  Put(quota_.data() + 22, clockRemainder_);
  Put(quota_.data() + 26, U64(p.data() + 10));
  Put(quota_.data() + 28,
      earned_ - consumed_ - reserved_ - discarded_ - (p[1] == 50 ? 0 : 1));
  Put(quota_.data() + 30, reserved_ + (p[1] == 50 ? 0 : 1));
  Put(quota_.data() + 36, identity_.control);
  Put(quota_.data() + 46, now);
  Put(quota_.data() + 48, U64(p.data() + 38));
  Put(quota_.data() + 52, newQuantumFrames_);
  Put(quota_.data() + 54, sourceFrames_);
  Put(quota_.data() + 56, repeatFrames_);
  Put(quota_.data() + 62, nativeControl_ ? 1 : 0);
  Put(quota_.data() + 24, clearedCarry_);
  Put(quota_.data() + 32, quantum_);
  Put(quota_.data() + 34, p[1] == 50 ? U64(fifo_[head_].canonical.data() + 104)
                                     : lastSourceIntent_);
  Put(quota_.data() + 38,
      p[1] == 50 ? U64(fifo_[head_].canonical.data() + 20) : lastSource_);
  Put(quota_.data() + 40, U64(permit_.data() + 10));
  Put(quota_.data() + 42, U64(permit_.data() + 8));
  Put(quota_.data() + 44, lastRootPoll_);
  Put(quota_.data() + 58, staleFrames_);
  Put(quota_.data() + 60, clockFault_);
}
uint32_t Controller::BeginGranted(uint64_t now) {
  if (runFault_)
    return failure_ ? failure_ : 18;
  if (failure_ && failure_ != 12 && failure_ != 13 && failure_ != 14 &&
      failure_ != 15 && failure_ != 23)
    return failure_;
  if (live_ || coldHold_ || !Ready() || !Fresh(now))
    return 34;
  if (nonce_ == UINT64_MAX || plan_ == UINT64_MAX || commit_ == UINT64_MAX ||
      quantum_ >= UINT64_MAX / 4 - 1)
    return failure_ = 36;
  if (!count_ && earned_ - consumed_ - reserved_ - discarded_ == 0)
    return 34;
  Permit p{};
  p[0] = 1;
  p[2] = nativeControl_ ? 1u : 0u;
  p[1] = count_ ? 50 : 0;
  p[3] = transport_.CandidateBank();
  p[4] = transport_.ReplayBank();
  p[5] = transport_.PendingPack();
  p[6] = count_ ? uint32_t(head_) : 0u;
  p[7] = RequiredAudits(count_ != 0, diagnostic_);
  Put(p.data() + 8, ++nonce_);
  Put(p.data() + 10, ++plan_);
  for (auto pair :
       std::array<std::pair<size_t, uint64_t>, 12>{{{12, identity_.run},
                                                    {14, identity_.owner},
                                                    {16, identity_.context},
                                                    {18, identity_.surface},
                                                    {20, identity_.geometry},
                                                    {22, identity_.config},
                                                    {24, identity_.session},
                                                    {26, identity_.permission},
                                                    {28, identity_.inputEpoch},
                                                    {30, identity_.control},
                                                    {32, commit_},
                                                    {88, quantum_}}})
    Put(p.data() + pair.first, pair.second);
  Put(p.data() + 34, commit_ + 1);
  Put(p.data() + 36, now);
  Put(p.data() + 38, now + 250000000ull);
  for (auto pair : std::array<std::pair<size_t, size_t>, 4>{
           {{40, 76}, {48, 36}, {56, 44}, {64, 52}}})
    std::copy_n(config_.data() + pair.second, 8, p.data() + pair.first);
  if (count_) {
    std::copy_n(fifo_[head_].canonical.data() + 80, 8, p.data() + 72);
    Put(p.data() + 90, U64(fifo_[head_].canonical.data() + 20));
  }
  if (!count_)
    Put(p.data() + 90, lastSource_);
  p[92] = diagnostic_;
  p[93] = forcedJ_;
  p[94] = rebind_ ? 1u : 0u;
  p[127] = kSeal;
  return BeginRoot(p);
}
uint32_t Controller::Complete(uint64_t now) {
  Completion tail{};
  uint32_t code = transport_.Poll(tail);
  if (code == 34)
    return code;
  if (code) {
    if (!failure_)
      failure_ = code;
    if (live_ && !transport_.Busy()) {
      live_ = false;
      if (permit_[1] < 50) {
        --reserved_;
        ++discarded_;
        ++revokedQuota_;
      }
      lastRootError_ = failure_;
      lastRootOutcome_ = failure_ == 9 ? 6u : failure_ == 11 ? 4u : 3u;
      lastRootPoll_ = Now();
      lastRootGpuTail_ = false;
      bool revoked = U64(permit_.data() + 30) != identity_.control;
      if (active_ && revoked &&
          (failure_ == 7 || failure_ == 8 || failure_ == 9 || failure_ == 11))
        failure_ = 0;
    }
    return code;
  }
  if (!live_)
    return 0;
  now = Now();
  live_ = false;
  const bool quantum = permit_[1] < 50;
  lastRootPoll_ = now;
  lastRootGpuTail_ = true;
  if (quantum)
    --reserved_;
  auto reject = [&](uint32_t error, uint32_t outcome) {
    if (quantum) {
      ++discarded_;
      ++revokedQuota_;
    }
    lastRootError_ = error;
    lastRootOutcome_ = outcome;
    failure_ = error;
    return error;
  };
  if (cancelled_ || !Fresh(now) || now > U64(permit_.data() + 38)) {
    uint32_t error = cancelled_ ? 11 : now > U64(permit_.data() + 38) ? 9 : 7;
    return reject(error, error == 9 ? 6u : error == 11 ? 4u : 5u);
  }
  bool valid =
      tail[0] == kMagic_tail && tail[1] == 1 && tail[2] == kMagic_profile &&
      tail[3] == 256 && tail[255] == kSeal && tail[4] == permit_[1] &&
      tail[37] == permit_[7] && !(tail[38] >> 20) &&
      tail[39] == (quantum ? 820u : 16u) && tail[40] + tail[41] == tail[39] &&
      tail[120] == permit_[3] && tail[121] == permit_[5] && tail[125] == 1 &&
      tail[126] == diagnostic_;
  for (size_t i = 0; i < 16; i++)
    valid = valid && tail[8 + i] == permit_[8 + i];
  for (size_t i = 0; i < 8; i++)
    valid = valid && tail[128 + i] == permit_[24 + i];
  valid = valid && U64(tail.data() + 24) == commit_ &&
          U64(tail.data() + 26) == commit_ + 1 &&
          U64(tail.data() + 28) == U64(permit_.data() + 36) &&
          U64(tail.data() + 30) == U64(permit_.data() + 38) &&
          U64(tail.data() + 32) == 0;
  for (auto group : std::array<std::pair<size_t, size_t>, 4>{
           {{48, 40}, {56, 48}, {64, 56}, {72, 64}}})
    for (size_t i = 0; i < 8; i++)
      valid = valid && tail[group.first + i] == permit_[group.second + i];
  for (size_t i = 0; i < 8; i++)
    valid = valid && tail[88 + i] == permit_[72 + i];
  for (size_t i = 146; i < 255; i++)
    valid = valid && tail[i] == 0;
  if (!valid) {
    runFault_ = true;
    return reject(2, 3);
  }
  tail_ = tail;
  lastRootError_ = tail[7];
  lastRootOutcome_ = tail[5];
  if (tail[5] == 1 && tail[42] == 1 && tail[43] == 3 &&
      (tail[38] & permit_[7]) == permit_[7]) {
    uint64_t expectedQ = quantum ? quantum_ + 1 : quantum_;
    if (U64(tail.data() + 34) != expectedQ ||
        (!quantum &&
         (!count_ ||
          U64(tail.data() + 44) != U64(fifo_[head_].canonical.data() + 20)))) {
      runFault_ = true;
      return reject(2, 3);
    }
    now = Now();
    if (cancelled_ || !Fresh(now) || now > U64(permit_.data() + 38))
      return reject(now > U64(permit_.data() + 38) ? 9 : 11,
                    now > U64(permit_.data() + 38) ? 6 : 4);
    transport_.Accept();
    acceptedTail_ = tail;
    acceptedIdentity_ = identity_;
    acceptedNonce_ = U64(tail.data() + 8);
    rebind_ = false;
    ++commit_;
    if (quantum) {
      ++quantum_;
      ++consumed_;
    } else {
      lastSourceIntent_ = U64(fifo_[head_].canonical.data() + 104);
      lastSource_ = U64(fifo_[head_].canonical.data() + 20);
      head_ = (head_ + 1) % 16;
      --count_;
    }
    failure_ = 0;
    return 0;
  }
  if (quantum) {
    ++discarded_;
    ++revokedQuota_;
  }
  if (tail[5] == 7 && (diagnostic_ == 1 || diagnostic_ == 3) && tail[42] == 0 &&
      U64(tail.data() + 34) == quantum_) {
    failure_ = tail[7];
    return failure_;
  }
  failure_ = tail[7] ? tail[7] : 37;
  if (tail[5] == 3) {
    runFault_ = true;
    active_ = false;
    cancelled_ = true;
  }
  return failure_;
}
uint32_t Controller::PumpBounded(uint64_t now) {
  if (!configured_)
    return 34;
  if (transport_.Busy() && !live_)
    return Complete(now);
  if (!live_)
    return 0;
  if (!Fresh(now) || cancelled_ || now > U64(permit_.data() + 38)) {
    cancelled_ = true;
  }
  if (cancelled_)
    transport_.Abort();
  auto e = transport_.PumpBounded(now);
  if (e && e != 34) {
    failure_ = e;
    cancelled_ = true;
    transport_.Abort();
  }
  auto p = Complete(Now());
  return p == 34 ? e : p;
}
bool Controller::Poll(Completion &out) const {
  out = tail_;
  return tail_[255] == kSeal;
}
uint32_t Controller::Rebind(uint64_t surface, uint64_t geometry) {
  if (!surface || !geometry)
    return 2;
  if (!RootIdle() || sceneLease_ || coldPin_ || coldHold_ ||
      !RetireSceneFence())
    return 34;
  identity_.surface = surface;
  identity_.geometry = geometry;
  rebind_ = commit_ != 0;
  return 0;
}
void Controller::SetActive(bool active, const Identity &id) {
  if (!(id == identity_)) {
    Stop();
    return;
  }
  active_ = active;
  if (!active)
    Stop();
  else {
    if (!runFault_ &&
        (failure_ == 7 || failure_ == 8 || failure_ == 9 || failure_ == 11))
      failure_ = 0;
    lastPulse_ = pulseSeq_ = lastUiNs_ = clockRemainder_ = 0;
  }
}
void Controller::Stop() {
  active_ = false;
  cancelled_ = true;
  lastPulse_ = 0;
  clearedCarry_ += clockRemainder_;
  clockRemainder_ = 0;
  uint64_t removed = earned_ - reserved_ - consumed_ - discarded_;
  discarded_ += removed;
  revokedQuota_ += removed;
  head_ = count_ = 0;
  ++identity_.control;
}
void Controller::Release(bool current) {
  Stop();
  transport_.Retire(current && !sceneLease_ && !sceneFence_);
  if (live_ && permit_[1] < 50 && reserved_) {
    --reserved_;
    ++discarded_;
    ++revokedQuota_;
  }
  live_ = false;
  coldHold_ = coldPin_ = false;
  configured_ = false;
  if (sceneFence_ && current)
    glDeleteSync(sceneFence_);
  sceneFence_ = nullptr;
  sceneLease_ = false;
}
bool Controller::RetireSceneFence() {
  if (!sceneFence_)
    return true;
  GLenum state = glClientWaitSync(sceneFence_, 0, 0);
  if (state == GL_TIMEOUT_EXPIRED)
    return false;
  if (state == GL_WAIT_FAILED) {
    if (!failure_)
      failure_ = 10;
    active_ = false;
    return false;
  }
  glDeleteSync(sceneFence_);
  sceneFence_ = nullptr;
  return true;
}
Controller::SceneLease Controller::AcquireScene() {
  if (!RetireSceneFence())
    return {};
  if (live_ || sceneLease_ || coldHold_ || coldPin_ || !commit_ ||
      !Fresh(Now()) || !(acceptedIdentity_ == identity_))
    return {};
  sceneLease_ = true;
  return {transport_.Field(),  transport_.Pack(),
          transport_.Layout(), commit_,
          acceptedIdentity_,   true};
}
bool Controller::ReleaseScene(uint64_t generation, GLsync fence) {
  if (!sceneLease_ || generation != commit_)
    return false;
  sceneLease_ = false;
  sceneFence_ = fence;
  glFlush();
  return true;
}
bool Controller::CanPresent(const SceneLease &lease, uint64_t now) const {
  return failure_ == 0 && lease.valid && lease.generation == commit_ &&
         lease.identity == acceptedIdentity_ &&
         acceptedIdentity_ == identity_ && Fresh(now) && !cancelled_;
}
bool Controller::PinCold() {
  if (!RetireSceneFence())
    return false;
  if (live_ || sceneLease_ || sceneFence_ || coldPin_ || transport_.Busy())
    return false;
  coldPin_ = true;
  return true;
}
uint32_t Controller::CaptureSpan(bool full, uint32_t *p, size_t size,
                                 size_t &written) {
  if (!coldPin_)
    return 34;
  return transport_.CaptureSpan(full, p, size, written);
}
std::string Controller::ColdToken() const {
  auto hash = Hash(permit_.data(), sizeof(permit_));
  return std::string("G13R1:") + std::to_string(kBoot) + ":" +
         std::to_string(issuedNonce_) + ":" + Hex(hash.data());
}
std::string Controller::RootReceipt() const {
  std::ostringstream o;
  o << "{\"clock\":\"CLOCK_MONOTONIC\",\"nonce\":\"" << U64(permit_.data() + 8)
    << "\",\"plan\":\"" << U64(permit_.data() + 10) << "\",\"nativePollNs\":\""
    << lastRootPoll_ << "\",\"startedNs\":\"" << U64(permit_.data() + 36)
    << "\",\"deadlineNs\":\"" << U64(permit_.data() + 38)
    << "\",\"nativeError\":" << lastRootError_
    << ",\"nativeOutcome\":" << lastRootOutcome_
    << ",\"gpuTailAvailable\":" << (lastRootGpuTail_ ? "true" : "false")
    << ",\"domain\":\""
    << (nativeControl_ ? "COMPUTE_ONLY_NATIVE_CONTROL" : "PAGE_NATIVE_CONTROL")
    << "\"}";
  return o.str();
}
std::string Controller::FieldToken() const {
  std::ostringstream o;
  o << "G13F1:" << kBoot << ':' << acceptedIdentity_.run << ':'
    << acceptedIdentity_.owner << ':' << acceptedIdentity_.context << ':'
    << acceptedIdentity_.surface << ':' << acceptedIdentity_.geometry << ':'
    << acceptedIdentity_.config << ':' << acceptedIdentity_.session << ':'
    << acceptedIdentity_.permission << ':' << acceptedIdentity_.inputEpoch
    << ':' << acceptedIdentity_.control << ':' << commit_ << ':'
    << acceptedNonce_ << ':' << Hex(config_.data() + 76);
  return o.str();
}
std::string Controller::Status() const {
  std::ostringstream o;
  o << "{\"route\":13,\"schema\":1,\"fieldToken\":" << Quote(FieldToken())
    << ",\"coldToken\":" << Quote(ColdToken()) << ",\"consumedSourceSeq\":\""
    << lastSource_ << "\",\"lastRoot\":" << RootReceipt()
    << ",\"motionBits\":[";
  if (acceptedTail_[255] == kSeal)
    for (size_t i = 0; i < 6; i++) {
      if (i)
        o << ',';
      o << acceptedTail_[136 + i];
    }
  o << "],\"state\":\""
    << (failure_  ? ((failure_ == 7 || failure_ == 8 || failure_ == 9 ||
                     failure_ == 11)
                         ? "SUSPENDED"
                     : (failure_ == 12 || failure_ == 13 || failure_ == 14 ||
                       failure_ == 15 || failure_ == 23)
                         ? "NUMERIC_BLOCKED"
                         : "FAULT")
        : Ready() ? "COMPUTE_READY"
                  : "CONFIG_PENDING")
    << "\",\"firstError\":" << failure_ << ",\"commit\":\"" << commit_
    << "\",\"acceptedQuantum\":\"" << quantum_ << "\",\"nonce\":\"" << nonce_
    << "\",\"queued\":" << count_ << ",\"earned\":\"" << earned_
    << "\",\"reserved\":\"" << reserved_ << "\",\"consumed\":\"" << consumed_
    << "\",\"discarded\":\"" << discarded_ << "\",\"capability\":"
    << (transport_.Capabilities().empty() ? "null" : transport_.Capabilities())
    << ",\"compile\":"
    << (transport_.CompileReceipt().empty() ? "null"
                                            : transport_.CompileReceipt())
    << ",\"physicsValidation\":\"NOT_MEASURED\",\"displayPresentVerified\":"
       "false}";
  return o.str();
}
} // namespace grs::g13
