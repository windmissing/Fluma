// Trial audit uses provisional origin/base+born, never public source commit.
bool m3AuditTrialState(){
    uvec2 sand=uvec2(0u),water=uvec2(0u);uint born=m3Get(M3_TRIAL_BORN),births=m3Get(M3_TRIAL_BIRTHS),merges=m3Get(M3_TRIAL_MERGES);
    if(births>uK||merges>births)return false;
    uint liveS=0u,liveW=0u;
    for(uint ns=0u;ns<2u;++ns){
        uint count=ns==0u?uK:uKw,base=ns==0u?0u:uWater,generation=ns==0u?10u:11u;
        for(uint slot=0u;slot<count;++slot){
            uint mass=C[base+slot];if(mass==0u){for(uint f=0u;f<16u;++f)if(f!=generation&&C[base+count*f+slot]!=0u)return false;continue;}
            uint origin=C[base+count*(ns==0u?8u:6u)+slot],originHi=C[base+count*(ns==0u?9u:7u)+slot],id=C[base+5u*count+slot];
            if(originHi!=0u||origin==0u||id==0u||C[base+generation*count+slot]==0u||mass>(ns==0u?244666u:CQ))return false;
            for(uint other=slot+1u;other<count;++other)if(C[base+other]!=0u&&C[base+5u*count+other]==id)return false;
            if(origin>m3SourceBase(34u)){
                uint first=m3SourceBase(ns==0u?39u:40u);
                if(ns!=(m3Input(2u)==1u?1u:0u)||origin!=m3Get(M3_PENDING_ORIGIN_LO)||m3Get(M3_PENDING_ORIGIN_HI)!=0u||id<first||id-first>=births||C[base+(ns==0u?7u:12u)*count+slot]!=m3Input(14u))return false;
            }else{
                uint at=uM3SourceEvents+(origin-1u)*16u,kind=W[at+4u];
                if((kind==1u?1u:0u)!=ns||C[base+count*(ns==0u?7u:12u)+slot]!=W[at+9u]||id<W[at+7u]||id-W[at+7u]>=W[at+8u])return false;
            }
            if(ns==0u)++liveS;else ++liveW;
        }
    }
    for(uint c=0u;c<128u;++c){
        uint bed=col(10u,c),w=col(6u,c),p=0u,j=0u;
        if(bed!=B[uColumn+10u*128u+c]||bed!=col(2u,c))return false;
        for(uint i=0u;i<uK;++i)if((C[uK+i]>>20u)==c){if(C[i]>CQ-p)return false;p+=C[i];}
        for(uint i=0u;i<uKw;++i)if((C[uWater+uKw+i]>>20u)==c){if(C[uWater+i]>CQ-j)return false;j+=C[uWater+i];}
        if(p>CQ-bed||w>CQ-bed-p||j>CQ-bed-p-w||p!=col(8u,c)||j!=col(7u,c)||col(11u,c)!=bed+p||col(12u,c)!=bed+p+w||col(13u,c)!=bed+p+w+j||bed+p+w>m3Cap(c))return false;
        if(!m3Sum(sand,bed+p)||!m3Sum(water,w+j))return false;
    }
    uvec2 expectedSand,expectedWater;
    bool isWater=m3Input(2u)==1u;
    if(!grsAdd64(uvec2(m3SourceBase(56u),m3SourceBase(57u)),uvec2(isWater?0u:born,0u),expectedSand)||!grsAdd64(uvec2(m3SourceBase(58u),m3SourceBase(59u)),uvec2(isWater?born:0u,0u),expectedWater))return false;
    return all(equal(sand,expectedSand))&&all(equal(water,expectedWater))&&liveS==C[uHeader+41u]&&liveW==C[uHeader+42u]&&C[uHeader+43u]==uK-liveS&&C[uHeader+44u]==uKw-liveW;
}
