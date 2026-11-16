// Source event serialization is shared by candidate execution and independent replay.
uniform uvec2 uNonce,uFrame,uControl;
uniform uint uM3SourceRows,uM3Trial,uM3Input;
const uint M3_EVENT_CURSOR=16u,M3_SCOPE_ORDINAL=17u,M3_BRANCH_LO=18u,M3_BRANCH_HI=19u;
const uint M3_TRIAL_BIRTHS=20u,M3_TRIAL_MERGES=21u,M3_SEED=22u,M3_TRIAL_BORN=23u;
const uint M3_PENDING_ORIGIN_LO=24u,M3_PENDING_ORIGIN_HI=25u,M3_SOURCE_COLUMN=26u;
bool m3Replay=false;
void m3SourceFail(uint reason,uint word){
    if(W[0]!=0u)return;
    W[0]=reason;W[2]=uPass+1u;W[3]=m3Get(M3_EVENT_CURSOR)+1u;W[4]=word;
}
void m3SourceRow(out uint row[128],uint kind,uint column){
    for(uint i=0u;i<128u;++i)row[i]=0u;
    row[0]=876950349u;row[1]=4u;row[2]=kind;row[3]=1u;
    row[4]=uNonce.x;row[5]=uNonce.y;row[6]=m3Get(M3_EVENT_CURSOR)+1u;
    row[8]=W[uM3Input+6u];row[9]=W[uM3Input+7u];row[10]=m3Get(M3_PENDING_ORIGIN_LO);row[11]=m3Get(M3_PENDING_ORIGIN_HI);
    uvec2 nextPhysics=uvec2(C[uHeader+16u],C[uHeader+17u]);
    if(kind!=M3_END&&!grsAdd64(uvec2(m3SourceBase(16u),m3SourceBase(17u)),uvec2(1u,0u),nextPhysics)){m3SourceFail(13u,16u);return;}
    row[12]=nextPhysics.x;row[13]=nextPhysics.y;row[17]=column;row[25]=m3Get(M3_TRIAL_BIRTHS);row[26]=m3Get(M3_TRIAL_MERGES);
    row[30]=C[uHeader+126u]|(C[uHeader+127u]<<1u)|4u;
    row[96]=W[uM3Input+2u]==2u?6u:0u;row[97]=uM3Trial;row[98]=m3Get(M3_SEED);row[99]=column;
    row[100]=m3Get(M3_LAMBDA);row[101]=m3Get(M3_BRANCH_LO);row[102]=m3Get(M3_BRANCH_HI);row[103]=m3Get(M3_SCOPE_ORDINAL);
}
bool m3SourceCredit(uint required){
    uint cursor=m3Get(M3_EVENT_CURSOR);
    if(cursor>uM3SourceRows||required>uM3SourceRows-cursor){m3SourceFail(12u,0u);return false;}return true;
}
void m3SourceEmit(uint row[128]){
    if(W[0]!=0u||!m3SourceCredit(1u))return;
    uint ordinal=m3Get(M3_EVENT_CURSOR);uvec4 mask=m3SourceMask(row[2]);
    if(all(equal(mask,uvec4(0u)))){m3SourceFail(15u,2u);return;}
    for(uint i=0u;i<128u;++i){uint enabled=(mask[i/32u]>>(i%32u))&1u;if(enabled==0u)row[i]=0u;}
    row[31]=3235774468u;
    if(m3Replay){
        if(E[ordinal*128u+31u]!=3235774468u){m3SourceFail(31u,31u);return;}
        for(uint i=0u;i<128u;++i)if(E[ordinal*128u+i]!=row[i]){m3SourceFail(31u,i);return;}
    }else{
        for(uint i=0u;i<128u;++i)if(i!=31u)E[ordinal*128u+i]=row[i];
        memoryBarrierBuffer();E[ordinal*128u+31u]=3235774468u;
    }
    m3Put(M3_EVENT_CURSOR,ordinal+1u);m3Put(M3_SCOPE_ORDINAL,m3Get(M3_SCOPE_ORDINAL)+1u);
}
