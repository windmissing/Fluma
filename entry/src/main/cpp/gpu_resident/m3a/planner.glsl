// T497 fixed five-role planner. Branch selection stays on GPU.
uniform uint uM3Omega,uM3CandidateRole;
uniform int uM3Target;
const uint M3_PLAN=2048u,M3_ROLE_DESCRIPTORS=2112u,M3_ROLE_WORDS=64u;
const uint M3_PLAN_SELECTED=0u,M3_PLAN_STATUS=1u,M3_PLAN_J0=2u,M3_PLAN_REQUESTED=3u,M3_PLAN_CLAMPED=4u;
const uint M3_PLAN_EPOCH_LO=5u,M3_PLAN_EPOCH_HI=6u,M3_PLAN_BLOCKED=7u;
uint m3PlanGet(uint field){return m3Get(M3_PLAN+field);}
void m3PlanPut(uint field,uint value){m3Put(M3_PLAN+field,value);}
uint m3RoleAddress(uint role){return uM3Scratch+M3_ROLE_DESCRIPTORS+role*M3_ROLE_WORDS;}
int m3Tangent(int index){return int(W[uAngles+uint(index+7680)*4u]);}
bool m3EntryFeasible(int index){
    int tangent=m3Tangent(index);
    for(uint c=0u;c<127u;++c)if(abs(int(col(10u,c+1u))-int(col(10u,c))-tangent)>605394)return false;
    return true;
}
void m3PlanInitialize(){
    for(uint i=0u;i<64u;++i)m3PlanPut(i,0u);
    for(uint r=0u;r<6u;++r)for(uint i=0u;i<64u;++i)W[m3RoleAddress(r)+i]=0u;
    int base=int(B[uHeader+45u]);
    if(base< -7680||base>7680||uM3Target< -7680||uM3Target>7680||uM3Omega>1966080u){qfail(20u);return;}
    if(!m3EntryFeasible(base)){qfail(14u);return;}
    int step=int(uM3Omega/120u),requested=clamp(uM3Target,max(-7680,base-step),min(7680,base+step));
    m3PlanPut(M3_PLAN_J0,uint(base));m3PlanPut(M3_PLAN_REQUESTED,uint(requested));m3PlanPut(M3_PLAN_STATUS,4u);
    m3PlanPut(M3_PLAN_EPOCH_LO,B[uHeader+152u]);m3PlanPut(M3_PLAN_EPOCH_HI,B[uHeader+153u]);
}
bool m3RoleExecute(uint role){
    if(W[0]!=0u)return false;
    uint selected=m3PlanGet(M3_PLAN_SELECTED);
    if(role==1u)return true;
    if(role==2u)return selected==0u&&m3PlanGet(M3_PLAN_CLAMPED)!=m3PlanGet(M3_PLAN_REQUESTED)&&W[m3RoleAddress(1u)+3u]!=0u;
    if(role==3u)return selected==0u;
    return role>=4u&&role<=5u&&selected!=0u;
}
int m3RoleAngle(uint role){
    if(role==1u)return int(m3PlanGet(M3_PLAN_REQUESTED));
    if(role==2u)return int(m3PlanGet(M3_PLAN_CLAMPED));
    if(role==3u)return int(m3PlanGet(M3_PLAN_J0));
    uint selected=m3PlanGet(M3_PLAN_SELECTED);return int(W[m3RoleAddress(selected)+1u]);
}
void m3RoleInitialize(uint role){
    uint at=m3RoleAddress(role);bool enabled=m3RoleExecute(role);W[at]=enabled?1u:0u;W[at+1u]=uint(m3RoleAngle(role));W[at+2u]=0u;W[at+3u]=0u;W[at+4u]=4u;
    if(!enabled)return;
    uvec2 epoch,next;epoch=uvec2(m3PlanGet(M3_PLAN_EPOCH_LO),m3PlanGet(M3_PLAN_EPOCH_HI));
    if(!grsAdd64(epoch,uvec2(1u,0u),next)){qfail(13u);return;}
    m3PlanPut(M3_PLAN_EPOCH_LO,next.x);m3PlanPut(M3_PLAN_EPOCH_HI,next.y);W[at+5u]=next.x;W[at+6u]=next.y;
    W[at+7u]=m3EntryFeasible(int(W[at+1u]))?1u:0u;
}
// Runs after that role's real physics/audit/replay, or on its unchanged base
// when its entrance angle has no legal hard-guard interval.
void m3RoleInterval(uint role){
    uint at=m3RoleAddress(role);if(W[at]==0u)return;
    int lower=-2147483647,upper=2147483647;uint lowerEdge=0xffffffffu,upperEdge=0xffffffffu;
    for(uint c=0u;c<127u;++c){
        int delta=int(col(10u,c+1u))-int(col(10u,c));int lo=delta-605394,hi=delta+605394;
        if(lo>lower){lower=lo;lowerEdge=c;}if(hi<upper){upper=hi;upperEdge=c;}
    }
    int base=int(m3PlanGet(M3_PLAN_J0)),step=int(uM3Omega/120u);
    int first=max(-7680,max(base-step,min(base,uM3Target))),last=min(7680,min(base+step,max(base,uM3Target)));
    // The frozen angle table is monotone. Search both interval endpoints.
    int lo=first,hi=last+1;
    while(lo<hi){int mid=lo+(hi-lo)/2;if(m3Tangent(mid)>=lower)hi=mid;else lo=mid+1;}
    first=lo;lo=first;hi=last+1;
    while(lo<hi){int mid=lo+(hi-lo)/2;if(m3Tangent(mid)>upper)hi=mid;else lo=mid+1;}
    last=lo-1;
    bool interval=first<=last&&first<=7680&&last>=-7680;
    int own=int(W[at+1u]);W[at+2u]=interval&&own>=first&&own<=last&&W[at+7u]!=0u?1u:0u;W[at+3u]=interval?1u:0u;
    W[at+8u]=uint(first);W[at+9u]=uint(last);W[at+10u]=lowerEdge;W[at+11u]=upperEdge;W[at+12u]=uint(lower);W[at+13u]=uint(upper);
    if(role==1u&&interval)m3PlanPut(M3_PLAN_CLAMPED,uint(clamp(own,first,last)));
}
// Existing PLAN_CONTROL_1/2/3 slots call this after each preview descriptor.
void m3PlanDecision(uint role){
    if(W[0]!=0u||m3PlanGet(M3_PLAN_SELECTED)!=0u)return;
    uint at=m3RoleAddress(role);
    if(W[at+2u]!=0u){m3PlanPut(M3_PLAN_SELECTED,role);m3PlanPut(M3_PLAN_STATUS,role==1u?0u:role==2u?1u:2u);return;}
    if(role==3u){m3PlanPut(M3_PLAN_BLOCKED,1u);m3PlanPut(M3_PLAN_STATUS,3u);}
}
