// Rebuild caches only at actual writer cuts; this consumes accepted MEDIUM
// labels and never reclassifies a packet from the later water surface.
uniform uint uV5SourceLedger,uV5SourceMeta,uV5OriginScratch;
void v5RefreshAcceptedMedium(){
 uvec2 wet=uvec2(0u),air=uvec2(0u),tags=uvec2(0u);
 for(uint c=0u;c<128u;++c){uint u=0u;for(uint slot=0u;slot<uK;++slot)if(C[slot]!=0u&&(C[uK+slot]>>20u)==c&&C[14u*uK+slot]==1u)u+=C[slot];C[uColumn+4u*128u+c]=u;}
 for(uint slot=0u;slot<uK;++slot)if(C[slot]!=0u){
  uvec2 next;if(!grsAdd64(C[14u*uK+slot]==1u?wet:air,uvec2(C[slot],0u),next))return;
  if(C[14u*uK+slot]==1u)wet=next;else air=next;
  if(!grsAdd64(tags,uvec2(C[v5Aux(slot)],0u),next))return;tags=next;
 }
 C[uHeader+173u]=wet.x;C[uHeader+174u]=wet.y;C[uHeader+175u]=air.x;C[uHeader+176u]=air.y;C[uHeader+177u]=tags.x;C[uHeader+178u]=tags.y;
}
bool v5CertificateValid(uint slot,uvec2 latestCommit){
 uint a=v5Cert(slot),b=v5Aux(slot);if(C[slot]==0u)return false;
 if(C[a]!=C[5u*uK+slot]||C[a+1u]!=C[10u*uK+slot]||C[a+2u]!=C[8u*uK+slot]||C[a+3u]!=C[9u*uK+slot]||C[a+4u]!=C[7u*uK+slot]||
    C[a+5u]>1u||C[a+8u]==0u||C[a+9u]==0u||C[a+9u]>122333u||C[a+10u]>=128u||C[a+13u]!=1u||C[a+14u]!=0u||C[a+15u]!=0u||
    C[b]>C[slot]||C[b+1u]!=C[a+5u]||C[b+2u]!=1u||C[b+3u]!=0u)return false;
 uvec2 birth=uvec2(C[a+6u],C[a+7u]);if(all(equal(birth,uvec2(0u)))||grsCompare64(birth,latestCommit)>0)return false;
 if(C[a+5u]==0u){
  uint origin=C[a+2u];if(origin==0u||origin>C[uHeader+34u])return false;
  uint ledger=uV5SourceLedger+(origin-1u)*16u,meta=uV5SourceMeta+(origin-1u)*32u;
  return C[a+11u]==0u&&C[a+12u]==0u&&C[a]>=W[ledger+7u]&&C[a]-W[ledger+7u]<W[ledger+8u]&&
         all(equal(birth,uvec2(W[meta+10u],W[meta+11u])));
 }
 return C[a+11u]<64u&&C[a+12u]>=C[a+9u]&&C[a+12u]<=268435456u;
}
bool v5AuditState(bool acceptedOrigin){
 if(C[uHeader+161u]!=1u||C[uHeader+162u]>2u||C[uHeader+163u]>65536u||C[uHeader+164u]>65536u||C[uHeader+181u]!=64u)return false;
 uvec2 wet=uvec2(0u),air=uvec2(0u),tags=uvec2(0u);
 if(acceptedOrigin)for(uint i=0u;i<8192u;++i)W[uV5OriginScratch+i]=0u;
 for(uint c=0u;c<128u;++c){
  if(!v5AuditLayers(c))return false;uint u=0u;
  if(acceptedOrigin)for(uint rank=0u;rank<C[uV5Depth+c];++rank){
   uint layer=v5Layer(c,rank),origin=C[layer],mass=C[layer+3u];
   if(origin==0u||origin>C[uHeader+34u])return false;uint ledger=uV5SourceLedger+(origin-1u)*16u;
   if(W[ledger+4u]==1u||W[ledger+4u]>2u||W[ledger+9u]!=C[layer+2u])return false;
   uint at=uV5OriginScratch+(origin-1u)*2u;uvec2 sum;if(!grsAdd64(uvec2(W[at],W[at+1u]),uvec2(mass,0u),sum))return false;W[at]=sum.x;W[at+1u]=sum.y;
  }
  for(uint slot=0u;slot<uK;++slot)if(C[slot]!=0u&&(C[uK+slot]>>20u)==c&&C[14u*uK+slot]==1u)u+=C[slot];
  if(u!=C[uColumn+4u*128u+c])return false;
 }
 for(uint slot=0u;slot<uK;++slot){
  if(C[slot]==0u){for(uint f=0u;f<16u;++f)if(C[v5Cert(slot)+f]!=0u)return false;for(uint f=0u;f<4u;++f)if(C[v5Aux(slot)+f]!=0u)return false;continue;}
  if(C[v5Aux(slot)]>C[slot]||C[v5Aux(slot)+2u]!=1u)return false;
  if(acceptedOrigin){
   uint origin=C[8u*uK+slot];if(origin==0u||origin>C[uHeader+34u])return false;uint at=uV5OriginScratch+(origin-1u)*2u;
   uvec2 sum;if(!grsAdd64(uvec2(W[at],W[at+1u]),uvec2(C[slot],0u),sum))return false;W[at]=sum.x;W[at+1u]=sum.y;
  }
  uvec2 next;if(!grsAdd64(C[14u*uK+slot]==1u?wet:air,uvec2(C[slot],0u),next))return false;if(C[14u*uK+slot]==1u)wet=next;else air=next;
  if(!grsAdd64(tags,uvec2(C[v5Aux(slot)],0u),next))return false;tags=next;
 }
 uvec2 cycle;if(!grsSub64(uvec2(C[uHeader+169u],C[uHeader+170u]),uvec2(C[uHeader+171u],C[uHeader+172u]),cycle)||any(notEqual(cycle,tags)))return false;
 if(acceptedOrigin)for(uint origin=0u;origin<C[uHeader+34u];++origin){uint ledger=uV5SourceLedger+origin*16u,at=uV5OriginScratch+origin*2u;uint expected=W[ledger+4u]==1u?0u:W[ledger+6u];if(W[at]!=expected||W[at+1u]!=0u)return false;}
 return all(equal(wet,uvec2(C[uHeader+173u],C[uHeader+174u])))&&all(equal(air,uvec2(C[uHeader+175u],C[uHeader+176u])))&&all(equal(tags,uvec2(C[uHeader+177u],C[uHeader+178u])));
}
