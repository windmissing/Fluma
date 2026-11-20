#pragma once
#include "wire.h"

namespace grs::g11vis {
// App mutex protects this bounded journal. An evicted, late owner response
// cannot mutate the occupying attempt.
struct ConfigureEvidence {
  struct Attempt {
    uint64_t id = 0, queuedNs = 0, ownerNs = 0, returnedNs = 0;
    const char *stage = "QUEUED";
    std::string admission = "null", scope = "null", ownerResult = "null",
                firstFailure = "null", cleanup = "null", bootstrap = "null",
                termination = "null", retiredPrior = "null", window = "null",
                loadedModule = "null";

    void Fail(const char *at, const std::string &raw) {
      if (firstFailure == "null")
        firstFailure = "{\"stage\":" + Quote(at) + ",\"raw\":" + raw + "}";
    }
  };
  std::array<Attempt, 8> rows{};
  uint64_t sequence = 0;

  Attempt *Find(uint64_t id) {
    auto &row = rows[id % rows.size()];
    return id && row.id == id ? &row : nullptr;
  }

  Attempt &Begin(uint64_t now) {
    auto &row = rows[++sequence % rows.size()];
    row = {};
    row.id = sequence;
    row.queuedNs = now;
    return row;
  }

  std::string Json() const {
    std::ostringstream out;
    out << '[';
    bool first = true;
    for (uint64_t i = sequence; i && sequence - i < rows.size(); --i) {
      const auto &a = rows[i % rows.size()];
      if (!first)
        out << ',';
      first = false;
      const auto time = [](uint64_t ns) { return ns ? Quote(std::to_string(ns)) : "null"; };
      out << "{\"attempt\":\"" << a.id << "\",\"stage\":" << Quote(a.stage)
          << ",\"queuedNs\":" << time(a.queuedNs) << ",\"ownerNs\":"
          << time(a.ownerNs) << ",\"returnedNs\":" << time(a.returnedNs)
          << ",\"admission\":" << a.admission << ",\"scope\":" << a.scope
          << ",\"nativeWindow\":" << a.window
          << ",\"ownerResult\":" << a.ownerResult << ",\"firstFailure\":"
          << a.firstFailure << ",\"cleanup\":" << a.cleanup
          << ",\"retiredPrior\":" << a.retiredPrior << ",\"bootstrap\":"
          << a.bootstrap << ",\"termination\":" << a.termination
          << ",\"loadedModule\":" << a.loadedModule << '}';
    }
    out << ']';
    return out.str();
  }
};
} // namespace grs::g11vis
