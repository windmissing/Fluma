#include "g9_dm_boot_capture.h"
#include "g9_boot.h"

namespace grs::g9boot {
namespace {
thread_local const Capture *active = nullptr;

class CaptureScope {
public:
  explicit CaptureScope(const Capture &capture) : previous_(active) { active = &capture; }
  ~CaptureScope() { active = previous_; }

private:
  const Capture *previous_;
};
} // namespace

const Capture *CurrentCapture() noexcept { return active; }

g9dm::Result CreateDmBootCaptured(g9dm::Transport &transport, const g9dm::Config &config,
                                const g9dm::ControlIdentity &id,
                                const g9dm::SeedFileIdentity &seed, const Capture &capture) {
  static const char kMappedDsoAnchor = 0;
  const CaptureScope scope(capture);
  Trace{"DM", g9dm::PipelineSourceHash(), g9dm::BootRoleSetSHA(), id.run, id.owner,
        id.context, &kMappedDsoAnchor, &capture}.Record("TRANSPORT_CREATE_ENTER");
  return transport.Create(config, id, seed);
}
} // namespace grs::g9boot
