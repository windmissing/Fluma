#pragma once
#include "dm_boot_receipt.h"
#include "gpu_resident/g9dm/controller.h"
#include "gpu_resident/g9dm/source_trace.h"
#include <atomic>
#include <charconv>
#include <sys/syscall.h>

namespace dmsource {
uint64_t Time() {
  auto time = CheckedClockRead();
  if (!time.ok)
    throw std::runtime_error("NATIVE_MONOTONIC_READ_FAILED");
  return time.ns;
}

class Scope {
  Controller &controller_;
  SourceTrace *prior_;
public:
  Scope(Controller &controller, SourceTrace &trace)
      : controller_(controller), prior_(SetSourceTrace(&trace)) {}
  ~Scope() {
    SetSourceTrace(prior_);
    controller_.Stop();
    const auto deadline = now() + 1000000000;
    bool drained = controller_.Drain();
    while (!drained && now() < deadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      drained = controller_.Drain();
    }
    controller_.Release(drained);
  }
};

class Heartbeat {
  std::atomic<bool> stop_{false};
  std::atomic<uint32_t> result_{0};
  std::thread worker_;
public:
  Heartbeat(Controller &controller, ControlIdentity id)
      : worker_([this, &controller, id] {
          for (uint64_t seq = 2; !stop_.load(); ++seq) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            if (stop_.load())
              break;
            const auto time = CheckedClockRead();
            if (!time.ok) {
              result_ = uint32_t(ControlResult::CONTROL_CLOCK_REVERSED);
              break;
            }
            const auto r = controller.Pulse({id, seq, time.ns}, time.ns);
            result_ = uint32_t(r);
            if (r != ControlResult::OK)
              break;
          }
        }) {}
  ~Heartbeat() { Finish(); }
  uint32_t Finish() {
    stop_ = true;
    if (worker_.joinable())
      worker_.join();
    return result_.load();
  }
};

Input128 Command() {
  Input128 command{};
  command[40] = 1;
  Put(command.data(), 41, 1);
  constexpr std::array<double, 5> values{0.2, 64.0, 200.0, 0.0, 0.0};
  static_assert(sizeof(double) == sizeof(uint64_t));
  for (size_t i = 0; i < values.size(); ++i) {
    uint64_t bits = 0;
    std::memcpy(&bits, &values[i], sizeof(bits));
    Put(command.data(), 64 + 2 * i, bits);
  }
  command[74] = 2;
  command[75] = 1;
  return command;
}

uint64_t ColdId(const std::string &row, const char *key) {
  const std::string prefix = std::string("\"") + key + "\":\"";
  const auto start = row.find(prefix);
  if (start == std::string::npos)
    return 0;
  const char *begin = row.data() + start + prefix.size();
  uint64_t id = 0;
  const auto result = std::from_chars(begin, row.data() + row.size(), id);
  return result.ec == std::errc{} && result.ptr < row.data() + row.size() &&
         *result.ptr == '"' ? id : 0;
}

bool SaveCold(Controller &controller, std::ofstream &receipt, const std::filesystem::path &output) {
  const auto begin = controller.ColdBegin(0, Time());
  receipt << begin << '\n';
  const auto request = ColdId(begin, "requestId");
  const auto trace = ColdId(begin, "traceId");
  if (!request || !trace)
    return false;
  controller.BuildCold(Time());
  const auto state = controller.ColdState(request, Time());
  receipt << state << '\n';
  auto path = output / "source-cold.u32";
  const auto copied = controller.ExportSourceCold(trace, Time(),
      [](void *context, const uint32_t *words, size_t count) noexcept {
        try {
          const auto &path = *static_cast<std::filesystem::path *>(context);
          std::ofstream out(path, std::ios::binary);
          // Built only as the little-endian ARM64 OHOS driver.
          out.write(reinterpret_cast<const char *>(words), count * sizeof(*words));
          out.close();
          return out.good();
        } catch (...) {
          return false;
        }
      }, &path);
  receipt << controller.ColdEnd(trace, Time()) << '\n';
  return copied;
}

const char *Flag(bool value) { return value ? "true" : "false"; }

