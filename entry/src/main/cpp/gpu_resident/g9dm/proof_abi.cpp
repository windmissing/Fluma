#include "transport.h"
#include "controller.h"
#include "../g9spf/completion_gate.h"
#include <type_traits>
static_assert(!std::is_default_constructible_v<grs::accept647::ActiveProgramIdentity>);
static_assert(!std::is_default_constructible_v<grs::accept647::NativeMappedObservation>);
static_assert(!std::is_default_constructible_v<grs::g9dm::ValidatedRecord>);
static_assert(!std::is_default_constructible_v<grs::g9dm::ValidatedCompositeParentDM>);
static_assert(!std::is_default_constructible_v<grs::g9spf::ValidatedCompositeParentR1>);
extern "C" __attribute__((visibility("default"),used)) const unsigned long long grs_dm_t647_proof_abi[]={
1,224,896,sizeof(grs::accept647::ProgramFacts),sizeof(grs::accept647::ActiveProgramIdentity),sizeof(grs::accept647::ExpectedContract),sizeof(grs::accept647::ActualEvidence),sizeof(grs::accept647::NativeMappedObservation),sizeof(grs::g9dm::ValidatedRecord),sizeof(grs::g9dm::ValidatedCompositeParentDM),sizeof(grs::g9spf::ValidatedCompositeParentR1),sizeof(grs::g9dm::Transport),sizeof(grs::g9dm::Config),sizeof(grs::g9dm::Controller),sizeof(grs::g9dm::ValidatedBlockedControlRecord),sizeof(grs::g9spf::ValidatedRecord)};
