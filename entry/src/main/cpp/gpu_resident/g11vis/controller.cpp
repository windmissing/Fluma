#include "controller.h"
#include <algorithm>
#include <chrono>
#include <sstream>
namespace grs::g11vis {
namespace {
template <class I> I Identity(const Scope &s) {
  I i;
  i.run = s.run;
  i.owner = s.owner;
  i.context = s.context;
  i.inputEpoch = s.inputEpoch;
  i.pageSession = s.pageSession;
  i.permission = s.permission;
  i.window = s.window;
  i.config = s.config;
  i.surface = s.surface;
  return i;
}
bool Same(const Scope &a, const Scope &b) {
  return a.run == b.run && a.owner == b.owner && a.context == b.context &&
         a.inputEpoch == b.inputEpoch && a.pageSession == b.pageSession &&
         a.permission == b.permission && a.window == b.window &&
         a.config == b.config && a.surface == b.surface;
}
std::string ScopeText(const Scope &s) {
  std::ostringstream o;
  o << "{";
  const char *names[] = {"run",        "owner",       "context",
                         "inputEpoch", "pageSession", "permission",
                         "window",     "config",      "surface"};
  const uint64_t values[] = {s.run,        s.owner,       s.context,
                             s.inputEpoch, s.pageSession, s.permission,
                             s.window,     s.config,      s.surface};
  for (size_t i = 0; i < 9; ++i) {
    if (i)
      o << ',';
    o << '"' << names[i] << "\":\"" << values[i] << '"';
  }
  o << '}';
  return o.str();
}
} // namespace
uint64_t Controller::Now() {
  return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count());
}
bool Controller::Check(void *u, const CompositePermit &p, uint64_t now) {
  auto &self = *static_cast<Controller *>(u);
  std::lock_guard<std::recursive_mutex> l(self.joint_);
  return self.Recheck(p, std::max(now, Now()));
}
bool Controller::Recheck(const CompositePermit &p, uint64_t now) const {
  if (revoked_.load(std::memory_order_acquire) || !configured_ || !root_ ||
      !(p == active_) || now > p.Deadline())
    return false;
  return std::visit(
      [&](const auto &s) {
        using S = std::decay_t<decltype(s)>;
        using M = typename S::Mode;
        return s.clock.ready && s.clock.baseline && s.clock.Fresh(now) &&
               s.clock.mode == M::RUNNABLE && s.clock.jobLive &&
               s.run.jobLive && s.run.active &&
               s.run.plan == p.Visit([](const auto &q) { return q.plan; }) &&
               s.run.nonce == p.Nonce();
      },
      control_);
}
void Controller::Fault(const char *why) {
  std::lock_guard<std::recursive_mutex> l(joint_);
  revoked_.store(true, std::memory_order_release);
  invalidated_.store(true, std::memory_order_release);
  result_ = why;
  root_ = false;
  stale_ = gpu_.HasValidatedPublication();
  std::visit(
      [](auto &s) {
        using S = std::decay_t<decltype(s)>;
        using C = typename S::Code;
        s.clock.RevokeLocked({C::STALE_REJECTED});
        s.run.active = s.run.sourcePhase = s.run.jobLive = false;
        s.run.plan = s.run.nonce = 0;
      },
      control_);
  cold_.Invalidate();
  gpu_.Witness().Invalidate();
  if (!gpu_.Initialized())
    gpu_.InvalidateBootstrap();
}
Controller::Result Controller::Configure(const ParentAdapter &parent,
                                         const Words64 &config,
                                         const Scope &id) {
  if (configured_ || gpu_.Parent() || !cold_.Empty() || sceneFence_ ||
      !id.run || !id.owner || !id.context || !id.inputEpoch ||
      !id.pageSession || !id.surface)
    return Result::WOULD_BLOCK;
  invalidated_.store(false, std::memory_order_release);
  id_ = id;
  if (parent.Kind() == ParentKind::DM)
    control_.emplace<DMControl>();
  else
    control_.emplace<R1Control>();
  std::visit(
      [&](auto &s) {
        using S = std::decay_t<decltype(s)>;
        s.clock.identity = Identity<typename S::Identity>(id);
        s.clock.identity.pageSession = 0;
        s.gate.Configure(std::get<typename S::Config>(parent.Configuration()),
                         Identity<typename S::Identity>(id));
      },
      control_);
  const auto result = gpu_.Create(parent, config, id);
  configured_ = result == Result::IN_PROGRESS;
  result_ = configured_ ? "BOOTSTRAP_READING" : "CONFIGURE_FAILED";
  rawReady_ = session_ = publication_ = stale_ = false;
  nonce_ = commit_ = quanta_ = frame_ = 0;
  head_ = size_ = 0;
  consumedSource_ = consumedImpulse_ = {};
  sourceConsumed_ = impulseConsumed_ = false;
  presented_.clear();
  return result;
}
bool Controller::SealBootstrap() {
  if (invalidated_.load(std::memory_order_acquire) || !configured_ ||
      !rawReady_ || gpu_.Initialized() || !gpu_.SealBootstrap(Now()))
    return false;
  result_ = "READY_INACTIVE";
  return true;
}
std::string Controller::BootstrapPage(uint32_t segment, uint32_t offset) {
  if (invalidated_.load(std::memory_order_acquire))
    return "{\"state\":\"STALE_SCOPE\"}";
  return gpu_.BootstrapPage(segment, offset, Now());
}
std::string Controller::BootstrapState() const {
  return gpu_.Bootstrap().Manifest();
}
bool Controller::BeginSession(const Scope &id) {
  std::lock_guard<std::recursive_mutex> l(joint_);
  if (!configured_ || !gpu_.Initialized() || session_ || !Same(id_, id))
    return false;
  bool ok = std::visit(
      [&](auto &s) {
        using S = std::decay_t<decltype(s)>;
        using C = typename S::Code;
        return s.clock.BeginSessionLocked(typename S::Handoff{
                   Identity<typename S::Identity>(id)}) == C::OK;
      },
      control_);
  session_ = ok;
  return ok;
}
bool Controller::SetActive(bool on, const Scope &id) {
  if (!on) {
    if (!Same(id_, id))
      return false;
    Stop();
    return true;
  }
  std::lock_guard<std::recursive_mutex> l(joint_);
  if (invalidated_.load(std::memory_order_acquire) || !session_ ||
      !Same(id_, id) || !gpu_.Initialized())
    return false;
  bool ok = std::visit(
      [](auto &s) {
        using S = std::decay_t<decltype(s)>;
        using C = typename S::Code;
        if (!s.clock.open || s.clock.fault != C::OK)
          return false;
        s.clock.ready = s.clock.uiFresh = s.clock.gravityFresh = true;
        return true;
      },
      control_);
  if (ok) {
    revoked_.store(false, std::memory_order_release);
    result_ = "ACTIVE_WAIT_PULSE";
  }
  return ok;
}
bool Controller::Pulse(uint64_t session, uint64_t seq, uint64_t ui) {
  std::lock_guard<std::recursive_mutex> l(joint_);
  if (!configured_ || !session_ || session != id_.pageSession ||
      revoked_.load())
    return false;
  const auto now = Now();
  bool ok = std::visit(
      [&](auto &s) {
        using S = std::decay_t<decltype(s)>;
        using C = typename S::Code;
        auto rc = s.clock.PulseLocked(
            typename S::Pulse{Identity<typename S::Identity>(id_), seq, ui},
            now);
        if (rc == C::OK)
          s.gate.Pose(s.clock.target, now);
        return rc == C::OK;
      },
      control_);
  if (!ok)
    Fault("PULSE_REJECTED");
  return ok;
}
bool Controller::Target(int32_t angle, const Scope &id) {
  std::lock_guard<std::recursive_mutex> l(joint_);
  if (!Same(id_, id) || revoked_.load())
    return false;
  const auto now = Now();
  return std::visit(
      [&](auto &s) {
        using S = std::decay_t<decltype(s)>;
        using C = typename S::Code;
        auto rc = s.clock.RegisterTargetLocked(
            Identity<typename S::Identity>(id_), angle);
        if (rc == C::OK)
          s.gate.Pose(angle, now);
        return rc == C::OK;
      },
      control_);
}
Controller::Result Controller::Offer(Input128 raw, uint64_t intent) {
  std::lock_guard<std::recursive_mutex> l(joint_);
  if (!configured_ || revoked_.load() || !session_)
    return Result::STALE;
  return std::visit(
      [&](auto &s) -> Result {
        using S = std::decay_t<decltype(s)>;
        using C = typename S::Code;
        auto rc = s.gate.Offer(raw, Now());
        if (uint32_t(rc) != 0)
          return Result(uint32_t(rc));
        if (!raw[40])
          return Result::OK;
        const auto &consumed =
            raw[40] == 1 ? consumedSource_ : consumedImpulse_;
        const bool has = raw[40] == 1 ? sourceConsumed_ : impulseConsumed_;
        auto same = [](const Input128 &a, const Input128 &b) {
          return g9spf::SameCommand(a, b);
        };
        const auto seq = accept647::Pair(raw.data(), 41);
        if (has) {
          const auto old = accept647::Pair(consumed.data(), 41);
          if (seq < old)
            return Result::STALE;
          if (seq == old)
            return same(raw, consumed) ? Result::OK : Result::INVALID;
        }
        for (size_t i = 0; i < size_; ++i) {
          const auto &old = queue_[(head_ + i) % 16];
          if (old[40] == raw[40] && accept647::Pair(old.data(), 41) == seq)
            return same(raw, old) ? Result::OK : Result::INVALID;
        }
        if (size_ == 16)
          return Result::WOULD_BLOCK;
        const size_t slot = (head_ + size_) % 16;
        if (raw[40] == 1) {
          typename S::Intent key{
              id_.run,        id_.owner,       id_.context,
              id_.inputEpoch, id_.pageSession, s.clock.sourceControlGeneration,
              intent};
          typename S::Packet packet;
          packet.words = raw;
          auto next = s.clock;
          if (next.RegisterSourceIntentLocked(key, packet, slot) != C::OK)
            return Result::INVALID;
          s.clock = next;
        }
        queue_[slot] = raw;
        ++size_;
        return Result::OK;
      },
      control_);
}
Controller::Result Controller::Model(const Input128 &body, uint64_t intent) {
  Input128 raw;
  {
    std::lock_guard<std::recursive_mutex> l(joint_);
    if (!std::visit(
            [&](auto &s) {
              return s.gate.ModelEnvelope(body, s.clock.target, Now(), raw);
            },
            control_))
      return Result::INVALID;
  }
  return Offer(raw, intent);
}
Controller::Result Controller::Pump(uint64_t frame, uint64_t geometry) {
  const auto now = Now();
  cold_.Maintain(now);
  if (!configured_)
    return Result::INVALID;
  if (!gpu_.Initialized()) {
    if (invalidated_.load(std::memory_order_acquire)) {
      gpu_.InvalidateBootstrap();
      gpu_.RetireBootstrap();
      return Result::STALE;
    }
    if (rawReady_) {
      if (now >= accept647::Pair(gpu_.Bootstrap().Receipt().words.data(), 84)) {
        Fault("BOOTSTRAP_TTL_EXPIRED");
        gpu_.RetireBootstrap();
        return Result::STALE;
      }
      return Result::WOULD_BLOCK;
    }
    const auto result = gpu_.PollBootstrap(now);
    if (result == Result::OK) {
      rawReady_ = true;
      result_ = "BOOTSTRAP_RAW_READY";
    } else if (result != Result::IN_PROGRESS)
      Fault("BOOTSTRAP_FAILED");
    return result;
  }
  if (!PollSceneFence()) {
    Fault("SCENE_FENCE_FAILED");
    return Result::GL_FAILED;
  }
  if (revoked_.load()) {
    Drain();
    return Result::STALE;
  }
  if (gpu_.Busy()) {
    const auto result = gpu_.Pump();
    if (result == Result::IN_PROGRESS)
      return result;
    if (result != Result::OK) {
      Fault(result == Result::DEADLINE ? "ROOT_DEADLINE" : "ROOT_REJECTED");
      return result;
    }
    // Proof creation observes the actual GL producer outside the joint
    // publication lock.
    auto accepted = gpu_.Accepted();
    auto blocked = gpu_.Blocked();
    bool retire = false;
    {
      std::lock_guard<std::recursive_mutex> l(joint_);
      const auto time = Now();
      if (!Recheck(active_, time)) {
        Fault("LATE_ROOT");
        return Result::STALE;
      }
      if (blocked) {
        bool ok = std::visit(
            [&](auto &s) {
              using S = std::decay_t<decltype(s)>;
              using M = typename S::Mode;
              if (active_.Kind() != 2 || !s.clock.activeRemaining)
                return false;
              auto next = s.clock.counters;
              auto add = [](uint64_t &a, uint64_t b) {
                return g9spf::CheckedAdd(a, b, a);
              };
              if (!add(next.blockedActive, s.clock.activeRemaining) ||
                  !add(next.blockedPending, s.clock.pending) ||
                  !add(next.blockedClearedCarry, next.carry) ||
                  !add(next.blockedEpisodes, 1))
                return false;
              next.carry = 0;
              next.retiredPlan = s.run.plan;
              next.retiredNonce = s.run.nonce;
              next.blockedBasePhysics = commit_;
              next.blockedOnInputSeq = next.meaningfulInputSeq;
              next.blockedOnPulseSeq = next.pulseSequence;
              next.rearmInputSeq = next.rearmAfterPulseSeq = 0;
              if (!s.run.RetireBlockedPlanLocked(s.run.plan, s.run.nonce))
                return false;
              s.clock.counters = next;
              s.clock.pending = s.clock.activeRemaining = 0;
              s.clock.grant = {};
              s.clock.jobLive = false;
              s.clock.activeRootKind = 0;
              s.clock.retiredIdentity = s.clock.identity;
              s.clock.blockReason = blocked->Words()[7];
              s.clock.mode = M::BLOCKED_WAIT;
              s.clock.lastGoodStale = true;
              return true;
            },
            control_);
        if (!ok) {
          Fault("BLOCK_CONTROL_REJECTED");
          return Result::INVALID;
        }
        root_ = false;
        stale_ = publication_;
        result_ = "BLOCKED_WAIT";
        retire = true;
      } else if (accepted) {
        // Prepare all quota changes in a private value, then commit
        // banks+clock+FIFO atomically.
        auto next = control_;
        bool ok = std::visit(
            [&](auto &s) {
              if (active_.Kind() == 2) {
                if (!s.clock.activeRemaining ||
                    s.clock.counters.accepted == UINT64_MAX)
                  return false;
                ++s.clock.counters.accepted;
                --s.clock.activeRemaining;
              }
              s.clock.jobLive = s.run.jobLive = false;
              s.clock.activeRootKind = 0;
              s.run.workspaceRetired = true;
              s.run.sourcePhase = false;
              if (active_.Kind() == 2 && s.clock.activeRemaining == 0) {
                s.run.active = false;
                s.run.plan = s.run.nonce = 0;
                s.run.quantumOrdinal = s.run.continuationStage = 0;
                s.clock.grant = {};
              } else if (s.run.active && active_.Kind() == 2)
                s.run.quantumOrdinal =
                    active_.Visit([](const auto &p) { return p.ordinal; });
              return true;
            },
            next);
        if (!ok || !gpu_.CommitComposite(*accepted, time)) {
          Fault("JOINT_COMMIT_REJECTED");
          return Result::INVALID;
        }
        control_ = std::move(next);
        const auto &tail = accepted->Words();
        commit_ = accept647::Pair(tail.data(), 30);
        quanta_ = accept647::Pair(tail.data(), 54);
        root_ = false;
        stale_ = tail[6] == 2;
        publication_ = !stale_;
        result_ = stale_ ? "DUPLICATE_STALE" : "ACCEPTED";
        if (size_ && ((active_.Kind() == 1 && queue_[head_][40] == 1) ||
                      (active_.Kind() == 2 && queue_[head_][40] == 2))) {
          auto &packet = queue_[head_];
          if (packet[40] == 1) {
            consumedSource_ = packet;
            sourceConsumed_ = true;
          } else {
            consumedImpulse_ = packet;
            impulseConsumed_ = true;
          }
          std::visit([&](auto &s) { s.clock.intents[head_] = {}; }, control_);
          packet = {};
          head_ = (head_ + 1) % 16;
          --size_;
        }
        std::visit(
            [&](auto &s) {
              using S = std::decay_t<decltype(s)>;
              using C = typename S::Code;
              if (active_.Kind() == 1 && !s.clock.activeRemaining &&
                  s.clock.RetireSourcePlanLocked(s.run.plan, s.run) != C::OK)
                Fault("SOURCE_PLAN_RETIRE_REJECTED");
            },
            control_);
      } else {
        Fault("COMPOSITE_PROOF_REJECTED");
        return Result::INVALID;
      }
    }
    if (retire) {
      if (!gpu_.RetireRejected(active_))
        return Result::IN_PROGRESS;
      std::visit([](auto &s) { s.run.workspaceRetired = true; }, control_);
    }
    return Result::OK;
  }
  if (cold_.Building()) {
    cold_.Pump(gpu_, now);
    return Result::WOULD_BLOCK;
  }
  gpu_.Witness().Pump(now);
  Input128 frozen;
  CompositePermit permit;
  bool claimError = false;
  {
    std::lock_guard<std::recursive_mutex> l(joint_);
    if (lease_ || sceneFence_ || root_ || !geometry)
      return Result::WOULD_BLOCK;
    bool ok = std::visit(
        [&](auto &s) {
          using S = std::decay_t<decltype(s)>;
          if (!s.run.workspaceRetired)
            return false;
          bool source = size_ && queue_[head_][40] == 1;
          if (!s.run.active) {
            typename S::Grant grant;
            if (!s.clock.ClaimLocked(now, source, s.run, grant))
              return false;
          }
          source = s.run.sourcePhase;
          const Input128 *command = nullptr;
          if (source) {
            if (!size_ || queue_[head_][40] != 1)
              return false;
            command = &queue_[head_];
          } else if (size_ && queue_[head_][40] == 2)
            command = &queue_[head_];
          typename S::Permit p;
          if (nonce_ == UINT64_MAX || gpu_.VisualGeneration() == UINT64_MAX ||
              frame < frame_ || now > UINT64_MAX - 250000000) {
            claimError = true;
            return false;
          }
          p.identity = Identity<typename S::Identity>(id_);
          p.nonce = ++nonce_;
          p.plan = s.run.plan;
          p.frame = frame;
          p.control = frame;
          p.geometry = geometry;
          p.deadline = now + 250000000;
          p.baseCommit = commit_;
          p.quanta = quanta_;
          p.kind = source ? 1 : 2;
          p.ordinal = source ? 0 : s.run.quantumOrdinal + 1;
          p.baseBank = 0;
          p.candidateBank = 2;
          p.arena = uint32_t(gpu_.VisualGeneration() & 1);
          p.arenaGeneration = gpu_.VisualGeneration() + 1;
          if constexpr (std::is_same_v<S, DMControl>) {
            p.rootBeginNs = now;
            p.clockGeneration = id_.pageSession;
            p.lifecycleGeneration = id_.permission;
          }
          if (uint32_t(s.gate.Freeze(command, p.nonce, now, frozen)) != 0) {
            claimError = true;
            return false;
          }
          frozen[24] = uint32_t(s.clock.grant.target);
          p.parameterSha = gpu_.Parent()->ParameterHash();
          p.layoutSha = gpu_.Parent()->PhysicalLayoutHash();
          p.shaderSha = gpu_.ShaderHash();
          p.inputSha = accept647::HashWords(frozen.data(), 128);
          permit.parent = p;
          permit.rootBeginNs = now;
          permit.visualGeneration = gpu_.VisualGeneration();
          permit.visualTicks = gpu_.VisualTicks();
          permit.compositeCode = gpu_.VisualConfiguration()[2];
          permit.visualConfigSha =
              accept647::HashWords(gpu_.VisualConfiguration().data(), 64);
          permit.pipelineSha = gpu_.ShaderHash();
          s.clock.jobLive = s.run.jobLive = true;
          s.clock.activeRootKind = p.kind;
          s.run.workspaceRetired = false;
          s.run.nonce = p.nonce;
          return true;
        },
        control_);
    if (!ok) {
      if (claimError)
        Fault("INPUT_OR_COUNTER_REJECTED");
      return claimError ? Result::STALE : Result::WOULD_BLOCK;
    }
    active_ = permit;
    root_ = true;
    frame_ = frame;
    geometry_ = geometry;
    publication_ = false;
    result_ = "ROOT_IN_PROGRESS";
  }
  const auto result = gpu_.Begin(permit, frozen, Check, this);
  if (result != Result::OK)
    Fault("ROOT_BEGIN_REJECTED");
  return result == Result::OK ? Result::IN_PROGRESS : result;
}
void Controller::Stop() { Fault("STOPPED"); }
bool Controller::Drain() {
  if (gpu_.Busy() && !gpu_.RetireRejected(active_))
    return false;
  if (invalidated_.load() && !gpu_.Witness().End(gpu_.Current()))
    return false;
  std::lock_guard<std::recursive_mutex> l(joint_);
  std::visit([](auto &s) { s.run.workspaceRetired = true; }, control_);
  return true;
}
bool Controller::Release(bool current, bool gone) {
  Stop();
  if (!gone && !Drain())
    return false;
  if (sceneFence_ && !gone && !PollSceneFence())
    return false;
  if (sceneFence_ && !gone)
    return false;
  if (!gpu_.Release(current, gone))
    return false;
  cold_.Invalidate();
  sceneFence_ = nullptr;
  configured_ = rawReady_ = session_ = root_ = lease_ = publication_ = false;
  return true;
}
bool Controller::PollSceneFence() {
  if (!sceneFence_)
    return true;
  auto f = sceneFence_;
  const auto result = glClientWaitSync(f, 0, 0);
  if (result == GL_TIMEOUT_EXPIRED)
    return true;
  if (result != GL_ALREADY_SIGNALED && result != GL_CONDITION_SATISFIED)
    return false;
  glDeleteSync(f);
  const auto error = glGetError();
  {
    std::lock_guard<std::recursive_mutex> l(joint_);
    if (sceneFence_ == f)
      sceneFence_ = nullptr;
  }
  return !error;
}
std::string Controller::TokenLocked() const {
  if (!publication_ || stale_ || !gpu_.HasValidatedPublication())
    return "";
  const auto *p = gpu_.Parent();
  std::ostringstream o;
  o << "{\"revision\":\"G11_JOINT_PUBLICATION_V2\",\"parent\":\""
    << (p->Kind() == ParentKind::DM ? "DM" : "R1") << "\",\"composite\":\""
    << gpu_.VisualConfiguration()[2] << "\",\"run\":\"" << id_.run
    << "\",\"context\":\"" << id_.context << "\",\"surface\":\"" << id_.surface
    << "\",\"pageSession\":\"" << id_.pageSession << "\",\"inputEpoch\":\""
    << id_.inputEpoch << "\",\"permission\":\"" << id_.permission
    << "\",\"config\":\"" << id_.config << "\",\"frame\":\"" << frame_
    << "\",\"physicsCommit\":\"" << commit_ << "\",\"acceptedQuanta\":\""
    << quanta_ << "\",\"visualGeneration\":\"" << gpu_.VisualGeneration()
    << "\",\"visualTicks\":\"" << gpu_.VisualTicks() << "\",\"parameterSHA\":\""
    << HexDigest(p->ParameterHash()) << "\",\"pipelineSHA\":\""
    << HexDigest(gpu_.ShaderHash()) << "\",\"visualConfigSHA\":\""
    << HexDigest(accept647::HashWords(gpu_.VisualConfiguration().data(), 64))
    << "\",\"visualSeedSHA\":\"" << HexDigest(gpu_.Bootstrap().SeedHash())
    << "\",\"recipeSHA\":\"" << kVisualRecipeSHA << "\"}";
  return o.str();
}
std::optional<Controller::SceneLease> Controller::AcquireScene() {
  if (!PollSceneFence())
    return {};
  std::lock_guard<std::recursive_mutex> l(joint_);
  if (!publication_ || root_ || gpu_.Busy() || lease_ || sceneFence_ ||
      cold_.Building() || revoked_.load() || leaseSerial_ == UINT64_MAX)
    return {};
  auto token = TokenLocked();
  if (token.empty())
    return {};
  lease_ = true;
  return SceneLease(gpu_, gpu_.VisualGeneration(), ++leaseSerial_, token);
}
bool Controller::CanPresent(const SceneLease &lease) const {
  std::lock_guard<std::recursive_mutex> l(joint_);
  if (!lease_ || lease.serial_ != leaseSerial_ ||
      lease.generation_ != gpu_.VisualGeneration() ||
      lease.token_ != TokenLocked() || revoked_.load())
    return false;
  return std::visit(
      [&](const auto &s) {
        using S = std::decay_t<decltype(s)>;
        using M = typename S::Mode;
        return s.clock.ready && s.clock.Fresh(Now()) &&
               s.clock.mode == M::RUNNABLE;
      },
      control_);
}
bool Controller::ReleaseScene(const SceneLease &lease, GLsync fence) {
  std::lock_guard<std::recursive_mutex> l(joint_);
  if (!lease_ || lease.serial_ != leaseSerial_ || sceneFence_ || !fence)
    return false;
  lease_ = false;
  sceneFence_ = fence;
  return true;
}
void Controller::Presented(const SceneLease &lease) {
  std::lock_guard<std::recursive_mutex> l(joint_);
  if (lease.serial_ == leaseSerial_ && !revoked_.load() &&
      lease.token_ == TokenLocked())
    presented_ = lease.token_;
}
void Controller::SceneFault(const SceneLease &lease) {
  std::lock_guard<std::recursive_mutex> l(joint_);
  if (lease.serial_ == leaseSerial_ &&
      !revoked_.load(std::memory_order_acquire))
    Fault("SCENE_DRAW_OR_PRESENT_FAILED");
}
std::string Controller::ScopeJson() const {
  std::lock_guard<std::recursive_mutex> l(joint_);
  return ScopeText(id_);
}
std::string Controller::Status() const {
  std::lock_guard<std::recursive_mutex> l(joint_);
  std::ostringstream o;
  o << "{\"state\":\"" << result_
    << "\",\"configured\":" << (configured_ ? "true" : "false")
    << ",\"bootstrapRawReady\":" << (rawReady_ ? "true" : "false")
    << ",\"bootstrapSealed\":" << (gpu_.Initialized() ? "true" : "false")
    << ",\"scope\":" << ScopeText(id_) << ",\"queued\":" << size_
    << ",\"physicsCommit\":\"" << commit_ << "\",\"quanta\":\"" << quanta_
    << "\",\"nonce\":\"" << nonce_ << "\",\"visualTicks\":\""
    << gpu_.VisualTicks() << "\",\"visualGeneration\":\""
    << gpu_.VisualGeneration() << "\",\"stale\":" << (stale_ ? "true" : "false")
    << ",\"sourceKind\":\"SYNTHETIC_TEST\",\"poseKind\":\"NATIVE_CONTROL\","
       "\"token\":";
  auto token = TokenLocked();
  o << (token.empty() ? "null" : token)
    << ",\"presentedMapping\":" << (presented_.empty() ? "null" : presented_);
  std::visit(
      [&](const auto &s) {
        o << ",\"quotaBalanced\":" << (s.clock.Balanced() ? "true" : "false")
          << ",\"generated\":\"" << s.clock.counters.generated
          << "\",\"accepted\":\"" << s.clock.counters.accepted
          << "\",\"clockFault\":" << uint32_t(s.clock.fault);
      },
      control_);
  o << ",\"tail\":[";
  for (size_t i = 0; i < 256; ++i) {
    if (i)
      o << ',';
    o << gpu_.Tail().words[i];
  }
  o << "]}";
  return o.str();
}
std::string Controller::ColdBegin(const std::string &token, bool full) {
  if (!PollSceneFence())
    return "{\"state\":\"SCENE_FENCE_FAILED\"}";
  std::string actual;
  std::array<uint32_t, 256> quota;
  {
    std::lock_guard<std::recursive_mutex> l(joint_);
    if (root_ || gpu_.Busy() || lease_ || sceneFence_ || revoked_.load() ||
        !publication_)
      return "{\"state\":\"WOULD_BLOCK\"}";
    actual = TokenLocked();
    quota = std::visit(
        [&](const auto &s) {
          return s.clock.SnapshotLocked(Now(), size_ && queue_[head_][40] == 1,
                                        s.run);
        },
        control_);
  }
  return cold_.Begin(token, actual, Now(), full, gpu_, quota);
}
std::string Controller::ColdState(uint64_t id) {
  if (invalidated_.load())
    cold_.Invalidate();
  return cold_.State(id, Now());
}
std::string Controller::ColdPage(uint64_t id, uint32_t segment,
                                 uint32_t offset) {
  if (invalidated_.load()) {
    cold_.Invalidate();
    return "{\"state\":\"STALE_SCOPE\"}";
  }
  return cold_.Read(id, segment, offset, Now());
}
std::string Controller::ColdEnd(uint64_t id) { return cold_.End(id, Now()); }
std::string Controller::ColdCapabilities() const {
  return cold_.Capabilities(gpu_);
}
bool Controller::ArmWitness(uint64_t nonce, uint32_t micro, uint32_t phase,
                            uint32_t role) {
  if (!gpu_.Initialized() || gpu_.Busy() || nonce_ == UINT64_MAX ||
      nonce != nonce_ + 1 || !gpu_.Current())
    return false;
  return gpu_.Witness().Arm(
      id_, *gpu_.Parent(), gpu_.VisualConfiguration(), gpu_.ShaderHash(),
      gpu_.Bootstrap().Receipt(),
      uint64_t(reinterpret_cast<uintptr_t>(eglGetCurrentContext())), nonce,
      micro, phase, role, *gpu_.Resources(), Now());
}
std::string Controller::WitnessState() const {
  return const_cast<VisualTransport &>(gpu_).Witness().State();
}
std::string Controller::WitnessPage(uint32_t offset) {
  if (invalidated_.load())
    gpu_.Witness().Invalidate();
  return gpu_.Witness().Page(offset, Now());
}
bool Controller::EndWitness() { return gpu_.Witness().End(gpu_.Current()); }
} // namespace grs::g11vis
