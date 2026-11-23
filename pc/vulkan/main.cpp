#include "boot.h"
#include "m1_d1_plan.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cwchar>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <windowsx.h>

namespace {
constexpr UINT kOwnerFinished = WM_APP + 1;
constexpr UINT kFirstPresent = WM_APP + 2;
constexpr UINT_PTR kSourceProbeTimer = 2;
struct Window {
  HWND hwnd = nullptr;
  std::mutex mutex, output;
  std::condition_variable wake;
  uint64_t generation = 0, eventSeq = 0;
  int width = 0, height = 0;
  UINT dpi = 96;
  bool closing = false;
  bool once = false, bootOnly = false, steady = false;
  std::string steadyRole;
  bool disableOptimization = false;
  std::wstring m1SourceModule;
  std::wstring m1QuantumCandidate, m1QuantumCommit, m1ColdFile;
  bool m1Present = false;
  bool dualSource = false;
  uint32_t material=0;
  std::wstring t849WriteFile;
  bool t854PrivateWrites = false;
  std::wstring t833CandidateFile;
  std::string t833RevokeAt;
  std::string t833Case;
  std::wstring t833ConfigFile,t833SeedFile;
  uint32_t t833ForcedJ=0;
  bool t833F05Rest=false;
  std::array<uint64_t,32> g13PulseHistory{};
  size_t g13PulseWrite=0;
  bool t833Allowed=true;
  std::atomic<uint64_t> sourcePulse{0};
  bool pointerReady = false;
  grs::g13vk::PointerSample held;
  grs::g13vk::SourceScope scope;
  grs::g13vk::M1Ingress ingress;
  grs::g13vk::M1HotCapture hotCapture;
  bool captureHot=false, captureCold=false, sourceTrace=false;
  bool rootCapture=false;
  std::wstring d1Mode,d1ReplayPrefix;
  std::condition_variable d1Ready;
  std::atomic<unsigned> m1Shown{0};
  uint64_t captureRun=0, captureReserved=0, droppedTrace=0;
  size_t coldOffset=0;
  uint64_t sampleTicks=0, offerSeq=0;
  int result = 1;

