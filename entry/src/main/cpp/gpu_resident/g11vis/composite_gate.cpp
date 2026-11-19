#include "composite_gate.h"
namespace grs::g11vis {
namespace {
bool Identity(const CompositePermit& p,const accept647::ExpectedContract& e){return p.Nonce()==e.U64(18)&&p.Kind()==e.Word(3)&&p.visualGeneration==e.U64(62)&&p.visualTicks==e.U64(64)&&p.rootBeginNs==e.U64(30)&&p.compositeCode==e.Word(6)&&((p.parent.index()==0&&e.SelectedFamily()==accept647::Family::G11_R1)||(p.parent.index()==1&&e.SelectedFamily()==accept647::Family::G11_DM));}
bool Visual(const accept647::Tail& w,const Digest& hash){if((w[42]&0xc0000u)!=0xc0000u||w[156]!=1)return false;for(size_t i=0;i<8;++i)if(w[232+i]!=hash[i])return false;return true;}
}
std::optional<ValidatedCompositeAccepted> CompositeGate::Accepted(const CompositePermit& p,const accept647::ExpectedContract& e,const accept647::NativeMappedObservation& o){
 if(!Identity(p,e))return {};
 if(p.parent.index()==0){auto proof=g9spf::CompletionGate::AcceptedComposite(e,o);if(!proof||!Visual(proof->Words(),proof->ExpectedHash()))return {};return ValidatedCompositeAccepted(p,ParentAcceptedProof(*proof));}
 auto proof=g9dm::CompletionGate::AcceptedComposite(e,o);if(!proof||!Visual(proof->Words(),proof->ExpectedHash()))return {};return ValidatedCompositeAccepted(p,ParentAcceptedProof(*proof));
}
std::optional<ValidatedCompositeBlocked> CompositeGate::Blocked(const CompositePermit& p,const accept647::ExpectedContract& e,const accept647::NativeMappedObservation& o){
 if(!Identity(p,e))return {};
 if(p.parent.index()==0){auto proof=g9spf::CompletionGate::BlockedComposite(e,o);if(!proof||!Visual(proof->Words(),proof->ExpectedHash()))return {};return ValidatedCompositeBlocked(p,ParentBlockedProof(*proof));}
 auto proof=g9dm::CompletionGate::BlockedComposite(e,o);if(!proof||!Visual(proof->Words(),proof->ExpectedHash()))return {};return ValidatedCompositeBlocked(p,ParentBlockedProof(*proof));
}
}
