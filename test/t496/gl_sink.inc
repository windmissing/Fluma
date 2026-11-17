// T496 CREATE_ONLY GL 编译链 ENTER/EXIT sink（T518 §3.3）。
// 用法：主 TU 在 include 产品 TU（app_bridge.cpp）之前：
//   #include "gl_sink.inc"        // 命名空间+真实现
//   #define glCreateShader t496_sink_glCreateShader
//   ...（6 个）
//   #include ".../app_bridge.cpp" // 产品代码调用被宏改写到 t496_sink_*
//   #undef ...（6 个）
// 产品 TU 之后 GLES3/gl3.h 再次声明原名不影响（宏已 undef）。
// 状态自持：不依赖主 TU 的 g_runId/g_arm/JsonEscape。
#pragma once
#include <cstring>
#include <fstream>
#include <chrono>
#include <string>
#include <mutex>
#include <atomic>
#include <sstream>

namespace t496sink {
inline std::string& RunId() { static std::string s; return s; }
inline std::string& Arm() { static std::string s; return s; }
inline std::string JEsc(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (static_cast<unsigned char>(c) < 0x20) { char b[8]; std::snprintf(b, sizeof(b), "\\u%04x", c); o += b; }
        else o += c;
    }
    return o;
}
inline std::ofstream& TraceFile() {
    static std::ofstream f(".codex/local-test/t496/t496-gltrace.ndjson", std::ios::app);
    return f;
}
inline std::mutex& TraceMu() { static std::mutex m; return m; }
inline std::atomic<uint64_t>& CallSeq() { static std::atomic<uint64_t> s{0}; return s; }
inline uint64_t WallNs() {
    return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
inline void Emit(const char* event, const char* call, uint64_t seq, uint64_t wallNs,
                 const std::string& detail = "") {
    std::lock_guard<std::mutex> lk(TraceMu());
    auto& f = TraceFile();
    f << "{\"runId\":\"" << JEsc(RunId()) << "\",\"arm\":\"" << JEsc(Arm())
      << "\",\"event\":\"" << event << "\",\"call\":\"" << call
      << "\",\"callSeq\":" << seq << ",\"wallNs\":" << wallNs;
    if (!detail.empty()) f << ",\"detail\":\"" << JEsc(detail) << "\"";
    f << "}\n" << std::flush;
}
} // namespace t496sink