  void Emit(const std::string &json) {
    std::lock_guard lock(output);
    std::cout << json << std::endl;
  }
  bool D1CaptureEnabled() const { return d1Mode==L"ON" || d1Mode==L"REPLAY_ON"; }
  void DrainSourceTrace() {
    if (!sourceTrace) return;
    std::array<grs::g13vk::M1Ingress::Trace,64> batch;
    uint64_t dropped=0;
    size_t count;
    do {
      count=ingress.ReadTrace(batch.data(),batch.size(),dropped);
      for (size_t i=0;i<count;++i) {
        const auto &record=batch[i];
        const auto &o=record.offer;
        std::ostringstream row;
        row << "{\"stage\":\"M1_OFFER_" << (record.terminal ? "TERMINAL" : "RETURN")
            << "\",\"serial\":\"" << record.serial << "\",\"run\":\"" << o.scope.run
            << "\",\"geometry\":\"" << o.scope.geometry << "\",\"offer\":\"" << o.seq
            << "\",\"actionId\":" << o.action
            << "\",\"sample\":\"" << o.sample.sequence << "\",\"hold\":\"" << o.sample.hold
            << "\",\"pulseTicks\":\"" << o.pulseTicks << "\",\"sampleTicks\":\"" << o.sampleTicks
            << "\",\"frequency\":\"" << o.frequency << "\",\"pulseNs\":\"" << o.pulseNs
            << "\",\"sampleNs\":\"" << o.sample.timeNs << "\",\"pixelX\":" << o.sample.clientX
            << ",\"pixelY\":" << o.sample.clientY << ",\"material\":" << o.sample.material
            << ",\"result\":" << int(record.result.code) << ",\"root\":\"" << record.result.root << "\"}";
        Emit(row.str());
      }
    } while (count==batch.size());
    if (droppedTrace!=dropped) {
      droppedTrace=dropped;
      Emit("{\"stage\":\"M1_OFFER_TRACE_INCOMPLETE\",\"dropped\":"+std::to_string(dropped)+"}");
    }
  }
  void RunD1() {
    using namespace grs::g13vk;
    SourceScope readyScope;
    const auto begin=std::chrono::steady_clock::now();
    uint64_t ready=0;
    {
      std::unique_lock lock(mutex);
      d1Ready.wait(lock,[&] { return closing || pointerReady; });
      if (closing) return;
      readyScope=scope;
      ready=grs::pc::TimeNs();
    }
    const auto anchor=std::chrono::steady_clock::now();
    const bool enabled=D1CaptureEnabled();
    const std::string mode(d1Mode.begin(),d1Mode.end());
    uint64_t previousNs=0,reservation=0;
    D1Action pending{};
    uint64_t createdNs=0,createdTicks=0;
    Emit("{\"stage\":\"M1_D1_READY\",\"readyNs\":\""+std::to_string(ready)+
         "\",\"run\":\""+std::to_string(readyScope.run)+"\",\"mode\":\""+mode+
         "\",\"waitMs\":"+std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(anchor-begin).count())+"}");
    for (const auto &action:M1D1Plan()) {
      std::this_thread::sleep_until(anchor+std::chrono::nanoseconds(action.ns));
      const auto ticks=grs::pc::QpcTicks(),ns=grs::pc::TimeNs();
      SourceOffer offer;
      bool current;
      {
        std::lock_guard lock(mutex);
        current=!closing && generation==readyScope.geometry && pointerReady;
        if (current && action.kind==D1Kind::Source) {
          pending=action;
          createdNs=ns;
          createdTicks=ticks;
        }
        if (current && action.kind==D1Kind::Pulse) {
          sourcePulse.store(ns,std::memory_order_relaxed);
          if (pending.id) {
            const auto serial=++eventSeq;
            held={serial,serial,createdNs,64.5,pending.y,int(64.5*width/128.),
                  int((1.-pending.y/256.)*height),0};
            scope.permission=held.hold;
            ingress.Authorize(scope,held.hold);
            offer={scope,++offerSeq,ns,ticks,createdTicks,grs::pc::QpcFrequency(),held,2,pending.id,
                   previousNs && ns>previousNs ? ns-previousNs : 0};
            pending={};
          }
          previousNs=ns;
        }
      }
      const char *state="REVOKED";
      if (current) {
        switch (action.kind) {
        case D1Kind::Pulse:
          state="DELIVERED";
          if (offer.seq) ingress.OfferSource(offer);
          break;
        case D1Kind::Source: state="AWAIT_PULSE"; break;
        case D1Kind::Reserve:
          reservation=action.id;
          state=!enabled ? "OFF_NOOP" :
            hotCapture.Reserve(readyScope,reservation,ns)==M1HotCapture::Code::Reserved ? "RESERVED" : "NOT_BUSY";
          break;
        case D1Kind::End:
          state=enabled ? "NO_PAGE" : "OFF_NOOP";
          if (enabled) if (const auto page=hotCapture.Read(readyScope,reservation,ns)) {
            std::ostringstream record;
            record << "{\"stage\":\"M1_D1_HOT_PAGE\",\"request\":" << reservation << ",\"root\":\"" << page->busyRoot
                   << "\",\"safePoint\":\"" << page->safePoint << "\",\"completion\":[";
            for (size_t i=0;i<page->completion.size();++i) {
              if (i) record << ',';
              record << page->completion[i];
            }
            Emit(record.str()+"]}");
            hotCapture.End(readyScope,reservation);
            state="ENDED";
          }
          break;
        }
      }
      Emit("{\"stage\":\"M1_D1_ACTION\",\"id\":"+std::to_string(action.id)+
           ",\"kind\":"+std::to_string(int(action.kind))+",\"scriptNs\":\""+std::to_string(action.ns)+
           "\",\"actualNs\":\""+std::to_string(ns)+"\",\"state\":\""+state+"\"}");
    }
    std::lock_guard lock(mutex);
    if (!closing) PostMessageW(hwnd,WM_CLOSE,0,0);
  }