void RootReceipt(std::ofstream &out, const SourceTrace &s, Result result, uint32_t heartbeat) {
  const auto &p = s.permit;
  Digest seed{};
  std::copy_n(s.config.begin() + 48, seed.size(), seed.begin());
  out << "{\"stage\":\"SOURCE_ROOT\",\"ticket\":\"1\",\"result\":" << uint32_t(result)
      << ",\"pid\":" << getpid() << ",\"tid\":" << syscall(SYS_gettid)
      << ",\"scope\":\"" << p.identity.run << "\",\"nonce\":\"" << p.nonce << "\",\"plan\":\"" << p.plan
      << "\",\"owner\":\"" << p.identity.owner << "\",\"context\":\"" << p.identity.context
      << "\",\"inputEpoch\":\"" << p.identity.inputEpoch << "\",\"session\":\"" << p.identity.pageSession
      << "\",\"permission\":\"" << p.identity.permission << "\",\"window\":\"" << p.identity.window
      << "\",\"config\":\"" << p.identity.config << "\",\"surface\":\"" << p.identity.surface
      << "\",\"geometry\":\"" << p.geometry << "\",\"frame\":\"" << p.frame
      << "\",\"arenaGeneration\":\"" << p.arenaGeneration << "\",\"baseBank\":" << p.baseBank
      << ",\"candidateBank\":" << p.candidateBank << ",\"arena\":" << p.arena
      << ",\"kind\":" << p.kind << ",\"ordinal\":" << p.ordinal
      << ",\"rootBeginNs\":\"" << p.rootBeginNs << "\",\"deadlineNs\":\"" << p.deadline
      << "\",\"fenceNs\":\"" << s.fenceNs << "\",\"commitNs\":\"" << s.commitNs
      << "\",\"baseCommit\":\"" << p.baseCommit << "\",\"commit\":\"" << s.commit
      << "\",\"baseQuanta\":\"" << p.quanta << "\",\"quanta\":\"" << s.quanta
      << "\",\"generation\":\"" << s.generation << "\",\"expectedSHA\":" << dmboot::Json(Hex(s.expected))
      << ",\"inputSHA\":" << dmboot::Json(Hex(p.inputSha))
      << ",\"sourceSHA\":" << dmboot::Json(Hex(p.shaderSha))
      << ",\"parameterSHA\":" << dmboot::Json(Hex(p.parameterSha))
      << ",\"layoutSHA\":" << dmboot::Json(Hex(p.layoutSha))
      << ",\"seedSHA\":" << dmboot::Json(Hex(seed));
  out << ",\"actualProgram\":" << s.uploadProgram << ",\"workObject\":" << s.workObject
      << ",\"eglContext\":\"" << s.eglContext << "\",\"eglDisplay\":\"" << s.eglDisplay
      << "\",\"eglSurface\":\"" << s.eglSurface << '"'
      << ",\"uploaded\":" << Flag(s.uploaded)
      << ",\"workByteOffset\":" << s.workOffset << ",\"workReadBytes\":" << (s.mapped ? 1024 : 0)
      << ",\"dispatches\":" << s.dispatches << ",\"barriers\":" << s.barriers
      << ",\"copyBarriers\":" << s.copyBarriers << ",\"fenceCreated\":" << Flag(s.fenceCreated)
      << ",\"fencePolls\":" << s.fencePolls << ",\"fenceWaitRaw\":";
  if (s.fencePolls)
    out << s.fenceWait;
  else
    out << "null";
  out << ",\"mapReturned\":" << Flag(s.mapReturned) << ",\"unmapRaw\":";
  if (s.mapReturned)
    out << s.unmapRaw;
  else
    out << "null";
  out << ",\"glSample\":" << s.gl << ",\"eglSample\":" << s.egl
      << ",\"firstGl\":" << s.firstGl << ",\"firstEgl\":" << s.firstEgl
      << ",\"typedAccepted\":" << Flag(s.typed) << ",\"preflight\":" << Flag(s.preflight)
      << ",\"proposedPlanRetired\":" << Flag(s.planRetired) << ",\"committed\":" << Flag(s.committed)
      << ",\"baseObject\":" << s.baseObject << ",\"candidateObject\":" << s.candidateObject
      << ",\"committedObject\":" << s.committedObject
      << ",\"queueBefore\":" << s.queueBefore << ",\"queueAfter\":" << s.queueAfter
      << ",\"pendingAtClaim\":" << s.pendingAtClaim << ",\"activeRemaining\":" << s.activeRemaining
      << ",\"pendingAfter\":" << s.pendingAfter << ",\"controlFault\":" << s.fault
      << ",\"heartbeatResult\":" << heartbeat << ",\"outcome\":" << s.completion[6]
      << ",\"admittedQ20\":" << s.completion[62] << ",\"passes\":[";
  for (uint32_t i = 0; i < s.dispatches; ++i) {
    if (i) out << ',';
    const auto &entry = s.passes[i];
    out << '[' << i << ',' << entry.op << ',' << entry.role << ',' << entry.j << ',' << entry.micro;
    for (auto object : entry.objects)
      out << ',' << object;
    out << ']';
  }
  out << "]}\n";
  out.flush();
}

