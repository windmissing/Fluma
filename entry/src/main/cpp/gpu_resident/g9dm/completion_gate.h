#pragma once
#include "research_config.h"
#include "../acceptance_t647/contract.h"
#include <optional>
namespace grs::g9dm {
struct NativeEvidence {uint32_t dispatches=0,barriers=0;uint64_t fenceNs=0;uint32_t gl=0,egl=0;bool fenceObserved=false;};
class CompletionGate;
class ValidatedRecord {
 Permit permit_;accept647::ValidatedAccepted proof_;
 ValidatedRecord(const Permit& p,const accept647::ValidatedAccepted& proof):permit_(p),proof_(proof){}
 friend class CompletionGate;
public:
 const Permit& Permission()const{return permit_;}const Completion256& Words()const{return proof_.Words();}
 bool ProducedBy(const void* producer)const{return proof_.Producer()==producer;}
 const auto& ExpectedHash()const{return proof_.ExpectedHash();}
};
class ValidatedBlockedControlRecord {
 Permit permit_;accept647::ValidatedBlocked proof_;
 ValidatedBlockedControlRecord(const Permit& p,const accept647::ValidatedBlocked& proof):permit_(p),proof_(proof){}
 friend class CompletionGate;
public:
 const Permit& Permission()const{return permit_;}const Completion256& Words()const{return proof_.Words();}
};
// Distinct composite proof: never accepted by the default bank commit API.
class ValidatedCompositeParentDM {
 accept647::ValidatedAccepted proof_;
 explicit ValidatedCompositeParentDM(const accept647::ValidatedAccepted& proof):proof_(proof){}
 friend class CompletionGate;
public:
 const auto& Words()const{return proof_.Words();}const auto& ExpectedHash()const{return proof_.ExpectedHash();}
 const void* Producer()const{return proof_.Producer();}
};
class ValidatedBlockedCompositeParentDM {
 accept647::ValidatedBlocked proof_;
 explicit ValidatedBlockedCompositeParentDM(const accept647::ValidatedBlocked& proof):proof_(proof){}
 friend class CompletionGate;
public:
 const auto& Words()const{return proof_.Words();}const auto& ExpectedHash()const{return proof_.ExpectedHash();}
 const void* Producer()const{return proof_.Producer();}
};
class CompletionGate {
 friend class Transport;
 friend class grs::pc::sand::VulkanOwner;
 static std::optional<ValidatedRecord> Accepted(const Permit&,const accept647::ExpectedContract&,const accept647::NativeMappedObservation&);
 static std::optional<ValidatedBlockedControlRecord> Blocked(const Permit&,const accept647::ExpectedContract&,const accept647::NativeMappedObservation&);
public:
 static std::optional<ValidatedCompositeParentDM> AcceptedComposite(const accept647::CompositeExpectedContract&,const accept647::NativeMappedObservation&);
 static std::optional<ValidatedBlockedCompositeParentDM> BlockedComposite(const accept647::CompositeExpectedContract&,const accept647::NativeMappedObservation&);
};
}
