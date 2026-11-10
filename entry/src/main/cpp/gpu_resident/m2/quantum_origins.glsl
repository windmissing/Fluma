// V3 inventory extension. This range belongs exclusively to QUANTUM WORK;
// SOURCE and QUANTUM never share a mutable origin accumulator.
bool quantumOriginsV3(){
    uint n=C[uHeader+34u], scratch=uSourceEvents+4096u*16u+uWords+256u;
    if(n>4096u||C[uHeader+14u]!=n||C[uHeader+15u]!=0u||C[uHeader+132u]!=1u||
       C[uHeader+133u]!=122333u||C[uHeader+134u]!=122333u||C[uHeader+135u]!=65536u||
       C[uHeader+136u]!=uK||C[uHeader+140u]!=128u)return false;
    uvec2 sand=uvec2(0u), water=uvec2(0u), serial=uvec2(0u);
    for(uint i=0u;i<n;++i){
        uint a=uSourceEvents+16u*i,m=W[a+6u];
        if(W[a+2u]!=i+1u||W[a+3u]!=0u||W[a+4u]>1u||m==0u||m>CQ||W[a+9u]==0u||W[a+14u]>W[a+8u]||W[a+8u]>(W[a+4u]==0u?uK:1u)||W[a+15u]>5u)return false;
        if(W[a+4u]==0u){if(!grsAdd64(sand,uvec2(m,0u),sand))return false;}
        else if(!grsAdd64(water,uvec2(m,0u),water))return false;
        if(!grsAdd64(serial,uvec2(W[a+8u]+W[a+14u],0u),serial))return false;
        W[scratch+2u*i]=0u;W[scratch+2u*i+1u]=0u;
    }
    if(serial!=uvec2(C[uHeader+142u],C[uHeader+143u]))return false;
    if(sand!=uvec2(C[uHeader+60u],C[uHeader+61u])||water!=uvec2(C[uHeader+62u],C[uHeader+63u]))return false;
    for(uint ns=0u;ns<2u;++ns)for(uint i=0u;i<slots(ns);++i){
        uint mass=C[at(ns,0u,i)];if(mass==0u)continue;
        uint f=ns==0u?8u:6u,origin=C[at(ns,f,i)];
        if(origin==0u||origin>n||C[at(ns,f+1u,i)]!=0u)return false;
        uint a=uSourceEvents+16u*(origin-1u);
        if(ns==1u){if(mass>W[a+6u])return false;continue;}
        uint p=scratch+2u*(origin-1u);uvec2 sum;
        if(mass>244666u||!grsAdd64(uvec2(W[p],W[p+1u]),uvec2(mass,0u),sum)||sum.y!=0u||sum.x>W[a+6u])return false;
        W[p]=sum.x;W[p+1u]=sum.y;
    }
    return true;
}
