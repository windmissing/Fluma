// Existing M2 resident index and weighted-RNE primitives are reused verbatim.
// All invocations enter this routine and every index barrier uniformly.
uniform uint uM3Index0,uM3Index1,uM3YRequest;
uniform int uM3Sin,uM3Cos;
shared uint m3SeedColumn,m3SeedRemaining,m3SeedAction,m3SeedSlotA,m3SeedSlotB,m3IndexAt;
const uint M3_SEED_NEXT=0u,M3_SEED_BIRTH=1u,M3_SEED_PAIR=2u,M3_SEED_STOP=3u;
void m2LoadSand(uint slot,out uint row[16]){for(uint f=0u;f<16u;++f)row[f]=C[f*uK+slot];}
void m3WriteSand(uint slot,uint row[16]){for(uint f=0u;f<16u;++f)C[f*uK+slot]=row[f];}
void m3SourceDerived(uint column){uint material=col(10u,column)+col(8u,column);C[uColumn+11u*128u+column]=material;C[uColumn+12u*128u+column]=material+col(6u,column);C[uColumn+13u*128u+column]=material+col(6u,column)+col(7u,column);}
void m3SeedFailure(uint reason){if(m3Get(M3_TRIAL_REASON)==0u)m3Put(M3_TRIAL_REASON,reason);m3SeedAction=M3_SEED_STOP;}
void m3SeedDecide(){
    m3SeedAction=M3_SEED_NEXT;if(W[0]!=0u||m3Get(M3_TRIAL_REASON)!=0u){m3SeedAction=M3_SEED_STOP;return;}
    if(m3SeedRemaining==0u)return;
    if(m3Get(M3_TRIAL_BIRTHS)>=uK){m3SeedFailure(5u);return;}
    if(C[uHeader+126u]!=0u||C[uHeader+39u]==0u){m3SeedFailure(6u);return;}
    for(uint i=0u;i<uK;++i)if(C[i]==0u){m3SeedSlotA=i;m3SeedAction=M3_SEED_BIRTH;return;}
    m3SeedAction=M3_SEED_PAIR;
}
bool m3SourceMutation(){uvec2 old=uvec2(C[uHeader+142u],C[uHeader+143u]),next;if(!grsAdd64(old,uvec2(1u,0u),next)){m3SourceFail(13u,142u);return false;}C[uHeader+142u]=next.x;C[uHeader+143u]=next.y;return true;}
void m3SeedMerge(uint a,uint b){
    uint beforeA[16],beforeB[16],afterA[16],afterB[16],row[128];m2LoadSand(a,beforeA);m2LoadSand(b,beforeB);
    uint column=beforeA[1]>>20u;
    if(beforeB[10]==0xffffffffu){m3SourceFail(21u,b);return;}
    if(!m2MergeRows(beforeA,beforeB,65536u,col(10u,column),m3Eta(column),col(6u,column),afterA,afterB)){m3SourceFail(15u,a);return;}
    if(!m3SourceCredit(3u))return;
    m3SourceRow(row,M3_MERGE,column);row[14]=0u;row[15]=a;row[16]=b;row[18]=beforeB[0];row[23]=row[24]=col(8u,column);row[19]=row[21]=m3Get(M3_TRIAL_BORN);row[26]=m3Get(M3_TRIAL_MERGES)+1u;row[28]=row[29]=C[uHeader+39u];
    for(uint f=0u;f<16u;++f){row[32u+f]=beforeA[f];row[48u+f]=beforeB[f];row[64u+f]=afterA[f];row[80u+f]=afterB[f];}
    m3SourceEmit(row);if(W[0]!=0u)return;
    if(!m3SourceMutation())return;
    m3WriteSand(a,afterA);m3WriteSand(b,afterB);C[uHeader+41u]--;C[uHeader+43u]++;m3Put(M3_TRIAL_MERGES,m3Get(M3_TRIAL_MERGES)+1u);
}
void m3SeedBirth(uint slot){
    uint old[16],born[16],row[128];m2LoadSand(slot,old);
    if(old[0]!=0u||old[10]==0xffffffffu){m3SourceFail(21u,slot);return;}
    uint column=m3SeedColumn,amount=min(122333u,m3SeedRemaining),id=C[uHeader+39u];
    uint finalEta=m3Get(M3_BASE_MATERIAL+column)+m3Get(M3_ADMITTED+column)+m3Get(M3_TARGET_WATER+column);
    uint bed=col(10u,column);
    bool bulk=m3Input(2u)==2u;
    if((bulk&&finalEta<bed+2u)||amount>m3Cap(column)-m3Eta(column)){m3SeedFailure(7u);return;}
    if(!m3SourceCredit(2u))return;
    uint y=bulk?max(bed+1u,min(m3Get(31u),finalEta-1u)):m3Get(31u);
    for(uint i=0u;i<16u;++i)born[i]=0u;
    born[0]=amount;born[1]=bulk?(column*1048576u)+524288u:m3Get(30u);born[2]=y;born[5]=id;born[6]=1u;born[7]=W[uM3Input+14u];born[8]=m3Get(M3_PENDING_ORIGIN_LO);born[9]=m3Get(M3_PENDING_ORIGIN_HI);born[10]=old[10]+1u;
    int vx,vy;
    if(!grsMulDivRNE(int(uM3Settle),uM3Sin,1048576u,vx)||!grsMulDivRNE(int(uM3Settle),uM3Cos,1048576u,vy)){m3SourceFail(13u,3u);return;}
    born[3]=bulk?uint(vx):0u;born[4]=bulk?uint(-vy):0u;born[14]=col(6u,column)>0u&&y>bed&&y<finalEta?1u:0u;
    m3SourceRow(row,M3_BIRTH,column);row[14]=0u;row[15]=slot;row[18]=amount;row[19]=m3Get(M3_TRIAL_BORN);row[21]=row[19]+amount;row[23]=col(8u,column);row[24]=row[23]+amount;row[28]=id;row[29]=id+1u;row[25]=m3Get(M3_TRIAL_BIRTHS)+1u;if(id==0xffffffffu)row[30]|=1u;
    for(uint f=0u;f<16u;++f){row[32u+f]=old[f];row[64u+f]=born[f];}
    m3SourceEmit(row);if(W[0]!=0u)return;
    if(!m3SourceMutation())return;
    m3WriteSand(slot,born);C[uHeader+39u]=id+1u;if(id==0xffffffffu)C[uHeader+126u]=1u;
    C[uHeader+41u]++;C[uHeader+43u]--;C[uColumn+8u*128u+column]+=amount;m3SourceDerived(column);
    m3SeedRemaining-=amount;m3Put(M3_TRIAL_BORN,m3Get(M3_TRIAL_BORN)+amount);m3Put(M3_TRIAL_BIRTHS,m3Get(M3_TRIAL_BIRTHS)+1u);
}
void m3BuildSeedIndex(){
    if(gl_LocalInvocationIndex==0u)m3IndexAt=uM3Index0;barrier();
    m2IndexSort(uM3Index0,uK==192u?256u:512u,uK);
}
void m3ExecuteSeeds(bool buildIndex){
    uint lane=gl_LocalInvocationIndex;
    if(lane==0u)m3IndexAt=uM3Index0;barrier();
    if(buildIndex)m3BuildSeedIndex();
    for(uint column=0u;column<128u;++column){
        if(lane==0u){
            m3SeedColumn=column;m3SeedRemaining=m3Get(M3_ADMITTED+column);m3SeedAction=M3_SEED_NEXT;
            if(m3Input(2u)==2u&&m3SeedRemaining>0u&&W[0]==0u&&m3Get(M3_TRIAL_REASON)==0u){uint row[128];m3Put(M3_SEED,m3Get(M3_SEED)+1u);m3SourceRow(row,M3_SEED_BEGIN,column);row[18]=m3SeedRemaining;row[41]=m3Get(M3_REQUESTED+column);row[42]=m3SeedRemaining;m3SourceEmit(row);}
        }
        memoryBarrierBuffer();barrier();
        for(uint iteration=0u;iteration<=uK;++iteration){
            if(lane==0u)m3SeedDecide();barrier();
            if(m3SeedAction==M3_SEED_STOP||m3SeedAction==M3_SEED_NEXT)break;
            if(m3SeedAction==M3_SEED_PAIR){
                uvec2 pair=m2IndexPair(m3IndexAt,uK==192u?256u:512u,uK,65536u);
                if(lane==0u){if(pair.x>=uK)m3SeedFailure(4u);else{m3SeedSlotA=pair.x;m3SeedSlotB=pair.y;m3SeedMerge(pair.x,pair.y);}}
                memoryBarrierBuffer();barrier();
                if(m3SeedAction==M3_SEED_STOP||W[0]!=0u)break;
                uint next=m3IndexAt==uM3Index0?uM3Index1:uM3Index0;
                m2IndexUpdate(m3IndexAt,next,uK==192u?256u:512u,uK,m3SeedSlotA,m3SeedSlotB);
                if(lane==0u){m3IndexAt=next;m3SeedSlotA=m3SeedSlotB;}barrier();
            }
            if(lane==0u)m3SeedBirth(m3SeedSlotA);
            memoryBarrierBuffer();barrier();
            if(W[0]!=0u||m3Get(M3_TRIAL_REASON)!=0u)break;
            uint next=m3IndexAt==uM3Index0?uM3Index1:uM3Index0;
            m2IndexUpdate(m3IndexAt,next,uK==192u?256u:512u,uK,m3SeedSlotA,uK);
            if(lane==0u)m3IndexAt=next;barrier();
        }
        if(W[0]!=0u||m3Get(M3_TRIAL_REASON)!=0u)break;
    }
}
void m3ExecuteSeeds(){m3ExecuteSeeds(true);}