  void Size() {
    RECT r{};
    if (!GetClientRect(hwnd, &r)) return;
    std::lock_guard lock(mutex);
    width = r.right - r.left;
    height = r.bottom - r.top;
    if (IsIconic(hwnd)) width = height = 0;
    dpi = GetDpiForWindow(hwnd);
    ++generation;
    pointerReady = false;
    held = {};
    ingress.Authorize({},0);
    hotCapture.Authorize({});
    wake.notify_one();
  }

  void Input(const char *kind, int x, int y) {
    std::unique_lock lock(mutex);
    if (closing) return;
    const auto sequence = ++eventSeq;
    const auto ticks = grs::pc::QpcTicks();
    const bool inside = x >= 0 && y >= 0 && x < width && y < height;
    const std::string action(kind);
    if (!d1Mode.empty() && (action=="DOWN" || action=="MOVE" || action=="UP")) return;
    if (action == "DOWN") {
      held = {};
      if (pointerReady && inside) {
        held.hold = sequence;
        held.material = material;
      }
    } else if (action != "MOVE" || !inside) {
      held = {};
    }
    if (held.hold) {
      held.sequence = sequence;
      held.timeNs = grs::pc::TimeNs();
      held.x = (double(x)+0.5)*128/width;
      held.y = (1-(double(y)+0.5)/height)*256;
      held.clientX = x;
      held.clientY = y;
      sampleTicks = ticks;
    }
    scope.permission=held.hold;
    ingress.Authorize(scope,held.hold);
    const auto geometry=generation, hold=held.hold;
    const auto eventDpi=dpi;
    lock.unlock();
    Emit("{\"stage\":\"WIN32_POINTER\",\"kind\":\"" + action +
         "\",\"geometry\":\"" + std::to_string(geometry) +
         "\",\"seq\":\"" + std::to_string(sequence) +
         "\",\"qpc\":\"" + std::to_string(ticks) +
         "\",\"x\":" + std::to_string(x) + ",\"y\":" + std::to_string(y) +
         ",\"dpi\":" + std::to_string(eventDpi) + ",\"hold\":\"" + std::to_string(hold) + "\"}");
  }

  void SelectMaterial(uint32_t next) {
    uint64_t geometry;
    {
      std::lock_guard lock(mutex);
      if (closing || material==next) return;
      material=next;
      held={};
      scope.permission=0;
      ingress.Authorize(scope,0);
      geometry=generation;
    }
    Emit("{\"stage\":\"M1_SOURCE_MODE\",\"geometry\":\""+std::to_string(geometry)+
         "\",\"material\":"+std::to_string(next)+"}");
  }

