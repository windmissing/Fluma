#include "controller.h"
#include "scene.h"
// Read-only compiled ABI evidence; no executable probe or product execution.
extern "C" __attribute__((visibility("default")))
const uint64_t G11NativeAbiV2[] = {
    2,
    sizeof(grs::g11vis::VisualSeedReceipt),
    sizeof(grs::g11vis::VisualBootstrap),
    sizeof(grs::g11vis::PhaseWitness),
    sizeof(grs::g11vis::VisualTransport),
    sizeof(grs::g11vis::Controller),
    sizeof(grs::g11vis::Cold),
    sizeof(grs::g11vis::CompositeScene),
    sizeof(grs::g11vis::R1Control),
    sizeof(grs::g11vis::DMControl),
    sizeof(grs::accept647::ProgramFacts),
    sizeof(grs::accept647::ActiveProgramIdentity),
    sizeof(grs::accept647::ExpectedContract),
    sizeof(grs::accept647::NativeMappedObservation),
    sizeof(std::optional<grs::accept647::ActiveProgramIdentity>),
    sizeof(std::optional<grs::accept647::ExpectedContract>),
    sizeof(grs::g11vis::ValidatedCompositeAccepted),
    sizeof(grs::g11vis::ValidatedCompositeBlocked),
    sizeof(grs::g11vis::CompositePermit),
    sizeof(grs::g11vis::Scope)};
