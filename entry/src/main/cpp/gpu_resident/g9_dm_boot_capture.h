#pragma once
#include "g9_boot_capture.h"
#include "g9dm/transport.h"

namespace grs::g9boot {
// Linked only in the DSO used by the standalone DM BOOT executable.
g9dm::Result CreateDmBootCaptured(g9dm::Transport &, const g9dm::Config &,
                                const g9dm::ControlIdentity &, const g9dm::SeedFileIdentity &,
                                const Capture &);
} // namespace grs::g9boot
