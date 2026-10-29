#include "profile.h"
#include <limits>
namespace grs { namespace m1 {
ConfigureCode Plan(uint32_t K,uint32_t Kw,uint32_t feature,uint32_t wireRevision,uint64_t align,uint64_t maxBlock,ProfilePlan& out){
 out={};if(feature!=features||wireRevision!=revision)return ConfigureCode::Unsupported;
 const Profile* p=nullptr;for(const auto& v:profiles)if(v.K==K&&v.Kw==Kw)p=&v;
 if(!p)return ConfigureCode::Unsupported;
 if(align==0||align%4!=0||maxBlock>uint64_t(std::numeric_limits<int64_t>::max()))return ConfigureCode::InvalidRange;
 auto aligned=[&](uint64_t n,uint64_t& result){uint64_t pad=(align-n%align)%align;if(n>maxBlock||pad>maxBlock-n)return false;result=n+pad;return true;};
 ProfilePlan candidate;candidate.profile=p;
 if(!aligned(p->stateBytes,candidate.stateStride)||!aligned(p->publicationBytes,candidate.publicationStride)||!aligned(uint64_t(p->events)*256,candidate.eventStride))return ConfigureCode::InvalidRange;
 out=candidate;return ConfigureCode::ProtocolReadyShaderNotImplemented;
}
}}