  void Pulse() {
    const auto ticks=grs::pc::QpcTicks(), ns=grs::pc::TimeNs();
    sourcePulse.store(ns,std::memory_order_relaxed);
    if (!t833CandidateFile.empty()) {
      std::lock_guard lock(mutex);
      g13PulseHistory[g13PulseWrite++%g13PulseHistory.size()]=ns;
      return;
    }
    grs::g13vk::SourceOffer offer;
    {
      std::lock_guard lock(mutex);
      if (closing || !pointerReady) return;
      offer.scope=scope;
      if (held.hold)
        offer={scope, ++offerSeq, ns, ticks, sampleTicks, grs::pc::QpcFrequency(), held, 1};
    }
    if (offer.seq) ingress.OfferSource(offer);
    auto captureScope=offer.scope;
    captureScope.permission=0;
    const auto request=captureScope.run;
    if (!(captureHot || captureCold) || !request || captureRun==request) return;
    if (rootCapture && !m1Shown.load(std::memory_order_relaxed)) return;
    using Code=grs::g13vk::M1HotCapture::Code;
    const auto state=hotCapture.Reserve(captureScope,request,ns);
    if (state==Code::Reserved && captureReserved!=request) {
      captureReserved=request;
      coldOffset=0;
      Emit("{\"stage\":\"M1_HOT_CAPTURE_RESERVED\",\"request\":\""+std::to_string(request)+
           "\",\"requestNs\":\""+std::to_string(ns)+"\"}");
    } else if (state==Code::Cancelled || state==Code::TimedOut || state==Code::Failed) {
      captureRun=request;
      Emit("{\"stage\":\"M1_HOT_CAPTURE_TERMINAL\",\"request\":\""+std::to_string(request)+
           "\",\"state\":\""+(state==Code::Cancelled ? "CANCELLED" :
                                   state==Code::TimedOut ? "TIMED_OUT" : "FAILED")+"\"}");
    }
    if (captureCold && (state==Code::ColdFrozen || state==Code::ColdRead)) {
      std::array<uint8_t,grs::g13vk::M1HotCapture::kPageLimit> bytes;
      if (const auto page=hotCapture.ReadCold(captureScope,request,coldOffset,bytes.data(),bytes.size(),ns)) {
        Emit("{\"stage\":\"M1_COLD_PAGE\",\"request\":\""+std::to_string(request)+
             "\",\"root\":\""+std::to_string(page->lineage.busyRoot)+"\",\"offset\":"+
             std::to_string(page->offset)+",\"count\":"+std::to_string(page->count)+",\"total\":"+
             std::to_string(page->total)+",\"sha\":\""+page->sha+"\"}");
        coldOffset+=page->count;
        if (coldOffset==page->total) {
          hotCapture.End(captureScope,request);
          captureRun=request;
        }
      }
    }
    if (!captureCold && state==Code::Frozen) {
      if (const auto page=hotCapture.Read(captureScope,request,ns)) {
        std::string row="{\"stage\":\"M1_HOT_CAPTURE\",\"request\":\""+std::to_string(request)+
          "\",\"root\":\""+std::to_string(page->busyRoot)+"\",\"requestedNs\":\""+
          std::to_string(page->requestedNs)+"\",\"frozenNs\":\""+std::to_string(page->frozenNs)+
          "\",\"safePoint\":\""+std::to_string(page->safePoint)+"\",\"blocker\":"+
          std::to_string(int(page->blocker))+",\"completion\":[";
        for (size_t i=0;i<page->completion.size();++i) {
          if (i) row+=',';
          row+=std::to_string(page->completion[i]);
        }
        hotCapture.End(captureScope,request);
        Emit(row+"]}");
        captureRun=request;
      }
    }
  }

  void Close() {
    std::lock_guard lock(mutex);
    closing = true;
    held = {};
    ingress.Authorize({},0);
    hotCapture.Authorize({});
    wake.notify_one();
    d1Ready.notify_all();
  }

