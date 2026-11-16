// T497 V4 occupied-head solvers. Called by lane zero of one 64-lane group.
// State access uses the current role's C bank. WORK scratch is caller-owned.
uniform uint uM3Scratch,uM3Payload,uM3WaterRate,uM3Settle;
const uint M3_DESCRIPTOR=0u,M3_REQUESTED=128u,M3_ADMITTED=256u,M3_TARGET_WATER=384u;
const uint M3_CAP=512u,M3_BASE_ETA=640u,M3_BASE_MATERIAL=768u,M3_WATER_ERROR=896u;
const uint M3_LAMBDA=0u,M3_TRIAL_REASON=1u,M3_WANT=2u,M3_ETA_HI=3u,M3_RESIDUAL=4u,M3_WATER_BUDGET=5u;
uint m3Get(uint at){return W[uM3Scratch+at];}
void m3Put(uint at,uint value){W[uM3Scratch+at]=value;}
bool m3Sum(inout uvec2 total,uint value){uvec2 next;if(!grsAdd64(total,uvec2(value,0u),next))return false;total=next;return true;}
uint m3Cap(uint c){
    uint cap=CQ-col(7u,c);
    for(uint i=0u;i<uKw;++i){
        uint mass=C[uWater+i];
        if(mass==0u||(C[uWater+uKw+i]>>20u)!=c)continue;
        uint y=C[uWater+2u*uKw+i];
        if(y==0u)return 0u;
        cap=min(cap,y-1u);
    }
    return cap;
}
uint m3Material(uint c){return col(10u,c)+col(8u,c);}
uint m3Eta(uint c){return m3Material(c)+col(6u,c);}
bool m3CaptureColumns(){
    for(uint c=0u;c<128u;++c){
        uint solid=col(10u,c),particles=col(8u,c),water=col(6u,c),flight=col(7u,c);
        if(solid>CQ||particles>CQ-solid||water>CQ-solid-particles||flight>CQ-solid-particles-water)return false;
        uint material=solid+particles,eta=material+water,cap=m3Cap(c);
        if(eta>cap||col(11u,c)!=material||col(12u,c)!=eta||col(13u,c)!=eta+flight)return false;
        m3Put(M3_CAP+c,cap);m3Put(M3_BASE_ETA+c,eta);m3Put(M3_BASE_MATERIAL+c,material);
    }
    return true;
}
bool m3DecodeBulk(){
    uvec2 total=uvec2(0u);
    for(uint c=0u;c<128u;++c){
        uint at=uM3Payload+4u*c;
        if(W[at+2u]!=0u||W[at+3u]!=0u)return false;
        uvec2 raw=uvec2(W[at],W[at+1u]);uint quantity=0u;
        if(!grsFloorBinary64Q20(raw,CQ,quantity)||(raw.y&0x7fffffffu)>0x40700000u||((raw.y&0x7fffffffu)==0x40700000u&&raw.x!=0u))return false;
        m3Put(M3_REQUESTED+c,quantity);
        if(!m3Sum(total,quantity)||total.y!=0u||total.x>CQ)return false;
    }
    return true;
}
uvec2 m3TrialWaterSum(uint shift,bool store){
    uvec2 sum=uvec2(0u);
    for(uint c=0u;c<128u;++c){
        uint virtualMaterial=m3Get(M3_BASE_MATERIAL+c)+m3Get(M3_ADMITTED+c);
        uint eta=clamp(m3Get(M3_BASE_ETA+c)+shift,virtualMaterial,m3Get(M3_CAP+c));
        uint water=eta-virtualMaterial;
        m3Sum(sum,water);if(store)m3Put(M3_TARGET_WATER+c,water);
    }
    return sum;
}
// 0 SUCCESS, 1 NUMERIC_RESOLUTION, 2 CAP_BOUND_RETRY, 3 ETA_CAPACITY_RETRY,
// 4 ETA_TRUNCATION_RETRY. Root driver translates these into frozen enum fields.
uint m3TrialEta(uint lambda){
    m3Put(M3_LAMBDA,lambda);uvec2 wanted=uvec2(0u),bound=uvec2(0u),water=uvec2(0u);
    for(uint c=0u;c<128u;++c){
        uint request=m3Get(M3_REQUESTED+c);uvec2 scaled=grsMul32(request,lambda);
        uint admitted=(scaled.x>>16u)|(scaled.y<<16u);m3Put(M3_ADMITTED+c,admitted);
        m3Sum(wanted,admitted);m3Sum(water,col(6u,c));
        if(request>0u)m3Sum(bound,m3Get(M3_CAP+c)-m3Get(M3_BASE_ETA+c));
    }
    if(wanted.y!=0u||wanted.x>CQ)return 3u;
    m3Put(M3_WANT,wanted.x);
    if(wanted.x==0u)return 1u;
    if(grsCompare64(wanted,bound)>0){
        // In this branch bound < wanted <= CQ, therefore its high word is zero.
        uvec4 quotient;uint remainder;
        if(!grsDivide128(uvec4(grsMul32(lambda,bound.x),0u,0u),wanted.x,quotient,remainder)||any(notEqual(quotient.yzw,uvec3(0u))))return 3u;
        m3Put(M3_LAMBDA,quotient.x);return 2u;
    }
    for(uint c=0u;c<128u;++c)if(m3Get(M3_ADMITTED+c)>m3Get(M3_CAP+c)-m3Get(M3_BASE_MATERIAL+c))return 3u;
    if(grsCompare64(m3TrialWaterSum(CQ,false),water)<0)return 4u;
    uint low=0u,high=CQ;
    for(uint iteration=0u;iteration<64u;++iteration){if(low<high){uint middle=low+(high-low)/2u;if(grsCompare64(m3TrialWaterSum(middle,false),water)>=0)high=middle;else low=middle+1u;}}
    uint eHi=low;m3Put(M3_ETA_HI,eHi);
    for(uint c=0u;c<128u;++c)if(m3Get(M3_BASE_ETA+c)+eHi>m3Get(M3_CAP+c))return 4u;
    uvec2 sumHi=m3TrialWaterSum(eHi,false);
    uint residual=0u;
    if(grsCompare64(sumHi,water)==0)m3TrialWaterSum(eHi,true);
    else{
        if(eHi==0u)return 4u;
        uvec2 sumLo=m3TrialWaterSum(eHi-1u,true);uvec2 difference;
        if(!grsSub64(water,sumLo,difference)||difference.y!=0u||difference.x>=128u)return 4u;
        residual=difference.x;uint remaining=residual;
        for(uint c=0u;c<128u;++c){
            uint virtualMaterial=m3Get(M3_BASE_MATERIAL+c)+m3Get(M3_ADMITTED+c);
            uint highWater=clamp(m3Get(M3_BASE_ETA+c)+eHi,virtualMaterial,m3Get(M3_CAP+c))-virtualMaterial;
            uint lowerWater=m3Get(M3_TARGET_WATER+c);
            if(highWater>lowerWater&&remaining>0u){m3Put(M3_TARGET_WATER+c,lowerWater+1u);--remaining;}
        }
        if(remaining!=0u)return 4u;
    }
    m3Put(M3_RESIDUAL,residual);return 0u;
}
int m3Offset(uint c,int tangent){int value=0;if(!grsMulDivRNE(tangent,int(2u*c)-127,2u,value))return 0;return value;}
uint m3LevelWater(int level,uint c,int tangent){
    int eta=clamp(level+m3Offset(c,tangent),int(m3Get(M3_BASE_MATERIAL+c)),int(m3Get(M3_CAP+c)));
    return uint(eta)-m3Get(M3_BASE_MATERIAL+c);
}
uvec2 m3LevelWaterSum(int level,int tangent,bool store){
    uvec2 sum=uvec2(0u);
    for(uint c=0u;c<128u;++c){uint water=m3LevelWater(level,c,tangent);m3Sum(sum,water);if(store)m3Put(M3_TARGET_WATER+c,water);}
    return sum;
}
bool m3WaterTarget(int tangent){
    if(!m3CaptureColumns()||uM3WaterRate>1006632960u)return false;
    uvec2 volume=uvec2(0u);for(uint c=0u;c<128u;++c)m3Sum(volume,col(6u,c));
    int low=-int(CQ),high=2*int(CQ);
    for(uint iteration=0u;iteration<40u;++iteration){if(low<high){int middle=low+(high-low)/2;if(grsCompare64(m3LevelWaterSum(middle,tangent,false),volume)>=0)high=middle;else low=middle+1;}}
    uvec2 highSum=m3LevelWaterSum(low,tangent,false);
    if(grsCompare64(highSum,volume)==0)m3LevelWaterSum(low,tangent,true);
    else{
        uvec2 lower=m3LevelWaterSum(low-1,tangent,true);uvec2 delta;
        if(!grsSub64(volume,lower,delta)||delta.y!=0u||delta.x>=128u)return false;
        uint remaining=delta.x;
        for(uint c=0u;c<128u;++c){uint old=m3Get(M3_TARGET_WATER+c);if(m3LevelWater(low,c,tangent)>old&&remaining>0u){m3Put(M3_TARGET_WATER+c,old+1u);--remaining;}}
        if(remaining!=0u)return false;
    }
    uvec2 positive=uvec2(0u);
    for(uint c=0u;c<128u;++c){uint actual=col(6u,c),target=m3Get(M3_TARGET_WATER+c);m3Put(M3_WATER_ERROR+c,uint(int(actual)-int(target)));if(actual>target)m3Sum(positive,actual-target);}
    uint alpha=min(Q,uM3WaterRate/120u);uvec4 quotient;uint remainder;
    if(!grsDivide128(grsMul64(positive,uvec2(alpha,0u)),Q,quotient,remainder)||any(notEqual(quotient.zw,uvec2(0u))))return false;
    m3Put(M3_WATER_BUDGET,quotient.x);m3Put(M3_WATER_BUDGET+1u,quotient.y);return true;
}
