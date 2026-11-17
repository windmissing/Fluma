#include "quota_reader_v3.h"
namespace grs::g9dm {
QuotaInspectionV3 InspectQuotaSnapshotV3(const QuotaSnapshotV3& w){
    QuotaInspectionV3 out;
    if(w[0]!=860963661||w[1]!=3||w[2]!=256||w[3]!=0xc0de0003||w[4]>3||(w[5]&~255u)||w[6]>4||w[7]>4||w[8]>2||w[9]>2)return out;
    for(size_t i=11;i<16;++i)if(w[i])return out;
    for(size_t i=100;i<256;++i)if(w[i])return out;
    auto p=[&](size_t i){return uint64_t(w[i])|(uint64_t(w[i+1])<<32);};
    uint64_t blocked=0;if(!CheckedAdd(p(44),p(46),blocked)||!CheckedAdd(blocked,p(48),blocked)||blocked!=p(50)||p(38)>=1000000000)return out;
    uint64_t sum=0;for(uint64_t v:{p(32),uint64_t(w[6]),uint64_t(w[7]),p(34),p(36),blocked})if(!CheckedAdd(sum,v,sum))return out;
    if(sum!=p(30))return out;
    auto right=WideUnsigned::Multiply(p(30),1000000000);if(!right.Add(p(38))||!right.Add(p(40))||!right.Add(p(52))||!(right==WideUnsigned::Multiply(p(58),120)))return out;
    const bool open=(w[5]&1u)!=0,ready=(w[5]&2u)!=0,baseline=(w[5]&4u)!=0,hasPulse=(w[5]&8u)!=0,live=(w[5]&16u)!=0,claim=(w[5]&32u)!=0,submit=(w[5]&64u)!=0;
    if((ready&&!open)||(baseline&&(!ready||!hasPulse))||(live&&(!p(72)||!w[8])))return out;
    if((w[4]==1||w[4]==2)&&(w[6]||w[7]||live||claim||submit||p(72)))return out;
    if(claim&&(w[4]!=0||!open||!ready||!baseline||!hasPulse||p(72)||live))return out;
    if(submit&&(w[4]!=0||!open||!ready||!baseline||!hasPulse||!p(72)))return out;
    if(p(72)&&p(72)==p(74))return out;
    const int32_t target=int32_t(w[98]);if(w[99]!=(target<0?UINT32_MAX:0)||target< -7680||target>7680)return out;
    if(p(16)!=p(88))return out;
    out.result=ControlResult::OK;out.generated=p(30);out.accepted=p(32);out.blocked=blocked;out.creditedElapsedNs=p(58);return out;
}
}
