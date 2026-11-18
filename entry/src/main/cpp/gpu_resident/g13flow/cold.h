#pragma once
#include "../cold_slot_broker.h"
#include "controller.h"
#include <memory>
namespace grs::g13 {
class Cold {
  struct Slot {
    coldbroker::Lease credit;
    std::unique_ptr<uint32_t[]> words;
    size_t count = 0;
    uint64_t id = 0, request = 0, created = 0;
    uint32_t segments = 0, nextSegment = 0;
    size_t nextWord = 0;
    bool ready = false, full = false;
    std::string token;
  };
  std::array<Slot, 2> slots_{};
  int pending_ = -1;
  Controller *pendingOwner_ = nullptr;
  std::string reply_ = "{}";
  std::string Build(Controller &, bool, const std::string &);
  void ReleasePending();
  Slot *Find(uint64_t);
  void Trim(uint64_t);
  static bool Receipt(uint32_t *, size_t, uint32_t, const std::string &);

public:
  ~Cold() { ReleasePending(); }
  std::string Begin(Controller &, const std::string &token, bool full,
                    const std::string &scene);
  std::string Page(uint64_t, uint32_t, size_t);
  std::string End(uint64_t);
  std::string Service(Controller &, const std::string &);
  void CancelPending(uint32_t, const char *);
  bool Pending() const { return pending_ >= 0; }
  const std::string &Reply() const { return reply_; }
};
} // namespace grs::g13
