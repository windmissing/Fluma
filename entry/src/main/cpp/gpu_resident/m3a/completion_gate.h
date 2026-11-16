#pragma once
#include "control_types.h"
#include "profiles_generated.h"
#include <optional>
namespace grs::m3a {
using Words256=std::array<uint32_t,256>;
struct RootPermitV4 {
    ControlIdentityV4 identity;
    uint64_t plan=0,nonce=0,control=0,frame=0,geometry=0,sourceCommit=0,physicsCommit=0,publicationGeneration=0,deadline=0;
    uint32_t kind=0,ordinal=0,baseBank=0,candidateBank=1,spareBank=2,eventArena=0,eventStart=0,eventCapacity=0,publicationSlot=0,uploadedBytes=0,carry=0;
    std::array<uint32_t,8> shaderSha{},angleSha{},layoutSha{};
};
class CompletionGate;
class ValidatedTailV4 {
    friend class CompletionGate;
    RootPermitV4 permit_;
    Words256 words_;
    explicit ValidatedTailV4(const RootPermitV4& p,const Words256& w):permit_(p),words_(w){}
public:
    const RootPermitV4& Permit()const{return permit_;}
    const Words256& Words()const{return words_;}
};
class ValidatedBlockedTailV4 {
    friend class CompletionGate;
    ValidatedTailV4 tail_;
    explicit ValidatedBlockedTailV4(ValidatedTailV4 value):tail_(std::move(value)){}
public:
    const RootPermitV4& Permit()const{return tail_.Permit();}
    const Words256& Words()const{return tail_.Words();}
};
class ValidatedSuccessTailV4 {
    friend class CompletionGate;
    ValidatedTailV4 tail_;
    explicit ValidatedSuccessTailV4(ValidatedTailV4 value):tail_(std::move(value)){}
public:
    const RootPermitV4& Permit()const{return tail_.Permit();}
    const Words256& Words()const{return tail_.Words();}
};
class ValidatedMetadataTailV4 {
    friend class CompletionGate;
    ValidatedTailV4 tail_;
    explicit ValidatedMetadataTailV4(ValidatedTailV4 value):tail_(std::move(value)){}
public:
    const RootPermitV4& Permit()const{return tail_.Permit();}
    const Words256& Words()const{return tail_.Words();}
};
struct NativeRootEvidence {
    uint32_t submittedPasses=0,barriers=0,uploadedBytes=0,gl=0,egl=0;
    uint64_t fenceSignalledNs=0;
};
class CompletionGate {
    static uint64_t Pair(const Words256& w,size_t at){return uint64_t(w[at])|(uint64_t(w[at+1])<<32);}
    static bool Common(const Profile&,const RootPermitV4&,const Words256&,const NativeRootEvidence&);
public:
    static std::optional<ValidatedBlockedTailV4> Blocked(const Profile&,const RootPermitV4&,const Words256&,const NativeRootEvidence&);
    static std::optional<ValidatedSuccessTailV4> Success(const Profile&,const RootPermitV4&,const Words256&,const NativeRootEvidence&);
    static std::optional<ValidatedMetadataTailV4> Metadata(const Profile&,const RootPermitV4&,const Words256&,const NativeRootEvidence&);
};
}
