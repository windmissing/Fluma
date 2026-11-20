#pragma once
#include "gpu_resident/g9_boot_capture.h"
#include "gpu_resident/program_cache/mapped_build_id.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

namespace dmboot {
inline std::string Json(const std::string &text) {
  std::string result = "\"";
  for (unsigned char c : text) {
    if (c == '"' || c == '\\')
      result += '\\';
    if (c < 32) {
      constexpr char digits[] = "0123456789abcdef";
      result += "\\u00";
      result += digits[c >> 4];
      result += digits[c & 15];
    } else {
      result += char(c);
    }
  }
  return result + '"';
}

inline std::string ProcessStartTick() {
  std::ifstream proc("/proc/self/stat");
  std::string row, token;
  std::getline(proc, row);
  const auto end = row.rfind(')');
  if (end == std::string::npos)
    return {};
  std::istringstream fields(row.substr(end + 1));
  for (int field = 3; field <= 22; ++field)
    if (!(fields >> token))
      return {};
  return token.find_first_not_of("0123456789") == std::string::npos ? token : "";
}

class Receipt {
public:
  // The executable accepts one attempt per new output directory/process.
  static constexpr uint64_t kTicket = 1;
  Receipt(std::ofstream &out, const std::filesystem::path &dir)
      : rows_(out), output_(dir) {}

  void Identity(uint64_t run, uint64_t owner, uint64_t context) {
    static const char kExecutableAnchor = 0;
    char buildId[129]{}, path[4096]{};
    const bool mapped = grs::BuildIdForAddress(&kExecutableAnchor, buildId);
    const auto length = readlink("/proc/self/exe", path, sizeof(path));
    const bool pathObserved = length > 0 && size_t(length) < sizeof(path);
    const auto start = ProcessStartTick();
    rows_ << "{\"schema\":\"DM_BOOT_ATTEMPT_V1\",\"stage\":\"ATTEMPT_IDENTITY\",\"ticket\":\""
          << kTicket << "\",\"pid\":" << getpid() << ",\"processStartTick\":"
          << (start.empty() ? "null" : Json(start))
          << ",\"driverMappedBuildId\":" << (mapped ? Json(buildId) : "null")
          << ",\"driverMappedPath\":" << (pathObserved ? Json(std::string(path, length)) : "null")
          << ",\"run\":\"" << run << "\",\"owner\":\"" << owner
          << "\",\"context\":\"" << context << "\"}\n";
    rows_.flush();
  }

  grs::g9boot::Capture Capture() {
    return {kTicket, this, [](void *context, const char *row, size_t len,
                             const uint32_t *tail) noexcept {
      static_cast<Receipt *>(context)->Record(row, len, tail);
    }};
  }

  bool Good() const { return rows_.good() && !writeFailed_; }
  bool TailWritten() const { return tailWritten_; }

private:
  std::ofstream &rows_;
  const std::filesystem::path &output_;
  bool writeFailed_ = false, tailWritten_ = false;

  void Record(const char *row, size_t len, const uint32_t *tail) noexcept {
    try {
      rows_.write(row, len);
      rows_.put('\n');
      rows_.flush();
      if (!tail || !rows_)
        return;
      const auto file = output_ / "boot-work-tail.u32";
      std::ofstream out(file, std::ios::binary);
      for (size_t i = 0; i < 2; ++i) {
        const uint32_t word = tail[i];
        const char bytes[]{char(word), char(word >> 8), char(word >> 16), char(word >> 24)};
        out.write(bytes, sizeof(bytes));
      }
      out.close();
      tailWritten_ = out.good();
      writeFailed_ = !tailWritten_;
    } catch (...) {
      writeFailed_ = true;
    }
  }
};
} // namespace dmboot
