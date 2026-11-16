#pragma once
#include "research_config.h"
#include <optional>
namespace grs::g9spf {
struct NativeEvidence {uint32_t dispatches=0,barriers=0;uint64_t fenceNs=0;uint32_t gl=0,egl=0;};
class CompletionGate;
class ValidatedRecord {
 Permit permit_;Completion256 tail_{};
 ValidatedRecord(const Permit& p,const Completion256& w):permit_(p),tail_(w){}
 friend class CompletionGate;
public:
 const Permit& Permission()const{return permit_;}const Completion256& Words()const{return tail_;}
};
// This is a G9-neutral control proof, not a cast of a V4 outcome or tail.
class ValidatedBlockedControlRecord {
 ValidatedRecord record_;
 explicit ValidatedBlockedControlRecord(const ValidatedRecord& r):record_(r){}
 friend class CompletionGate;
public:
 const Permit& Permission()const{return record_.Permission();}const Completion256& Words()const{return record_.Words();}
};
class CompletionGate {
 static bool Common(const Profile&,const Permit&,const Completion256&,const NativeEvidence&);
public:
 static std::optional<ValidatedRecord> Accepted(const Profile&,const Permit&,const Completion256&,const NativeEvidence&);
 static std::optional<ValidatedBlockedControlRecord> Blocked(const Profile&,const Permit&,const Completion256&,const NativeEvidence&);
};
}
