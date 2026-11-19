#pragma once
#include "parent_adapter.h"
#include "../g9spf/completion_gate.h"
#include "../g9dm/completion_gate.h"
namespace grs::g11vis {
using ParentAcceptedProof=std::variant<g9spf::ValidatedCompositeParentR1,g9dm::ValidatedCompositeParentDM>;
using ParentBlockedProof=std::variant<g9spf::ValidatedBlockedCompositeParentR1,g9dm::ValidatedBlockedCompositeParentDM>;
class CompositeGate;
class ValidatedCompositeAccepted {
 CompositePermit permit_;ParentAcceptedProof proof_;
 ValidatedCompositeAccepted(const CompositePermit& p,ParentAcceptedProof proof):permit_(p),proof_(std::move(proof)){}
 friend class CompositeGate;
public:
 const CompositePermit& Permission()const{return permit_;}
 const accept647::Tail& Words()const{return std::visit([](const auto& p)->const accept647::Tail&{return p.Words();},proof_);}
 const Digest& ExpectedHash()const{return std::visit([](const auto& p)->const Digest&{return p.ExpectedHash();},proof_);}
 const void* Producer()const{return std::visit([](const auto& p){return p.Producer();},proof_);}
};
class ValidatedCompositeBlocked {
 CompositePermit permit_;ParentBlockedProof proof_;
 ValidatedCompositeBlocked(const CompositePermit& p,ParentBlockedProof proof):permit_(p),proof_(std::move(proof)){}
 friend class CompositeGate;
public:
 const CompositePermit& Permission()const{return permit_;}
 const accept647::Tail& Words()const{return std::visit([](const auto& p)->const accept647::Tail&{return p.Words();},proof_);}
 const Digest& ExpectedHash()const{return std::visit([](const auto& p)->const Digest&{return p.ExpectedHash();},proof_);}
 const void* Producer()const{return std::visit([](const auto& p){return p.Producer();},proof_);}
};
class CompositeGate {
public:
 static std::optional<ValidatedCompositeAccepted> Accepted(const CompositePermit&,const accept647::ExpectedContract&,const accept647::NativeMappedObservation&);
 static std::optional<ValidatedCompositeBlocked> Blocked(const CompositePermit&,const accept647::ExpectedContract&,const accept647::NativeMappedObservation&);
};
}