int Run(std::ofstream &out, const std::filesystem::path &output, const Config &config,
        const SeedFileIdentity &seed, const char *cache) {
  if (config.k != 192 || config.fullTrialTrace)
    throw std::runtime_error("SOURCE_REQUIRES_K192_ROOT_MODE");
  const auto run = Time();
  const ControlIdentity id{run, 1, run, 1, 1, 1, 1, 1, 1};
  dmboot::Receipt receipt(out, output);
  receipt.Identity(id.run, id.owner, id.context);
  out << "{\"stage\":\"CONFIGURE_ENTER\",\"variant\":\"BOOT_SOURCE\",\"roleSetSHA\":"
      << dmboot::Json(SourceRoleSetSHA()) << ",\"sourceSHA\":" << dmboot::Json(PipelineSourceHash())
      << ",\"cacheConfigured\":" << Flag(cache && ConfigureBootCache(cache)) << "}\n";
  out.flush();
  if (!out)
    throw std::runtime_error("RECEIPT_IO_FAILED");
  Controller controller;
  const auto capture = receipt.Capture();
  SourceTrace trace;
  trace.bootCapture = &capture;
  Scope scope(controller, trace);
  auto result = controller.ConfigureResearch(config, id, seed);
  out << "{\"stage\":\"CONFIGURE_EXIT\",\"result\":" << uint32_t(result)
      << ",\"bootAudited\":" << Flag(trace.bootAudited)
      << ",\"bootProgram\":" << trace.bootProgram << ",\"sourceProgram\":" << trace.program
      << ",\"firstGl\":" << trace.firstGl << ",\"firstEgl\":" << trace.firstEgl << "}\n";
  out.flush();
  if (result != Result::OK || !receipt.Good() || !receipt.TailWritten())
    return 3;
  const auto body = Command();
  write(output / "source-command.u32", body.data(), body.size());
  out << "{\"stage\":\"SOURCE_ADMISSION_ENTER\",\"ticket\":\"1\"}\n";
  out.flush();
  const auto baseline = Time();
  const auto pulse = controller.Pulse({id, 1, baseline}, baseline);
  result = pulse == ControlResult::OK ? controller.OfferModelCommand(body, Time(), 1) : Result::STALE;
  const auto offered = result;
  if (result == Result::OK)
    result = controller.BeginRoot(Time(), 1, 1);
  const auto began = result;
  uint32_t heartbeat = 0;
  if (result == Result::OK) {
    Heartbeat pulses(controller, id);
    do {
      result = controller.PumpBounded(Time());
      if (result == Result::IN_PROGRESS)
        std::this_thread::yield();
    } while (result == Result::IN_PROGRESS);
    heartbeat = pulses.Finish();
  }
  out << "{\"stage\":\"SOURCE_ADMISSION_EXIT\",\"pulse\":" << uint32_t(pulse)
      << ",\"offered\":" << uint32_t(offered) << ",\"began\":" << uint32_t(began) << "}\n";
  RootReceipt(out, trace, result, heartbeat);
  if (trace.uploaded) {
    write(output / "source-input.u32", trace.input.data(), trace.input.size());
    write(output / "source-config.u32", trace.config.data(), trace.config.size());
  }
  if (trace.mapped)
    write(output / "source-completion.u32", trace.completion.data(), trace.completion.size());
  if (result != Result::OK || !trace.committed)
    return 6;
  const bool cold = SaveCold(controller, out, output);
  const bool observed = trace.typed && trace.planRetired && trace.completion[6] == 1 &&
      trace.completion[62] == 209715 && trace.commit == 1 && trace.quanta == 0 &&
      trace.dispatches == 12 && trace.barriers == 12 && trace.copyBarriers == 3 &&
      trace.pendingAtClaim == 0 && trace.activeRemaining == 0 &&
      trace.queueBefore == 1 && trace.queueAfter == 0 && trace.committedObject == trace.candidateObject;
  const auto cachePrepare = cache && observed ? controller.PrepareSourceCache() : GL_INVALID_OPERATION;
  out << "{\"stage\":\"SOURCE_EXIT\",\"positiveObserved\":" << Flag(observed)
      << ",\"coldSaved\":" << Flag(cold) << ",\"coldGl\":" << trace.coldGl
      << ",\"coldEgl\":" << trace.coldEgl << ",\"cachePrepareGl\":";
  if (cache && observed)
    out << cachePrepare;
  else
    out << "null";
  out << ",\"quantumAuthorized\":false}\n";
  out.flush();
  return observed && cold && out ? 0 : 6;
}
} // namespace dmsource
