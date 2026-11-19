#include "controller.h"
#include "implementation_generated.h"
#include <sstream>
namespace grs::g9rx {
namespace {
uint64_t ScopeWord(const ControlIdentity &id, size_t n) {
  const uint64_t words[] = {id.run,        id.owner,       id.context,
                            id.inputEpoch, id.pageSession, id.permission,
                            id.window,     id.config,      id.surface};
  return words[n];
}
void CopyDigest(const Digest &hash, uint32_t *at) {
  std::copy(hash.begin(), hash.end(), at);
}
bool CompleteScope(const ControlIdentity &id) {
  for (size_t i = 0; i < 9; ++i)
    if (!ScopeWord(id, i))
      return false;
  return true;
}
bool SameBody(const Input128 &a, const Input128 &b) {
  return a[2] == b[2] && Pair(a.data(), 4) == Pair(b.data(), 4) &&
         Pair(a.data(), 12) == Pair(b.data(), 12) &&
         std::equal(a.begin() + 26, a.begin() + 34, b.begin() + 26) &&
         std::equal(a.begin() + 40, a.end(), b.begin() + 40);
}
} // namespace
Controller::Controller() {
  static_assert(sizeof(ProgressClockV3::Intent) == 16);
  clock_.intents.ptr = protocol_.payload.bridge.data() + 1168;
}
void Controller::RevokeLocked(uint32_t why) {
  protocol_.Revoke(why);
  clock_.RevokeLocked({ControlResult::STALE_REJECTED});
  run_.active = run_.sourcePhase = run_.jobLive = false;
  run_.plan = run_.nonce = 0;
  run_.quantumOrdinal = 0;
  stale_ = lastGood_;
}
bool Controller::RecheckLocked(const Permit &p, uint64_t now) const {
  const auto &ctl = protocol_.payload.control;
  if (!configured_ || !ctl[RxProtocol::ACTIVE] || !ctl[RxProtocol::ROOT_LIVE] ||
      !(p == active_) || p.identity != identity_ || p.baseCommit != commit_ ||
      p.clockGeneration != protocol_.Get(RxProtocol::CLOCK_GEN) ||
      p.lifecycleGeneration != protocol_.Get(RxProtocol::LIFECYCLE_GEN) ||
      protocol_.sync.lifecycleInvalidation.load() != p.lifecycleGeneration ||
      now < p.rootBeginNs || now > p.deadline || !clock_.Fresh(now) ||
      !clock_.ready || !clock_.baseline ||
      clock_.mode != PlanningMode::RUNNABLE || !run_.active ||
      run_.plan != p.plan || run_.nonce != p.nonce || !clock_.jobLive ||
      !run_.jobLive)
    return false;
  if (ctl[RxProtocol::CLAIMED]) {
    auto &input = protocol_.payload.inputs[ctl[RxProtocol::CLAIM_SLOT]];
    if (!protocol_.CurrentAge(now, Pair(input.data(), 43)))
      return false;
  }
  return true;
}
bool Controller::Check(void *user, const Permit &p, uint64_t now) {
  auto &self = *static_cast<Controller *>(user);
  auto &sync = self.protocol_.sync;
  uint64_t pulse = sync.ownerPulseNs.load();
  if (!sync.ownerActive.load() || !sync.ownerRootLive.load() ||
      sync.lifecycleInvalidation.load() != p.lifecycleGeneration ||
      now < p.rootBeginNs || now > p.deadline || now < pulse ||
      now - pulse > ProgressClockV3::GapNs || !(p == self.active_))
    return false;
  const auto &frozen = self.protocol_.payload.inputs[16];
  return frozen[40] == 0 ||
         self.protocol_.CurrentAge(now, Pair(frozen.data(), 43));
}

Result Controller::ConfigureResearch(const Config &config,
                                     const ControlIdentity &id,
                                     const void *loadedFunction) {
  if (configured_ || !config.Valid() || !CompleteScope(id))
    return Result::INVALID;
  auto clockRead = CheckedClockRead();
  auto resolutionRead = ClockResolution();
  uint32_t resolution = uint32_t(resolutionRead.ns);
  auto observedModule = ReadLoadedIdentity(loadedFunction);
  {
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    loaded_ = observedModule;
  }
  if (!clockRead.ok || !resolutionRead.ok || !RoundingQualified() ||
      !loaded_.valid)
    return Result::INPUT_CLOCK_UNVERIFIED;
  uint64_t nextClockGeneration = 0, nextLifecycleGeneration = 0;
  {
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    if (!Add(protocol_.Get(RxProtocol::CLOCK_GEN), 1, nextClockGeneration) ||
        !Add(protocol_.sync.lifecycleInvalidation.load(), 1,
             nextLifecycleGeneration))
      return Result::INVALID;
  }
  auto result = transport_.Create(config, id, {});
  if (result != Result::OK) {
    if(result==Result::SOURCE_IDENTITY_MISMATCH){std::lock_guard<std::mutex> lock(protocol_.sync.protocol);if(!protocol_.payload.control[RxProtocol::FIRST_ERROR])protocol_.payload.control[RxProtocol::FIRST_ERROR]=uint32_t(result);}
    return result;
  }
  const auto *p = transport_.SelectedProfile();
  ProfileRequirements requirements{p->stateBytes, p->workBytes, p->arenaBytes};
  if (transport_.QueryComputeCaps(requirements, &caps_) != Result::OK) {
    transport_.Release(true);
    return Result::UNSUPPORTED;
  }
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  auto intents = clock_.intents;
  clock_ = ProgressClockV3{};
  clock_.intents = intents;
  protocol_.payload = RxProtocolPayload{};
  nonce_ = frame_ = commit_ = quanta_ = generation_ = publication_ = 0;
  tail_ = {};
  lastGood_ = stale_ = false;
  identity_ = id;
  config_ = config;
  config_.seedSha = transport_.SeedHash();
  configured_ = true;
  protocol_.payload.metadata[0] = config.capsule64;
  auto &cap = protocol_.payload.metadata[2];
  cap = {};
  cap[0] = 1129467463;
  cap[1] = 1;
  cap[2] = kProfileCode;
  cap[3] = 31;
  cap[4] = CLOCK_MONOTONIC;
  cap[5] = resolution;
  protocol_.Set(RxProtocol::CLOCK_GEN, nextClockGeneration);
  protocol_.Set(RxProtocol::LIFECYCLE_GEN, nextLifecycleGeneration);
  protocol_.sync.lifecycleInvalidation.store(nextLifecycleGeneration);
  Put(cap.data(), 6, nextClockGeneration);
  Put(cap.data(), 8, nextLifecycleGeneration);
  for (size_t i = 0; i < 9; ++i)
    Put(cap.data(), 10 + 2 * i, ScopeWord(id, i));
  CopyDigest(loaded_.sha, cap.data() + 28);
  CopyDigest(FromHex(kClockCodeSHA), cap.data() + 36);
  CopyDigest(FromHex(kClockSpecSHA), cap.data() + 44);
  CopyDigest(FromHex(kInputPolicySHA), cap.data() + 52);
  cap[60] = cap[61] = cap[62] = 1;
  std::snprintf(reinterpret_cast<char *>(protocol_.payload.bridge.data() + 640),
                1024, "%s", loaded_.receipt);
  clock_.identity = id;
  clock_.open = clock_.ready = clock_.uiFresh = clock_.gravityFresh = true;
  clock_.counters.session = id.pageSession;
  poseObserved_ = clockRead.ns;
  poseSequence_ = 1;
  protocol_.Set(RxProtocol::LAST_OWNER, clockRead.ns);
  return Result::OK;
}
std::string Controller::Session() const {
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  std::ostringstream out;
  out << "{\"scope\":[";
  for (size_t i = 0; i < 9; ++i) {
    if (i)
      out << ',';
    out << '"' << ScopeWord(identity_, i) << '"';
  }
  out << "],\"profile\":\"GNR1\",\"domain\":\"NATIVE_RECEIPT_SPATIAL_"
         "CONTROL\"}";
  return out.str();
}
std::string Controller::LoadedReceipt() const {
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  return loaded_.receipt;
}
bool Controller::SetActive(bool value, const ControlIdentity &expected,
                           CheckedTime time) {
  auto resolution = ClockResolution();
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  if (!configured_ || expected != identity_ || !time.ok || !resolution.ok ||
      !RoundingQualified() || !loaded_.valid)
    return false;
  if (!value) {
    RevokeLocked(31);
    return true;
  }
  if (!clock_.open) {
    if (!run_.workspaceRetired ||
        protocol_.payload.control[RxProtocol::ROOT_LIVE] || lease_ ||
        sceneFence_ || coldPin_ || identity_.inputEpoch == UINT64_MAX ||
        identity_.pageSession == UINT64_MAX ||
        protocol_.Get(RxProtocol::CLOCK_GEN) == UINT64_MAX ||
        protocol_.sync.lifecycleInvalidation.load() == UINT64_MAX)
      return false;
    auto next = identity_;
    ++next.inputEpoch;
    ++next.pageSession;
    auto result = clock_.BeginSessionLocked({next});
    if (result != ControlResult::OK)
      return false;
    identity_ = next;
    clock_.ready = clock_.uiFresh = clock_.gravityFresh = true;
    uint64_t generation = protocol_.sync.lifecycleInvalidation.load() + 1;
    protocol_.sync.lifecycleInvalidation.store(generation);
    protocol_.Set(RxProtocol::LIFECYCLE_GEN, generation);
    protocol_.Set(RxProtocol::CLOCK_GEN,
                  protocol_.Get(RxProtocol::CLOCK_GEN) + 1);
    auto &cap = protocol_.payload.metadata[2];
    cap[3] = 31;
    cap[5] = resolution.ns;
    Put(cap.data(), 6, protocol_.Get(RxProtocol::CLOCK_GEN));
    Put(cap.data(), 8, generation);
    for (size_t i = 0; i < 9; ++i)
      Put(cap.data(), 10 + 2 * i, ScopeWord(identity_, i));
    protocol_.payload.control[RxProtocol::SIZE] =
        protocol_.payload.control[RxProtocol::HEAD] =
            protocol_.payload.control[RxProtocol::CLAIMED] =
                protocol_.payload.control[RxProtocol::STROKE] = 0;
    clock_.intents.Clear();
    protocol_.payload.metadata[3][2] = 0;
    poseObserved_ = time.ns;
  }
  protocol_.payload.control[RxProtocol::ACTIVE] = 1;
  protocol_.sync.ownerActive.store(true);
  return true;
}
void Controller::InvalidateGeometry() {
  protocol_.sync.CancelStroke();
  protocol_.sync.mappingPending.store(true);
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  if (configured_)
    RevokeLocked(41);
}

ControlResult Controller::Pulse(const PagePulseV3 &pulse, CheckedTime now) {
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  if (!now.ok) {
    RevokeLocked(1);
    return ControlResult::CONTROL_CLOCK_REVERSED;
  }
  auto result = clock_.PulseLocked(pulse, now.ns);
  if (result == ControlResult::OK) {
    protocol_.sync.ownerPulseNs.store(now.ns);
    poseObserved_ = now.ns;
    if (poseSequence_ == UINT64_MAX) {
      RevokeLocked(4);
      return ControlResult::QUOTA_OVERFLOW;
    }
    ++poseSequence_;
  } else if (result != ControlResult::STALE_REJECTED &&
             result != ControlResult::SEQUENCE_REJECTED)
    RevokeLocked(uint32_t(result));
  return result;
}
ControlResult Controller::Target(const ControlIdentity &id, int32_t angle,
                                 CheckedTime now) {
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  if (!now.ok) {
    RevokeLocked(1);
    return ControlResult::CONTROL_CLOCK_REVERSED;
  }
  int32_t old = clock_.target;
  auto result = clock_.RegisterTargetLocked(id, angle);
  if (result == ControlResult::OK) {
    protocol_.sync.ownerPulseNs.store(now.ns);
    poseObserved_ = now.ns;
    if (old != angle) {
      protocol_.payload.control[RxProtocol::STROKE] = 0;
      protocol_.payload.metadata[3][2] = 0;
      if (!protocol_.payload.control[RxProtocol::ROOT_LIVE]) {
        while (protocol_.payload.control[RxProtocol::SIZE])
          DropHeadLocked(now.ns, 41);
      } else {
        RevokeLocked(41);
      }
    }
  }
  return result;
}
void Controller::DropHeadLocked(uint64_t now, uint32_t why) {
  auto &c = protocol_.payload.control;
  uint32_t at = c[RxProtocol::HEAD];
  auto &input = protocol_.payload.inputs[at];
  protocol_.Journal(5, why, now, Pair(input.data(), 41), 0,
                    Pair(input.data(), 43));
  input = {};
  protocol_.payload.witnesses[at] = {};
  clock_.intents.Set(at, {});
  c[RxProtocol::HEAD] = (at + 1) % 16;
  --c[RxProtocol::SIZE];
}
void Controller::ExpireLocked(uint64_t now) {
  auto &c = protocol_.payload.control;
  if (c[RxProtocol::ROOT_LIVE] || transport_.Busy() || c[RxProtocol::CLAIMED])
    return;
  for (size_t i = 0; i < 16 && c[RxProtocol::SIZE]; ++i) {
    const auto &input = protocol_.payload.inputs[c[RxProtocol::HEAD]];
    if (protocol_.CurrentAge(now, Pair(input.data(), 43)))
      break;
    DropHeadLocked(now, 32);
  }
}
Result Controller::OfferSource(Input128 body, uint64_t intent,
                               CheckedTime receipt) {
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  auto &c = protocol_.payload.control;
  if (!receipt.ok || !configured_ || !c[RxProtocol::ACTIVE] || !intent)
    return Result::STALE;
  body[0] = 1481788999;
  body[1] = 3;
  body[2] = kProfileCode;
  body[3] = 0;
  Put(body.data(), 4, identity_.run);
  Put(body.data(), 12, identity_.inputEpoch);
  CopyDigest(config_.parameterSha, body.data() + 26);
  body[40] = 1;
  for (size_t i = 45; i < 64; ++i)
    if (body[i])
      return Result::INVALID;
  for (size_t i = 77; i < 128; ++i)
    if (body[i])
      return Result::INVALID;
  for (size_t i = 0; i < c[RxProtocol::SIZE]; ++i) {
    uint32_t slot = (c[RxProtocol::HEAD] + i) % 16;
    if (clock_.intents.Get(slot).generation == clock_.sourceControlGeneration &&
        clock_.intents.Get(slot).sequence == intent) {
      const auto &old = protocol_.payload.inputs[slot];
      return std::equal(old.begin() + 64, old.begin() + 77, body.begin() + 64)
                 ? Result::OK
                 : Result::INVALID;
    }
  }
  if (intent == protocol_.Get(72) &&
      Pair(protocol_.payload.inputs[17].data(), 4) == identity_.run &&
      Pair(protocol_.payload.inputs[17].data(), 12) == identity_.inputEpoch) {
    const auto &old = protocol_.payload.inputs[17];
    return std::equal(old.begin() + 64, old.begin() + 77, body.begin() + 64)
               ? Result::OK
               : Result::INVALID;
  }
  if (intent <= clock_.counters.sourceIntentSeq)
    return Result::STALE;
  uint64_t command = 0;
  if (!Add(protocol_.Get(RxProtocol::COMMAND_SEQ), 1, command))
    return Result::INVALID;
  Put(body.data(), 41, command);
  Put(body.data(), 43, receipt.ns);
  if (c[RxProtocol::SIZE] >= 16)
    return Result::WOULD_BLOCK;
  uint32_t at = (c[RxProtocol::HEAD] + c[RxProtocol::SIZE]) % 16;
  SourceIntentKey key{identity_.run,
                      identity_.owner,
                      identity_.context,
                      identity_.inputEpoch,
                      identity_.pageSession,
                      clock_.sourceControlGeneration,
                      intent};
  InputPacket packet{body};
  auto registered = clock_.RegisterSourceIntentLocked(key, packet, at);
  if (registered == ControlResult::ALREADY_RETIRED)
    return Result::STALE;
  if (registered != ControlResult::OK)
    return Result::INVALID;
  protocol_.Set(RxProtocol::COMMAND_SEQ, command);
  protocol_.payload.inputs[at] = body;
  protocol_.payload.witnesses[at] = {};
  ++c[RxProtocol::SIZE];
  return Result::OK;
}
bool Controller::FreezeLocked(uint64_t now, uint64_t frame, uint64_t geometry,
                              Input128 &frozen, Permit &p) {
  auto &c = protocol_.payload.control;
  bool source = c[RxProtocol::SIZE] &&
                protocol_.payload.inputs[c[RxProtocol::HEAD]][40] == 1;
  if (!run_.active) {
    PlanGrantV3 grant;
    if (!clock_.ClaimLocked(now, source, run_, grant))
      return false;
  }
  source = run_.sourcePhase;
  uint32_t slot = c[RxProtocol::HEAD];
  bool command = c[RxProtocol::SIZE] &&
                 ((source && protocol_.payload.inputs[slot][40] == 1) ||
                  (!source && protocol_.payload.inputs[slot][40] == 2));
  if (source && !command)
    return false;
  if (nonce_ == UINT64_MAX || generation_ == UINT64_MAX || frame < frame_ ||
      !geometry || !Add(now, 250000000, p.deadline))
    return false;
  frozen = command ? protocol_.payload.inputs[slot] : Input128{};
  if (command && !protocol_.CurrentAge(now, Pair(frozen.data(), 43)))
    return false;
  p.identity = identity_;
  p.nonce = ++nonce_;
  p.plan = run_.plan;
  p.frame = frame;
  p.control = frame;
  p.geometry = geometry;
  p.rootBeginNs = now;
  p.clockGeneration = protocol_.Get(RxProtocol::CLOCK_GEN);
  p.lifecycleGeneration = protocol_.Get(RxProtocol::LIFECYCLE_GEN);
  p.baseCommit = commit_;
  p.quanta = quanta_;
  p.kind = source ? 1 : 2;
  p.ordinal = source ? 0 : run_.quantumOrdinal + 1;
  p.baseBank = transport_.CurrentBank();
  p.candidateBank = 2;
  p.arena = generation_ & 1;
  p.arenaGeneration = generation_ + 1;
  p.parameterSha = config_.parameterSha;
  p.layoutSha = FromHex(kLayoutSHA);
  p.shaderSha = transport_.ShaderHash();
  frozen[0] = 1481788999;
  frozen[1] = 3;
  frozen[2] = kProfileCode;
  Put(frozen.data(), 4, identity_.run);
  Put(frozen.data(), 6, identity_.owner);
  Put(frozen.data(), 8, identity_.context);
  Put(frozen.data(), 10, p.nonce);
  Put(frozen.data(), 12, identity_.inputEpoch);
  Put(frozen.data(), 14, poseSequence_);
  Put(frozen.data(), 16, poseObserved_);
  Put(frozen.data(), 18, now);
  Put(frozen.data(), 20, now);
  frozen[22] = 2;
  frozen[23] = 2;
  frozen[24] = uint32_t(clock_.grant.target);
  frozen[25] = 1;
  CopyDigest(config_.parameterSha, frozen.data() + 26);
  auto freezeTime = CheckedClockRead();
  if (!freezeTime.ok || freezeTime.ns < now) {
    RevokeLocked(1);
    return false;
  }
  if (freezeTime.ns > p.deadline) {
    RevokeLocked(42);
    return false;
  }
  if (command &&
      !protocol_.CurrentAge(freezeTime.ns, Pair(frozen.data(), 43))) {
    if (freezeTime.ns >= Pair(frozen.data(), 43)) {
      DropHeadLocked(freezeTime.ns, 32);
      run_.sourcePhase = false;
      if (!clock_.activeRemaining)
        clock_.RetireSourcePlanLocked(run_.plan, run_);
    }
    return false;
  }
  Put(frozen.data(), 18, freezeTime.ns);
  Put(frozen.data(), 20, freezeTime.ns);
  p.inputSha = HashWords(frozen.data(), 128);
  protocol_.payload.inputs[16] = frozen;
  protocol_.payload.witnesses[17] = {};
  std::fill(protocol_.FrameCommit(17), protocol_.FrameCommit(17) + 4, 0u);
  protocol_.payload.witnesses[18] = protocol_.payload.witnesses[19];
  std::copy(protocol_.FrameCommit(19), protocol_.FrameCommit(19) + 4,
            protocol_.FrameCommit(18));
  if (command) {
    c[RxProtocol::CLAIMED] = 1;
    c[RxProtocol::CLAIM_SLOT] = slot;
    if (frozen[40] == 2) {
      auto &record = protocol_.payload.witnesses[17];
      record = protocol_.payload.witnesses[slot];
      record[0] = 859199570;
      record[1] = 1;
      record[2] = 256;
      record[3] = kProfileCode;
      record[4] = 1;
      record[5] = 2;
      record[6] = 3;
      record[7] = 1;
      Put(record.data(), 46, freezeTime.ns);
      Put(record.data(), 48, now);
      Put(record.data(), 50, p.deadline);
      Put(record.data(), 140, p.nonce);
      CopyDigest(p.inputSha, record.data() + 176);
      Put(record.data(), 232, freezeTime.ns - Pair(record.data(), 36));
      record[239] = 15;
      std::copy(protocol_.FrameCommit(slot), protocol_.FrameCommit(slot) + 4,
                protocol_.FrameCommit(17));
    }
  }
  active_ = p;
  c[RxProtocol::ROOT_LIVE] = 1;
  protocol_.sync.ownerRootLive.store(true);
  clock_.jobLive = run_.jobLive = true;
  clock_.activeRootKind = p.kind;
  run_.workspaceRetired = false;
  run_.nonce = p.nonce;
  frame_ = frame;
  protocol_.Journal(2, 0, now, Pair(frozen.data(), 41), p.nonce,
                    Pair(frozen.data(), 43));
  return true;
}
Result Controller::BeginRoot(CheckedTime now, uint64_t frame,
                             uint64_t geometry) {
  if (!now.ok)
    return Result::INPUT_CLOCK_UNVERIFIED;
  if (!PollSceneFence())
    return Result::GL_FAILED;
  Input128 frozen{};
  Permit p;
  {
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    auto &c = protocol_.payload.control;
    if (!configured_ || !c[RxProtocol::ACTIVE] || c[RxProtocol::ROOT_LIVE] ||
        transport_.Busy() || lease_ || sceneFence_ || coldPin_ ||
        !run_.workspaceRetired)
      return Result::WOULD_BLOCK;
    ExpireLocked(now.ns);
    if (!FreezeLocked(now.ns, frame, geometry, frozen, p))
      return Result::WOULD_BLOCK;
  }
  auto result = transport_.Begin(p, frozen, Check, this);
  if (result != Result::OK) {
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    RevokeLocked(uint32_t(result));
  }
  return result;
}
Result Controller::PumpBounded(CheckedTime time) {
  if (!time.ok)
    return Result::INPUT_CLOCK_UNVERIFIED;
  auto result = transport_.Pump();
  if (result == Result::IN_PROGRESS)
    return result;
  if (result != Result::OK) {
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    RevokeLocked(uint32_t(result));
    return result;
  }
  auto accepted = transport_.AcceptedProof();
  auto blocked = transport_.BlockedProof();
  auto now = CheckedClockRead();
  if (!now.ok)
    return Result::INPUT_CLOCK_UNVERIFIED;
  if (blocked) {
    if (!transport_.RetireRejected(active_))
      return Result::IN_PROGRESS;
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    if (planning_.BlockPlanningControlLocked(*blocked, now.ns) !=
        ControlResult::OK) {
      RevokeLocked(33);
      return Result::INVALID;
    }
    protocol_.payload.control[RxProtocol::ROOT_LIVE] =
        protocol_.payload.control[RxProtocol::CLAIMED] = 0;
    run_.workspaceRetired = true;
    protocol_.sync.ownerRootLive.store(false);
    tail_ = blocked->Words();
    stale_ = lastGood_;
    protocol_.payload.metadata[3][2] = 0;
    protocol_.Journal(4, 4, now.ns, 0, active_.nonce);
    return Result::OK;
  }
  if (!accepted) {
    std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
    RevokeLocked(34);
    return Result::INVALID;
  }
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  if (!RecheckLocked(active_, now.ns)) {
    RevokeLocked(36);
    return Result::STALE;
  }
  auto nextClock = clock_;
  auto nextRun = run_;
  PlanningControl nextPlanning(nextClock, nextRun);
  if (active_.kind == 2) {
    if (nextPlanning.AcceptQuantumLocked(*accepted, now.ns) !=
        ControlResult::OK) {
      RevokeLocked(37);
      return Result::STALE;
    }
  } else {
    nextClock.jobLive = nextRun.jobLive = false;
    nextClock.activeRootKind = 0;
    nextRun.sourcePhase = false;
    if (!nextClock.activeRemaining) {
      nextRun.workspaceRetired = true;
      if (nextClock.RetireSourcePlanLocked(active_.plan, nextRun) !=
          ControlResult::OK) {
        RevokeLocked(40);
        return Result::STALE;
      }
    }
  }
  if (!transport_.CommitAccepted(*accepted)) {
    RevokeLocked(35);
    return Result::STALE;
  }
  clock_ = nextClock;
  run_ = nextRun;
  auto &c = protocol_.payload.control;
  const auto &w = accepted->Words();
  if (c[RxProtocol::CLAIMED]) {
    uint32_t at = c[RxProtocol::CLAIM_SLOT];
    if (at != c[RxProtocol::HEAD]) {
      RevokeLocked(38);
      return Result::INVALID;
    }
    auto &original = protocol_.payload.inputs[at];
    if (original[40] == 2) {
      protocol_.payload.inputs[18] = protocol_.payload.inputs[16];
      protocol_.payload.witnesses[19] = protocol_.payload.witnesses[17];
      protocol_.payload.witnesses[19][4] = 2;
      std::copy(protocol_.FrameCommit(17), protocol_.FrameCommit(17) + 4,
                protocol_.FrameCommit(19));
    } else {
      protocol_.payload.inputs[17] = protocol_.payload.inputs[16];
      protocol_.Set(72, clock_.intents.Get(at).sequence);
    }
    clock_.intents.Set(at, {});
    c[RxProtocol::HEAD] = (at + 1) % 16;
    --c[RxProtocol::SIZE];
    c[RxProtocol::CLAIMED] = 0;
  }
  commit_ = Pair(w.data(), 30);
  quanta_ = Pair(w.data(), 54);
  if (w[6] != 2)
    ++generation_;
  tail_ = w;
  lastGood_ = true;
  stale_ = w[6] == 2;
  if (stale_)
    protocol_.payload.metadata[3][2] = 0;
  c[RxProtocol::ROOT_LIVE] = 0;
  protocol_.sync.ownerRootLive.store(false);
  run_.workspaceRetired = true;
  if (run_.active && active_.kind == 2)
    run_.quantumOrdinal = active_.ordinal;
  protocol_.Journal(3, w[6], transport_.CommitObservedNs(),
                    Pair(protocol_.payload.inputs[16].data(), 41),
                    active_.nonce,
                    Pair(protocol_.payload.inputs[16].data(), 43), commit_);
  return Result::OK;
}
Result Controller::Stop() {
  protocol_.sync.ownerActive.store(false);
  protocol_.sync.CancelStroke();
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  RevokeLocked(39);
  return Result::OK;
}
bool Controller::Drain() {
  if (transport_.Busy() && !transport_.RetireRejected(active_))
    return false;
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  protocol_.payload.control[RxProtocol::ROOT_LIVE] =
      protocol_.payload.control[RxProtocol::CLAIMED] = 0;
  run_.workspaceRetired = true;
  protocol_.sync.ownerRootLive.store(false);
  return true;
}
void Controller::Release(bool current) {
  if (current && sceneFence_)
    glDeleteSync(sceneFence_);
  sceneFence_ = nullptr;
  transport_.Release(current);
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  RevokeLocked(40);
  configured_ = lease_ = false;
  run_.workspaceRetired = true;
}
Controller::SceneView Controller::AcquireScene() {
  if (!PollSceneFence())
    return {};
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  if (!configured_ || !lastGood_ ||
      protocol_.payload.control[RxProtocol::ROOT_LIVE] || transport_.Busy() ||
      lease_ || sceneFence_ || coldPin_)
    return {};
  lease_ = true;
  return {
      true,        stale_,    transport_.StateBuffer(transport_.CurrentBank()),
      generation_, identity_, config_.k};
}
bool Controller::ReleaseScene(uint64_t generation, GLsync fence) {
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  if (!lease_ || generation != generation_ || !fence || sceneFence_)
    return false;
  lease_ = false;
  sceneFence_ = fence;
  return true;
}
bool Controller::PollSceneFence() {
  if (!sceneFence_)
    return true;
  auto result = glClientWaitSync(sceneFence_, 0, 0);
  if (result == GL_WAIT_FAILED)
    return false;
  if (result == GL_TIMEOUT_EXPIRED)
    return true;
  glDeleteSync(sceneFence_);
  sceneFence_ = nullptr;
  return glGetError() == GL_NO_ERROR;
}
bool Controller::CanPresent(uint64_t generation, uint64_t now) const {
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  return configured_ && lastGood_ && !stale_ && generation == generation_ &&
         clock_.ready && clock_.open && clock_.Fresh(now) &&
         protocol_.payload.control[RxProtocol::ACTIVE] &&
         now <= active_.deadline &&
         int32_t(protocol_.payload.inputs[16][24]) == clock_.target;
}
std::string Controller::Status() const {
  std::lock_guard<std::mutex> lock(protocol_.sync.protocol);
  std::ostringstream out;
  out << "{\"profile\":\"GNR1\",\"domain\":\"NATIVE_RECEIPT_SPATIAL_CONTROL\","
         "\"configured\":"
      << (configured_ ? "true" : "false") << ",\"active\":"
      << (protocol_.payload.control[RxProtocol::ACTIVE] ? "true" : "false")
      << ",\"commit\":\"" << commit_ << "\",\"quanta\":\"" << quanta_
      << "\",\"queue\":" << protocol_.payload.control[RxProtocol::SIZE]
      << ",\"sceneStale\":" << (stale_ ? "true" : "false")
      << ",\"mode\":" << uint32_t(clock_.mode) << ",\"firstFailure\":"
      << protocol_.payload.control[RxProtocol::FIRST_ERROR]
      << ",\"rawClockErrno\":" << protocol_.payload.control[76]
      << ",\"clockApiStage\":" << protocol_.payload.control[77]
      << ",\"capability\":[";
  for (size_t i = 0; i < 64; ++i) {
    if (i)
      out << ',';
    out << protocol_.Capability()[i];
  }
  out << "],\"mapping\":[";
  for (size_t i = 0; i < 64; ++i) {
    if (i)
      out << ',';
    out << protocol_.Mapping()[i];
  }
  out << "]}";
  return out.str();
}

bool Controller::PublishMapping(uint64_t generation, uint32_t width,
                                uint32_t height, double density,
                                CheckedTime time) {
  std::unique_lock<std::mutex> lock(protocol_.sync.protocol, std::try_to_lock);
  if (!lock.owns_lock()) {
    protocol_.sync.CancelStroke();
    protocol_.sync.mappingPending.store(true);
    return false;
  }
  auto &map = protocol_.payload.metadata[3];
  if (!time.ok || !configured_ || !lastGood_ || stale_ ||
      generation != generation_ || !width || !height ||
      !std::isfinite(density) || density <= 0 || publication_ == UINT64_MAX) {
    map[2] = 0;
    return false;
  }
  bool changed = map[45] != width || map[46] != height ||
                 Real(map.data(), 38) != density ||
                 int32_t(map[44]) != int32_t(protocol_.payload.inputs[16][24]);
  uint64_t mapping = Pair(map.data(), 8), geometry = Pair(map.data(), 4),
           viewport = Pair(map.data(), 6);
  if (changed) {
    if (mapping == UINT64_MAX || geometry == UINT64_MAX ||
        viewport == UINT64_MAX) {
      RevokeLocked(4);
      return false;
    }
    ++mapping;
    ++geometry;
    ++viewport;
    protocol_.payload.control[RxProtocol::STROKE] = 0;
  }
  map = {};
  map[0] = 1347244615;
  map[1] = 1;
  map[2] = 7;
  map[3] = 1;
  Put(map.data(), 4, geometry);
  Put(map.data(), 6, viewport);
  Put(map.data(), 8, mapping);
  Put(map.data(), 26, commit_);
  Put(map.data(), 28, ++publication_);
  RealPut(map.data(), 34, width);
  RealPut(map.data(), 36, height);
  RealPut(map.data(), 38, density);
  map[44] = protocol_.payload.inputs[16][24];
  map[45] = width;
  map[46] = height;
  CopyDigest(config_.parameterSha, map.data() + 48);
  CopyDigest(config_.seedSha, map.data() + 56);
  p011gpu::Sha256 hash;
  for (uint32_t w : {0x4b475247u, 1u, kProfileCode, 1u})
    HashWord(hash, w);
  for (size_t i = 0; i < 9; ++i) {
    uint64_t v = ScopeWord(identity_, i);
    HashWord(hash, v);
    HashWord(hash, v >> 32);
  }
  for (size_t i = 4; i < 10; ++i)
    HashWord(hash, map[i]);
  for (size_t i = 30; i < 38; ++i)
    HashWord(hash, map[i]);
  for (size_t i = 38; i < 44; ++i)
    HashWord(hash, map[i]);
  HashWord(hash, map[44]);
  for (size_t i = 48; i < 64; ++i)
    HashWord(hash, map[i]);
  for (auto w : FromHex(kMappingSpecSHA))
    HashWord(hash, w);
  auto geometrySha = Finish(hash);
  CopyDigest(geometrySha, map.data() + 10);
  p011gpu::Sha256 frame;
  for (uint32_t w : {0x57465247u, 1u, kProfileCode})
    HashWord(frame, w);
  for (size_t i = 0; i < 9; ++i) {
    uint64_t v = ScopeWord(identity_, i);
    HashWord(frame, v);
    HashWord(frame, v >> 32);
  }
  for (auto w : geometrySha)
    HashWord(frame, w);
  for (size_t i = 26; i < 30; ++i)
    HashWord(frame, map[i]);
  for (auto w : FromHex(kPipelineSHA))
    HashWord(frame, w);
  CopyDigest(Finish(frame), map.data() + 18);
  Put(protocol_.payload.scratch.data(), 252, time.ns);
  protocol_.sync.mappingPending.store(false);
  protocol_.Journal(6, 0, time.ns, 0, active_.nonce, 0, commit_, publication_);
  return true;
}
} // namespace grs::g9rx