  void Owner() {
    uint64_t previous = 0;
    std::unique_lock lock(mutex);
    for (;;) {
      wake.wait(lock, [&] { return closing || (generation != previous && width > 0 && height > 0); });
      if (closing) break;
      previous = generation;
      m1Shown.store(0,std::memory_order_relaxed);
      const auto snapshotDpi = dpi;
      const auto snapshotWidth = width, snapshotHeight = height;
      lock.unlock();
      auto current = [&] {
        std::lock_guard guard(mutex);
        return !closing && generation == previous && t833Allowed;
      };
      MONITORINFOEXW display{};
      display.cbSize = sizeof(display);
      DEVMODEW mode{};
      mode.dmSize = sizeof(mode);
      const auto monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
      const bool haveMode = GetMonitorInfoW(monitor, &display) &&
                            EnumDisplaySettingsW(display.szDevice, ENUM_CURRENT_SETTINGS, &mode);
      Emit("{\"stage\":\"WINDOW_AVAILABLE\",\"geometry\":\"" + std::to_string(previous) +
           "\",\"hwnd\":\"" + std::to_string(reinterpret_cast<uintptr_t>(hwnd)) +
           "\",\"width\":" + std::to_string(snapshotWidth) + ",\"height\":" +
           std::to_string(snapshotHeight) + ",\"dpi\":" + std::to_string(snapshotDpi) +
           ",\"modeHz\":" + (haveMode ? std::to_string(mode.dmDisplayFrequency) : "null") + "}");
      const auto terminal = grs::g13vk::RunBoot({previous, 1, previous, hwnd, !bootOnly || m1Present, current, steady,
                                               steadyRole, disableOptimization, m1SourceModule,
                                               [&] { return sourcePulse.load(std::memory_order_relaxed); },
                                               m1QuantumCandidate, m1QuantumCommit, m1ColdFile,
                                               [&](const std::string &readyReceipt) {
                                                 std::lock_guard sink(output);
                                                 {
                                                   std::lock_guard guard(mutex);
                                                   if (closing || generation != previous) return false;
                                                   pointerReady = true;
                                                   d1Ready.notify_one();
                                                 }
                                                 std::cout << readyReceipt << std::endl;
                                                 return true;
                                               },
                                               [&] {
                                                 std::lock_guard guard(mutex);
                                                 return !closing && generation == previous ? held :
                                                     grs::g13vk::PointerSample{};
                                               },
                                               [&](uint64_t hold, const std::function<void()> &accept) {
                                                 std::lock_guard guard(mutex);
                                                 if (closing || generation != previous || !t833Allowed ||
                                                     (hold && held.hold != hold))
                                                   return false;
                                                 accept();
                                                 return true;
                                               }, once, t849WriteFile, t854PrivateWrites,
                                               [&](grs::g13vk::SourceScope next) {
                                                 std::lock_guard guard(mutex);
                                                 if (closing || generation != previous) return;
                                                 scope=next;
                                                 ingress.Authorize(scope,0);
                                                 hotCapture.Authorize(next);
                                               }, &ingress, (captureHot || captureCold || D1CaptureEnabled()) ? &hotCapture : nullptr,
                                               dualSource,captureCold,t833CandidateFile,t833RevokeAt,[&] {
                                                 std::lock_guard guard(mutex);
                                                 t833Allowed=false;
                                               },t833Case,t833ConfigFile,t833SeedFile,t833ForcedJ,rootCapture,d1ReplayPrefix,
                                               !d1Mode.empty(),[&](uint64_t after) {
                                                 std::lock_guard guard(mutex);
                                                 std::vector<uint64_t> samples;
                                                 const auto count=std::min(g13PulseWrite,g13PulseHistory.size());
                                                 for (size_t i=g13PulseWrite-count;i<g13PulseWrite;++i) {
                                                   const auto ns=g13PulseHistory[i%g13PulseHistory.size()];
                                                   if (ns>after) samples.push_back(ns);
                                                 }
                                                 return samples;
                                               },t833F05Rest},
          [&](const std::string &report) {
            Emit(report);
            if (report.find("\"state\":\"M1_QUANTUM_PRESENT_QUEUED\"")!=std::string::npos)
              m1Shown.fetch_add(1,std::memory_order_relaxed);
            if (report.find("\"state\":\"BOOT_DIAGNOSTIC_PRESENT_QUEUED\"") != std::string::npos ||
                report.find("\"state\":\"M1_DRY_SOURCE_PRESENT_QUEUED\"") != std::string::npos ||
                report.find("\"state\":\"M1_QUANTUM_PRESENT_QUEUED\"") != std::string::npos) {
              result = 0;
              PostMessageW(hwnd, kFirstPresent, 0, 0);
            }
            return current();
          });
      Emit(terminal);
      if (!m1QuantumCandidate.empty()) {
        std::lock_guard guard(mutex);
        if (generation == previous) {
          pointerReady = false;
          held = {};
          ingress.Authorize({},0);
          hotCapture.Authorize({});
        }
      }
      const char *qualified = !t833CandidateFile.empty() ?
                              (t833RevokeAt.empty() ? (t833Case=="F02" ?
                               "\"state\":\"T833_PRIVATE_PROJECT_CAPTURED\"" :
                               "\"state\":\"T833_CANDIDATE_CAPTURED\"") :
                               "\"state\":\"T833_CANDIDATE_REVOKED\"") :
                              t854PrivateWrites ? "\"state\":\"T854_PRIVATE_WRITES_CAPTURED\"" :
                              !t849WriteFile.empty() ? "\"state\":\"T849_PRIVATE_WRITES_CAPTURED\"" :
                              m1Present ? "\"state\":\"M1_DRY_SOURCE_PRESENT_RETIRED\"" :
                              !m1SourceModule.empty() ? "\"state\":\"M1_DRY_SOURCE_ACCEPTED\"" :
                              !steadyRole.empty() ? "\"state\":\"STEADY_ROLE_QUALIFIED\"" :
                              steady ? "\"state\":\"STEADY_ROLES_QUALIFIED\"" :
                                      "\"state\":\"BOOT_NUMERIC_QUALIFIED\"";
      if (bootOnly && terminal.find(qualified) != std::string::npos) result = 0;
      const bool failed = terminal.find("\"state\":\"FAILED\"") != std::string::npos ||
                          terminal.find("\"state\":\"WSI_FAILED\"") != std::string::npos;
      if (bootOnly || (once && failed)) break;
      lock.lock();
    }
    PostMessageW(hwnd, kOwnerFinished, 0, 0);
  }
};

