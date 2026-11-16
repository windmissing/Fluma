#pragma once
#include "profiles_generated.h"
namespace grs::m3a {
// Explicit V4 header gate. No V2/V3 data is interpreted as occupied-medium3.
inline bool ReadPublicationHeaderV4(const uint32_t* words,size_t length,const Profile& profile){
    if(!words||length!=256||words[0]!=1347637831||words[1]!=4||words[2]!=256||words[3]!=3235823838u)return false;
    if(words[31]>=2||words[34]>profile.k||words[35]>profile.kw||words[36]!=128)return false;
    for(size_t i=65;i<256;++i)if(words[i])return false;
    // Publication12..30 mirrors state14..32, including all nine versions.
    const uint32_t versions[]={1,1,1,3,4,4,4,15,0};
    for(size_t i=0;i<9;++i)if(words[22+i]!=versions[i])return false;
    return true;
}
}
