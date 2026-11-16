// Shared V4 state audit. All state/ledger references are frozen role inputs.
// One caller owns the 8192-word origin accumulator in its packed WORK.
uniform uint uM3OriginScratch;
bool m3AuditInventory(){
    if(C[uHeader]!=1213420103u||C[uHeader+1u]!=4u||C[uHeader+2u]!=256u)return false;
    const uint versions[9]=uint[9](1u,1u,1u,3u,4u,4u,4u,15u,0u);
    for(uint i=0u;i<9u;++i)if(C[uHeader+24u+i]!=versions[i])return false;
    if(C[uHeader+122u]!=1u||C[uHeader+123u]!=65536u||C[uHeader+124u]!=2u||C[uHeader+125u]!=2u||C[uHeader+145u]!=1u||C[uHeader+146u]!=7u||C[uHeader+156u]!=512u)return false;
    for(uint i=161u;i<256u;++i)if(C[uHeader+i]!=0u)return false;
    uint prefix=C[uHeader+34u];
    if(prefix>4096u||C[uHeader+33u]<prefix||C[uHeader+33u]>4096u||C[uHeader+14u]!=prefix||C[uHeader+15u]!=0u)return false;
    uvec2 sourcedSand=uvec2(0u),sourcedWater=uvec2(0u);
    for(uint origin=0u;origin<prefix;++origin){
        uint at=uM3SourceEvents+origin*16u,meta=uM3SourceMeta+origin*32u,kind=W[at+4u],quantity=W[at+6u];
        if(kind>2u||quantity==0u||quantity>CQ||W[at+2u]!=origin+1u||W[at+3u]!=0u||W[at+9u]==0u||W[at+8u]>uK||W[at+14u]>W[at+8u])return false;
        if(W[meta]!=kind||W[meta+6u]!=quantity||W[meta+8u]!=origin+1u||W[meta+9u]!=0u||W[meta+14u]!=W[at+9u])return false;
        for(uint field=28u;field<32u;++field)if(W[meta+field]!=0u)return false;
        if(kind==1u){if(!m3Sum(sourcedWater,quantity))return false;}
        else if(!m3Sum(sourcedSand,quantity))return false;
        W[uM3OriginScratch+origin*2u]=0u;W[uM3OriginScratch+origin*2u+1u]=0u;
    }
    uint sandLive=0u,waterLive=0u;
    for(uint ns=0u;ns<2u;++ns){
        uint count=ns==0u?uK:uKw,base=ns==0u?0u:uWater,generation=ns==0u?10u:11u,next=C[uHeader+39u+ns],exhausted=C[uHeader+126u+ns];
        if(exhausted>1u||(ns==0u?((next==0u)!=(exhausted==1u)):(next==0u||(exhausted==1u&&next!=0xffffffffu))))return false;
        for(uint slot=0u;slot<count;++slot){
            uint mass=C[base+slot];
            if(mass==0u){for(uint f=0u;f<16u;++f)if(f!=generation&&C[base+f*count+slot]!=0u)return false;continue;}
            uint x=C[base+count+slot],y=C[base+count*2u+slot],id=C[base+count*5u+slot];
            uint tag=C[base+count*(ns==0u?6u:10u)+slot],origin=C[base+count*(ns==0u?8u:6u)+slot],originHi=C[base+count*(ns==0u?9u:7u)+slot];
            int vx=int(C[base+3u*count+slot]),vy=int(C[base+4u*count+slot]);
            if(mass>(ns==0u?244666u:CQ)||x>=134217728u||y>CQ||id==0u||C[base+generation*count+slot]==0u||vx< -1073741824||vx>1073741824||vy< -1073741824||vy>1073741824||(ns==1u&&vy>0))return false;
            if((tag&15u)<1u||(tag&15u)>3u||(tag&0xffff0ff0u)!=0u||(tag>>12u)>6u||origin==0u||origin>prefix||originHi!=0u)return false;
            if(next!=0u&&id>=next&&!(ns==1u&&exhausted==1u&&id==0xffffffffu))return false;
            for(uint other=slot+1u;other<count;++other)if(C[base+other]!=0u&&C[base+5u*count+other]==id)return false;
            uint event=uM3SourceEvents+(origin-1u)*16u,kind=W[event+4u];
            if((kind==1u?1u:0u)!=ns||C[base+count*(ns==0u?7u:12u)+slot]!=W[event+9u]||id<W[event+7u]||id-W[event+7u]>=W[event+8u])return false;
            if(ns==0u){
                if(C[11u*count+slot]!=(tag>>12u)||C[12u*count+slot]!=0u||C[13u*count+slot]!=0u||C[14u*count+slot]>1u)return false;
                uint at=uM3OriginScratch+(origin-1u)*2u;uvec2 sum;
                if(!grsAdd64(uvec2(W[at],W[at+1u]),uvec2(mass,0u),sum)||sum.y!=0u||sum.x>W[event+6u])return false;
                W[at]=sum.x;W[at+1u]=sum.y;++sandLive;
            }else{if(C[base+15u*count+slot]!=0u||mass>W[event+6u])return false;++waterLive;}
        }
    }
    uvec2 sand=uvec2(0u),water=uvec2(0u),settled=uvec2(0u),flight=uvec2(0u);
    for(uint c=0u;c<128u;++c){
        uint bed=col(10u,c),w=col(6u,c),particles=0u,inflight=0u;
        if(bed!=col(2u,c)||bed>CQ||w>CQ-bed)return false;
        for(uint f=0u;f<10u;++f)if(f!=2u&&f!=6u&&f!=7u&&f!=8u&&col(f,c)!=0u)return false;
        for(uint i=0u;i<uK;++i)if((C[uK+i]>>20u)==c){if(C[i]>CQ-particles)return false;particles+=C[i];}
        for(uint i=0u;i<uKw;++i)if((C[uWater+uKw+i]>>20u)==c){if(C[uWater+i]>CQ-inflight)return false;inflight+=C[uWater+i];}
        if(particles>CQ-bed||w>CQ-bed-particles||inflight>CQ-bed-particles-w||col(8u,c)!=particles||col(7u,c)!=inflight||col(11u,c)!=bed+particles||col(12u,c)!=bed+particles+w||col(13u,c)!=bed+particles+w+inflight||bed+particles+w>m3Cap(c))return false;
        if(!m3Sum(sand,bed+particles)||!m3Sum(water,w+inflight)||!m3Sum(settled,w)||!m3Sum(flight,inflight))return false;
    }
    uvec2 physical;if(!grsAdd64(uvec2(C[uHeader+14u],C[uHeader+15u]),uvec2(C[uHeader+118u],C[uHeader+119u]),physical)||any(notEqual(physical,uvec2(C[uHeader+16u],C[uHeader+17u]))))return false;
    uvec2 expectedSand,expectedWater;
    if(!grsAdd64(uvec2(C[uHeader+64u],C[uHeader+65u]),sourcedSand,expectedSand)||!grsAdd64(uvec2(C[uHeader+66u],C[uHeader+67u]),sourcedWater,expectedWater)||any(notEqual(sand,expectedSand))||any(notEqual(water,expectedWater)))return false;
    return sandLive==C[uHeader+41u]&&waterLive==C[uHeader+42u]&&C[uHeader+43u]==uK-sandLive&&C[uHeader+44u]==uKw-waterLive&&all(equal(sand,uvec2(C[uHeader+56u],C[uHeader+57u])))&&all(equal(water,uvec2(C[uHeader+58u],C[uHeader+59u])))&&all(equal(sourcedSand,uvec2(C[uHeader+60u],C[uHeader+61u])))&&all(equal(sourcedWater,uvec2(C[uHeader+62u],C[uHeader+63u])))&&all(equal(settled,uvec2(C[uHeader+112u],C[uHeader+113u])))&&all(equal(flight,uvec2(C[uHeader+114u],C[uHeader+115u])));
}