LRESULT CALLBACK Dispatch(HWND hwnd, UINT message, WPARAM w, LPARAM l) {
  auto *window = reinterpret_cast<Window *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  if (message == WM_NCCREATE) {
    window = static_cast<Window *>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
    window->hwnd = hwnd;
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
  }
  if (!window) return DefWindowProcW(hwnd, message, w, l);
  switch (message) {
  case WM_SIZE: window->Size(); return 0;
  case WM_DPICHANGED: {
    const auto &r = *reinterpret_cast<RECT *>(l);
    SetWindowPos(hwnd, nullptr, r.left, r.top, r.right-r.left, r.bottom-r.top, SWP_NOACTIVATE | SWP_NOZORDER);
    window->Size();
    return 0;
  }
  case WM_LBUTTONDOWN: SetCapture(hwnd); window->Input("DOWN", GET_X_LPARAM(l), GET_Y_LPARAM(l)); return 0;
  case WM_MOUSEMOVE:
    if (w & MK_LBUTTON) window->Input("MOVE", GET_X_LPARAM(l), GET_Y_LPARAM(l));
    return 0;
  case WM_LBUTTONUP: window->Input("UP", GET_X_LPARAM(l), GET_Y_LPARAM(l)); ReleaseCapture(); return 0;
  case WM_KEYDOWN:
    if (window->dualSource && (w=='1' || w=='2')) {
      window->SelectMaterial(w=='2' ? 1u : 0u);
      return 0;
    }
    break;
  case WM_CANCELMODE: window->Input("CANCEL", 0, 0); if (GetCapture() == hwnd) ReleaseCapture(); return 0;
  case WM_CAPTURECHANGED: window->Input("CAPTURE_LOST", 0, 0); return 0;
  case WM_KILLFOCUS: window->Input("FOCUS_LOST", 0, 0); if (GetCapture() == hwnd) ReleaseCapture(); return 0;
  case WM_CLOSE: window->Close(); return 0;
  case kFirstPresent:
    if (window->once) SetTimer(hwnd, 1, 700, nullptr);
    return 0;
  case WM_TIMER:
    if (w == kSourceProbeTimer) {
      if (window->d1Mode.empty()) window->Pulse();
      window->DrainSourceTrace();
      return 0;
    }
    KillTimer(hwnd, 1);
    window->Close();
    return 0;
  case kOwnerFinished: window->DrainSourceTrace(); DestroyWindow(hwnd); return 0;
  case WM_DESTROY: PostQuitMessage(0); return 0;
  }
  return DefWindowProcW(hwnd, message, w, l);
}
}

