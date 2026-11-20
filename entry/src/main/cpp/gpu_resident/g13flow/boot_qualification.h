#pragma once
#include "../g9rx/loaded_identity.h"
#include "wire.h"
#include <GLES3/gl31.h>
#include <sstream>
#include <unistd.h>

namespace grs::g13 {
struct BootReadback {
  bool controlRead = false, mathRead = false;
  bool op65InputsRead = false, op66InputsRead = false;
  uint32_t seal = 0, tailError = 0, result = 0;
  GLenum fence = 0, firstGl = 0;
  int64_t firstEgl = -1, mathReadGl = -1, op65InputGl = -1, op66InputGl = -1;
  const char *sourceSHA = "";
  const char *effectiveRejectStage = "NONE";
  const char *op65State = "UNOBSERVED";
  const char *op66State = "UNOBSERVED";
  GLuint program = 0, ctrl = 0, work = 0;
  uintptr_t fenceHandle = 0;
  size_t ctrlTailByteOffset = 0, workByteOffset = 0;
  std::string route, cacheKey, binarySHA, environmentSHA;
  std::array<uint32_t, 16> fp32{};
  std::array<uint32_t, 5> math{};
  std::array<uint32_t, 4> op65Inputs{};
  std::array<uint32_t, 5> op66Inputs{};
  std::array<bool, 2> present{};
  std::array<uint32_t, 2> mismatches{};
  bool qualified = false;
};

inline std::string ProcessStartTicks() {
  FILE *file = std::fopen("/proc/self/stat", "r");
  if (!file)
    return {};
  char row[4096]{};
  const bool good = std::fgets(row, sizeof(row), file) != nullptr;
  std::fclose(file);
  const char *end = good ? std::strrchr(row, ')') : nullptr;
  if (!end)
    return {};
  std::istringstream fields(end + 1);
  std::string token;
  for (unsigned field = 3; field <= 22; ++field)
    if (!(fields >> token))
      return {};
  return token.find_first_not_of("0123456789") == std::string::npos
             ? token : std::string{};
}

template <class Words>
void BootWords(std::ostringstream &out, const char *name, const Words &raw,
               bool available) {
  out << ",\"" << name << "\":[";
  if (available)
    for (size_t i = 0; i < raw.size(); ++i) {
      if (i)
        out << ',';
      out << raw[i];
    }
  out << ']';
}
} // namespace grs::g13
