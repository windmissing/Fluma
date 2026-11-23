#include "boot.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cwchar>
#include <iostream>
#include <mutex>
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
  std::atomic<uint64_t> sourcePulse{0};
  bool pointerReady = false;
  grs::g13vk::PointerSample held;
  int result = 1;

  void Emit(const std::string &json) {
    std::lock_guard lock(output);
    std::cout << json << std::endl;
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
    wake.notify_one();
  }

  void Input(const char *kind, int x, int y) {
    std::lock_guard lock(mutex);
    if (closing) return;
    const auto sequence = ++eventSeq;
    const bool inside = x >= 0 && y >= 0 && x < width && y < height;
    const std::string action(kind);
    if (action == "DOWN") {
      held = {};
      if (pointerReady && inside) held.hold = sequence;
    } else if (action != "MOVE" || !inside) {
      held = {};
    }
    if (held.hold) {
      held.sequence = sequence;
      held.timeNs = grs::pc::TimeNs();
      held.x = (double(x)+0.5)*128/width;
      held.y = (1-(double(y)+0.5)/height)*256;
    }
    Emit("{\"stage\":\"WIN32_POINTER\",\"kind\":\"" + action +
         "\",\"geometry\":\"" + std::to_string(generation) +
         "\",\"seq\":\"" + std::to_string(sequence) +
         "\",\"qpc\":\"" + std::to_string(grs::pc::QpcTicks()) +
         "\",\"x\":" + std::to_string(x) + ",\"y\":" + std::to_string(y) +
         ",\"dpi\":" + std::to_string(dpi) + ",\"hold\":\"" + std::to_string(held.hold) + "\"}");
  }

  void Close() {
    std::lock_guard lock(mutex);
    closing = true;
    held = {};
    wake.notify_one();
  }

  void Owner() {
    uint64_t previous = 0;
    std::unique_lock lock(mutex);
    for (;;) {
      wake.wait(lock, [&] { return closing || (generation != previous && width > 0 && height > 0); });
      if (closing) break;
      previous = generation;
      const auto snapshotDpi = dpi;
      const auto snapshotWidth = width, snapshotHeight = height;
      lock.unlock();
      auto current = [&] {
        std::lock_guard guard(mutex);
        return !closing && generation == previous;
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
                                               [&] {
                                                 std::lock_guard guard(mutex);
                                                 pointerReady = !closing && generation == previous;
                                               },
                                               [&] {
                                                 std::lock_guard guard(mutex);
                                                 return !closing && generation == previous ? held :
                                                     grs::g13vk::PointerSample{};
                                               },
                                               [&](uint64_t hold, const std::function<void()> &accept) {
                                                 std::lock_guard guard(mutex);
                                                 if (closing || generation != previous || (hold && held.hold != hold))
                                                   return false;
                                                 accept();
                                                 return true;
                                               }},
          [&](const std::string &report) {
            Emit(report);
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
        }
      }
      const char *qualified = m1Present ? "\"state\":\"M1_DRY_SOURCE_PRESENT_RETIRED\"" :
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
  case WM_CANCELMODE: window->Input("CANCEL", 0, 0); if (GetCapture() == hwnd) ReleaseCapture(); return 0;
  case WM_CAPTURECHANGED: window->Input("CAPTURE_LOST", 0, 0); return 0;
  case WM_KILLFOCUS: window->Input("FOCUS_LOST", 0, 0); if (GetCapture() == hwnd) ReleaseCapture(); return 0;
  case WM_CLOSE: window->Close(); return 0;
  case kFirstPresent:
    if (window->once) SetTimer(hwnd, 1, 700, nullptr);
    return 0;
  case WM_TIMER:
    if (w == kSourceProbeTimer) {
      window->sourcePulse.store(grs::pc::TimeNs(), std::memory_order_relaxed);
      return 0;
    }
    KillTimer(hwnd, 1);
    window->Close();
    return 0;
  case kOwnerFinished: DestroyWindow(hwnd); return 0;
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
    else if (!std::wcscmp(argv[i], L"--m1-window") && i+3 < argc) {
      window.m1SourceModule = argv[++i];
      window.m1QuantumCandidate = argv[++i];
      window.m1QuantumCommit = argv[++i];
      window.m1Present = true;
    }
    else if (!std::wcscmp(argv[i], L"--m1-cold") && i+1 < argc) {
      window.m1ColdFile = argv[++i];
    }
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
                   "[--m1-cold FILE]]\n";
      return 2;
    }
  }
  if (window.disableOptimization && window.steadyRole.empty()) return 2;
  if (window.steady && !window.m1SourceModule.empty()) return 2;
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
  if (!window.m1SourceModule.empty() && !SetTimer(hwnd, kSourceProbeTimer, 10, nullptr)) return 5;
  if (!window.bootOnly || window.m1Present) ShowWindow(hwnd, SW_SHOW);
  window.Size();
  std::thread owner([&] { window.Owner(); });
  MSG message{};
  while (GetMessageW(&message, nullptr, 0, 0) > 0) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
  window.Close();
  owner.join();
  return window.result;
}