int wmain(int argc, wchar_t **argv) {
  Window window;
  for (int i = 1; i < argc; ++i) {
    if (!std::wcscmp(argv[i], L"--once")) window.once = true;
    else if (!std::wcscmp(argv[i], L"--boot-only")) window.bootOnly = true;
    else if (!std::wcscmp(argv[i], L"--qualify-steady")) {
      window.bootOnly = true;
      window.steady = true;
    }
    else if (!std::wcscmp(argv[i], L"--qualify-steady-role") && i+1 < argc) {
      const std::wstring name(argv[++i]);
      window.steadyRole.assign(name.begin(), name.end());
      window.bootOnly = window.steady = true;
    }
    else if (!std::wcscmp(argv[i], L"--disable-optimization")) {
      window.disableOptimization = true;
    }
    else if (!std::wcscmp(argv[i], L"--qualify-t849-writes") && i+1 < argc) {
      window.t849WriteFile = argv[++i];
      window.bootOnly = window.steady = true;
    }
    else if (!std::wcscmp(argv[i], L"--qualify-t854-writes") && i+1 < argc) {
      window.t849WriteFile = argv[++i];
      window.t854PrivateWrites = true;
      window.bootOnly = window.steady = true;
    }
    else if (!std::wcscmp(argv[i],L"--g13-candidate") && i+1 < argc) {
      window.t833CandidateFile=argv[++i];
      window.bootOnly=true;
    }
    else if (!std::wcscmp(argv[i],L"--g13-revoke-before")) window.t833RevokeAt="before";
    else if (!std::wcscmp(argv[i],L"--g13-revoke-after")) window.t833RevokeAt="after";
    else if (!std::wcscmp(argv[i],L"--g13-fixture") && i+3 < argc) {
      const std::wstring name=argv[++i];
      if (name!=L"F02" && name!=L"F03" && name!=L"F05") return 2;
      window.t833Case.assign(name.begin(),name.end());
      window.t833ConfigFile=argv[++i];
      window.t833SeedFile=argv[++i];
    }
    else if (!std::wcscmp(argv[i],L"--g13-trial") && i+1 < argc) {
      const std::wstring trial=argv[++i];
      if (trial!=L"1" && trial!=L"2" && trial!=L"4") return 2;
      window.t833ForcedJ=uint32_t(trial[0]-L'0');
    }
    else if (!std::wcscmp(argv[i],L"--g13-f05-rest")) window.t833F05Rest=true;
    else if (!std::wcscmp(argv[i], L"--m1-window") && i+3 < argc) {
      window.m1SourceModule = argv[++i];
      window.m1QuantumCandidate = argv[++i];
      window.m1QuantumCommit = argv[++i];
      window.m1Present = true;
    }
    else if ((!std::wcscmp(argv[i],L"--m1-cold") || !std::wcscmp(argv[i],L"--m1-cold-on-busy")) && i+1 < argc) {
      window.captureCold=!std::wcscmp(argv[i],L"--m1-cold-on-busy");
      window.m1ColdFile = argv[++i];
    }
    else if (!std::wcscmp(argv[i],L"--m1-root-on-busy") && i+1 < argc) {
      window.rootCapture=window.captureCold=true;
      window.m1ColdFile=argv[++i];
    }
    else if (!std::wcscmp(argv[i], L"--m1-capture-hot")) window.captureHot=true;
    else if (!std::wcscmp(argv[i], L"--m1-dual-source")) window.dualSource=true;
    else if (!std::wcscmp(argv[i], L"--m1-source-trace")) window.sourceTrace=true;
    else if (!std::wcscmp(argv[i],L"--m1-d1") && i+1<argc) {
      window.d1Mode=argv[++i];
      window.sourceTrace=true;
    }
    else if (!std::wcscmp(argv[i],L"--m1-d1-replay-prefix") && i+1<argc) window.d1ReplayPrefix=argv[++i];
    else if ((!std::wcscmp(argv[i], L"--probe-m1-source") ||
              !std::wcscmp(argv[i], L"--present-m1-source")) && i+1 < argc) {
      window.m1Present = !std::wcscmp(argv[i], L"--present-m1-source");
      window.m1SourceModule = argv[++i];
      window.bootOnly = true;
    }
    else {
      std::cerr << "usage: hello_world_pc [--once] [--boot-only | --qualify-steady | "
                   "--qualify-steady-role NAME [--disable-optimization] | --probe-m1-source SPV | "
                   "--present-m1-source SPV | --m1-window SOURCE_SPV CANDIDATE_SPV COMMIT_SPV "
                   "[--m1-cold FILE | --m1-cold-on-busy FILE | --m1-root-on-busy FILE] "
                   "[--m1-capture-hot] [--m1-source-trace] [--m1-d1 OFF|ON|REPLAY_OFF|REPLAY_ON] "
                   "[--m1-d1-replay-prefix PATH] "
                   "[--m1-dual-source] | --qualify-t849-writes FILE | --g13-candidate FILE "
                   "[--g13-fixture F02|F03|F05 CONFIG STATE] [--g13-trial 1|2|4] [--g13-f05-rest] "
                   "[--g13-revoke-before | --g13-revoke-after]]\n";
      return 2;
    }
  }
  if (window.disableOptimization && window.steadyRole.empty()) return 2;
  if (!window.t849WriteFile.empty() && !window.steadyRole.empty()) return 2;
  if (window.steady && !window.m1SourceModule.empty()) return 2;
  if (window.captureHot && window.m1QuantumCandidate.empty()) return 2;
  if (window.captureHot && window.captureCold) return 2;
  if (window.rootCapture && window.once) return 2;
  const bool d1Replay=window.d1Mode==L"REPLAY_OFF" || window.d1Mode==L"REPLAY_ON";
  if (d1Replay!=!window.d1ReplayPrefix.empty()) return 2;
  if (!window.d1Mode.empty() && ((!d1Replay && window.d1Mode!=L"OFF" && window.d1Mode!=L"ON") ||
      window.m1QuantumCandidate.empty() || window.dualSource || window.once ||
      window.captureHot || !window.m1ColdFile.empty())) return 2;
  if (!window.t833RevokeAt.empty() && window.t833CandidateFile.empty()) return 2;
  if (!window.t833Case.empty() && window.t833CandidateFile.empty()) return 2;
  if (window.t833ForcedJ && window.t833Case!="F03") return 2;
  if (window.t833F05Rest && window.t833Case!="F05") return 2;
  if (!window.t833CandidateFile.empty() && (window.steady || !window.m1SourceModule.empty())) return 2;
  if (window.sourceTrace) window.ingress.EnableTrace();
  if (window.dualSource && (window.m1QuantumCandidate.empty() || !window.m1ColdFile.empty())) return 2;
  if ((!window.m1ColdFile.empty() && window.m1QuantumCandidate.empty()) ||
      (!window.m1QuantumCandidate.empty() && window.bootOnly)) return 2;
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  HINSTANCE instance = GetModuleHandleW(nullptr);
  WNDCLASSW type{};
  type.lpfnWndProc = Dispatch;
  type.hInstance = instance;
  type.lpszClassName = L"HelloWorldPcVulkanBoot";
  type.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
  if (!RegisterClassW(&type)) return 3;
  HWND hwnd = CreateWindowExW(0, type.lpszClassName,
      window.m1QuantumCandidate.empty() ? L"HelloWorld Vulkan BOOT diagnostic" : L"HelloWorld Vulkan M1",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 500, nullptr, nullptr, instance, &window);
  if (!hwnd) return 4;
  if ((!window.m1SourceModule.empty() || !window.t833CandidateFile.empty()) &&
      !SetTimer(hwnd,kSourceProbeTimer,10,nullptr)) return 5;
  if (!window.bootOnly || window.m1Present) ShowWindow(hwnd, SW_SHOW);
  window.Size();
  std::thread owner([&] { window.Owner(); });
  std::thread d1;
  if (!window.d1Mode.empty()) d1=std::thread([&] { window.RunD1(); });
  MSG message{};
  while (GetMessageW(&message, nullptr, 0, 0) > 0) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
  window.Close();
  owner.join();
  if (d1.joinable()) d1.join();
  return window.result;
}
