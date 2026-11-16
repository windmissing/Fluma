// Root-level schema/order check complements per-trial operand replay.
bool m3AuditSourceEvents(){
    uint rows=m3Get(M3_EVENT_CURSOR);if(rows==0u||rows>uM3SourceRows)return false;
    uint trial=0u,scopeOrdinal=0u,seenEnd=0u,activeTrial=0u;
    for(uint row=0u;row<rows;++row){
        uint at=row*128u,kind=E[at+2u];uvec4 mask=m3SourceMask(kind);
        if(all(equal(mask,uvec4(0u)))||E[at]!=876950349u||E[at+1u]!=4u||E[at+3u]!=1u||E[at+4u]!=uNonce.x||E[at+5u]!=uNonce.y||E[at+6u]!=row+1u||E[at+7u]!=0u||E[at+31u]!=3235774468u||E[at+8u]!=m3Input(6u)||E[at+9u]!=m3Input(7u))return false;
        for(uint field=0u;field<128u;++field)if(((mask[field/32u]>>(field%32u))&1u)==0u&&E[at+field]!=0u)return false;
        if(kind==M3_END){if(row+1u!=rows||seenEnd!=0u||activeTrial!=0u||E[at+96u]!=0u)return false;++seenEnd;continue;}
        if(m3Input(2u)==2u){
            if(kind==M3_TRIAL_BEGIN){if(activeTrial!=0u||E[at+97u]!=trial)return false;activeTrial=1u;scopeOrdinal=0u;}
            if(activeTrial==0u||E[at+96u]!=6u||E[at+97u]!=trial||E[at+103u]!=scopeOrdinal)return false;
            ++scopeOrdinal;
            if(kind==M3_TRIAL_END){activeTrial=0u;++trial;}
        }else if(E[at+96u]!=0u||E[at+103u]!=scopeOrdinal++)return false;
    }
    return seenEnd==1u&&(m3Input(2u)!=2u||trial==m3Get(M3_ATTEMPTS));
}
bool m3AuditRawCommand(){
    if(m3Get(M3_APPEND_DEDUP)==0u)return true;
    uint index=B[uHeader+33u];if(index>=4096u||C[uHeader+33u]!=index+1u)return false;
    for(uint i=0u;i<32u;++i)if(W[uM3Dedup+index*64u+i]!=m3Input(i))return false;
    for(uint i=0u;i<512u;++i)if(W[uM3RawDedup+index*512u+i]!=m3Input(32u+i))return false;
    return true;
}
uniform int uM3Tangent;
bool m3AuditSourceTransaction(){
    bool accepted=m3Get(M3_SOURCE_SUCCESS)!=0u;
    uvec2 source,physics,generation;
    if(!grsAdd64(uvec2(B[uHeader+14u],B[uHeader+15u]),uvec2(accepted?1u:0u,0u),source)||!grsAdd64(uvec2(B[uHeader+16u],B[uHeader+17u]),uvec2(accepted?1u:0u,0u),physics)||!grsAdd64(uvec2(B[uHeader+72u],B[uHeader+73u]),uvec2(1u,0u),generation))return false;
    if(any(notEqual(source,uvec2(C[uHeader+14u],C[uHeader+15u])))||any(notEqual(physics,uvec2(C[uHeader+16u],C[uHeader+17u])))||any(notEqual(generation,uvec2(C[uHeader+72u],C[uHeader+73u]))))return false;
    if(C[uHeader+34u]!=B[uHeader+34u]+(accepted?1u:0u)||C[uHeader+33u]!=B[uHeader+33u]+m3Get(M3_APPEND_DEDUP)||C[uHeader+150u]!=0u||C[uHeader+151u]!=0u||C[uHeader+158u]!=0u)return false;
    if(C[uHeader+148u]!=(m3Input(2u)==2u?m3Get(M3_ATTEMPTS):0u))return false;
    if((m3Get(M3_ATTEMPTS)>0u||m3Input(2u)!=2u&&m3Get(M3_ROOT_STATE)!=0u)&&m3Get(M3_REPLAY_DONE)!=1u)return false;
    if(int(C[uHeader+45u])< -7680||int(C[uHeader+45u])>7680)return false;
    for(uint c=0u;c<128u;++c){
        if(C[uColumn+14u*128u+c]!=B[uColumn+14u*128u+c]||C[uColumn+15u*128u+c]!=B[uColumn+15u*128u+c])return false;
        if(c<127u&&abs(int(col(10u,c+1u))-int(col(10u,c))-uM3Tangent)>605394)return false;
    }
    for(uint i=52u;i<=55u;++i)if(C[uHeader+i]!=B[uHeader+i])return false;
    return true;
}
