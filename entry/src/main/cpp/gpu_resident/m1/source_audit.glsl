// Source-only audit families. No CPU inventories or inferred success masks.
bool auditSourceDomains(){
 if(!audit(false)||W[S+12u]!=1u)return false;
 for(uint i=24u;i<=32u;i++)if(C[uHeader+i]!=B[uHeader+i])return false;
 for(uint i=84u;i<112u;i++)if(C[uHeader+i]!=B[uHeader+i])return false;
 for(uint i=132u;i<256u;i++)if(C[uHeader+i]!=0u)return false;
 W[S+15u]=267u; // stock, conservation, capacity, exact spare replay already checked
 for(uint kind=0u;kind<2u;kind++){
  uint n=kind==0u?uK:uKw,base=kind==0u?0u:uWater,g=kind==0u?10u:11u,next=C[uHeader+(kind==0u?39u:40u)],ex=C[uHeader+126u+kind];
  if(ex>1u||(next==0u)!=(ex==1u))return false;
  for(uint i=0u;i<n;i++){
   uint m=C[base+i];if(m==0u){for(uint f=0u;f<16u;f++)if(f!=g&&C[base+f*n+i]!=0u)return false;continue;}
   uint id=C[base+5u*n+i];if(id==0u||C[base+g*n+i]==0u||(next!=0u&&id>=next))return false;
   if(C[base+2u*n+i]>CQ||C[base+3u*n+i]!=0u||C[base+4u*n+i]!=(kind==0u?0u:0u-uEmitQ))return false;
   if(kind==0u){if(C[base+6u*n+i]!=1u||C[base+7u*n+i]==0u)return false;for(uint f=11u;f<16u;f++)if(C[base+f*n+i]!=0u)return false;}
   else if(C[base+10u*n+i]!=1u||C[base+12u*n+i]==0u||C[base+15u*n+i]!=0u||C[base+2u*n+i]<=C[uColumn+12u*128u+(C[base+n+i]>>20u)])return false;
   for(uint j=i+1u;j<n;j++)if(C[base+j]!=0u&&C[base+5u*n+j]==id)return false;
  }
 }
 W[S+15u]|=4u;
 // Source has no motion grant, local/total spend, bed transfer or accepted angle change.
 // Exact unchanged operands prove zero budget and guard deltas, including mixed/static stock.
 for(uint c=0u;c<128u;c++){
  if(C[uColumn+c]!=0u||C[uColumn+128u+c]!=0u||C[uColumn+3u*128u+c]!=0u||C[uColumn+4u*128u+c]!=0u||C[uColumn+5u*128u+c]!=0u||C[uColumn+9u*128u+c]!=0u||C[uColumn+10u*128u+c]!=C[uColumn+2u*128u+c])return false;
  for(uint f=0u;f<16u;f++)if(f!=6u&&f!=7u&&f!=8u&&f!=12u&&f!=13u&&B[uColumn+f*128u+c]!=C[uColumn+f*128u+c])return false;
 }
 for(uint f=45u;f<=46u;f++)if(B[uHeader+f]!=C[uHeader+f])return false;
 for(uint f=68u;f<=71u;f++)if(B[uHeader+f]!=C[uHeader+f])return false;
 for(uint f=116u;f<=125u;f++)if(B[uHeader+f]!=C[uHeader+f])return false;
 W[S+15u]|=96u;
 uint prefix=C[uHeader+33u],events=C[uHeader+34u],successes=0u;
 if(prefix>4096u||events>prefix||C[uHeader+74u]!=prefix||C[uHeader+75u]!=0u)return false;
 uvec2 sumS=uvec2(0u),sumW=uvec2(0u);
 for(uint i=0u;i<prefix;i++){
  uint a=DEDUP+64u*i,t=a+32u;uint status=W[t],q=W[t+3u],kind=W[a+2u];
  if(W[a]!=1129534023u||W[a+1u]!=2u||W[a+4u]!=C[uHeader+4u]||W[a+5u]!=C[uHeader+5u]||W[t+2u]!=0u||status<1u||status>3u||W[t+31u]>3u)return false;
  // All terminal keys are unique; a conflicting or replayed request never appends.
  for(uint j=0u;j<i;j++)if(W[DEDUP+64u*j+6u]==W[a+6u]&&W[DEDUP+64u*j+7u]==W[a+7u])return false;
  if(status==3u){if(q!=0u||W[t+14u]!=0u||W[t+15u]!=0u||W[t+10u]!=successes||W[t+11u]!=0u||W[t+12u]!=successes||W[t+13u]!=0u) return false;continue;}
  if(q==0u||kind>1u||W[t+14u]!=successes+1u||W[t+15u]!=0u||successes>=events)return false;
  if(W[t+10u]!=successes+1u||W[t+11u]!=0u||W[t+12u]!=successes+1u||W[t+13u]!=0u||grsCompare64(sumS,uvec2(W[t+16u],W[t+17u]))!=0||grsCompare64(sumW,uvec2(W[t+20u],W[t+21u]))!=0)return false;
  if(kind==0u){if(!grsAdd64(sumS,uvec2(q,0u),sumS))return false;}
  else if(!grsAdd64(sumW,uvec2(q,0u),sumW))return false;
  if(grsCompare64(sumS,uvec2(W[t+18u],W[t+19u]))!=0||grsCompare64(sumW,uvec2(W[t+22u],W[t+23u]))!=0||grsCompare64(sumS,uvec2(W[t+24u],W[t+25u]))!=0||grsCompare64(sumW,uvec2(W[t+26u],W[t+27u]))!=0)return false;
  uint e=sourceEvents()+successes*16u,x,y;
  if(!grsFloorBinary64Q20(uvec2(W[a+16u],W[a+17u]),134217727u,x)||!grsFloorBinary64Q20(uvec2(W[a+18u],W[a+19u]),CQ,y))return false;
  uint v[16]=uint[16](W[a+6u],W[a+7u],W[t+14u],W[t+15u],kind,W[t+5u],q,W[t+7u],W[t+6u],W[a+14u],x,y,W[t+8u],W[t+9u],0u,0u);
  for(uint j=0u;j<16u;j++)if(W[e+j]!=v[j])return false;
  successes++;
 }
 if(successes!=events||C[uHeader+14u]!=successes||C[uHeader+15u]!=0u||C[uHeader+16u]!=successes||C[uHeader+17u]!=0u||grsCompare64(sumS,pairAt(false,uHeader+60u))!=0||grsCompare64(sumW,pairAt(false,uHeader+62u))!=0)return false;
 W[S+15u]|=128u;
 // Every live packet has a real accepted terminal of its own phase/namespace,
 // source ID, original position and consecutive checked ID interval.
 for(uint kind=0u;kind<2u;kind++){
  uint n=kind==0u?uK:uKw,base=kind==0u?0u:uWater;
  for(uint i=0u;i<n;i++)if(C[base+i]!=0u){
   uint origin=kind==0u?8u:6u,src=kind==0u?7u:12u;bool found=false;
   for(uint j=0u;j<prefix;j++){
    uint a=DEDUP+64u*j,t=a+32u;if(W[a+2u]!=kind||W[t+3u]==0u||W[t+14u]!=C[base+origin*n+i]||W[t+15u]!=C[base+(origin+1u)*n+i])continue;
    uint first=W[t+7u],births=W[t+6u],id=C[base+5u*n+i];if(first==0u||births==0u||id<first||id-first>=births||W[a+14u]!=C[base+src*n+i])return false;
    uint rank=id-first,expected=kind==0u?min(PACKAGE,W[t+3u]-rank*PACKAGE):W[t+3u];
    uint x,y;if(!grsFloorBinary64Q20(uvec2(W[a+16u],W[a+17u]),134217727u,x)||!grsFloorBinary64Q20(uvec2(W[a+18u],W[a+19u]),CQ,y)||C[base+n+i]!=x||C[base+2u*n+i]!=y)return false;
   if(kind==1u&&(C[base+8u*n+i]!=W[t+12u]||C[base+9u*n+i]!=W[t+13u]||C[base+13u*n+i]!=0u||C[base+14u*n+i]!=0u))return false;
   if(C[base+i]!=expected)return false;found=true;break;
   }if(!found)return false;
  }
 }
 W[S+15u]|=16u;return W[S+15u]==511u;
}
bool auditPublication(){
 if(W[S]==4u)return false;
 uint sandRank=0u,waterRank=0u;
 for(uint i=0u;i<uK;i++)if(C[i]!=0u){uint s=sandRank++;uint v[16]=uint[16](C[5u*uK+i],C[10u*uK+i],C[7u*uK+i],C[i],C[uK+i],C[2u*uK+i],C[3u*uK+i],C[4u*uK+i],grsColumnPreservingFloatBits(C[uK+i]),grsQ20FloatBits(C[2u*uK+i]),0x3fc00000u,C[uK+i]>>20u,C[6u*uK+i]&15u,0u,C[8u*uK+i],C[9u*uK+i]);for(uint f=0u;f<16u;f++)if(E[f*uK+s]!=v[f])return false;}
 for(uint i=sandRank;i<uK;i++)for(uint f=0u;f<16u;f++)if(E[f*uK+i]!=0u)return false;
 for(uint i=0u;i<uKw;i++)if(C[uWater+i]!=0u){uint s=waterRank++;for(uint f=0u;f<16u;f++)if(E[pubWater()+f*uKw+s]!=C[uWater+f*uKw+i])return false;if(E[pubWater()+16u*uKw+s]!=grsColumnPreservingFloatBits(C[uWater+uKw+i])||E[pubWater()+17u*uKw+s]!=grsQ20FloatBits(C[uWater+2u*uKw+i])||E[pubWater()+18u*uKw+s]!=1u||E[pubWater()+19u*uKw+s]!=0u)return false;}
 for(uint i=waterRank;i<uKw;i++)for(uint f=0u;f<20u;f++)if(E[pubWater()+f*uKw+i]!=0u)return false;
 for(uint c=0u;c<128u;c++){uint s=C[uColumn+10u*128u+c],w=C[uColumn+6u*128u+c],j=C[uColumn+7u*128u+c],p=C[uColumn+8u*128u+c],eta=s+w,r=eta+j+p;uint v[16]=uint[16](s,w,eta,j,p,C[uColumn+c],C[uColumn+2u*128u+c],r,grsQ20FloatBits(s),grsQ20FloatBits(eta),grsQ20FloatBits(w),grsQ20FloatBits(r),w>0u?1u:0u,c,0u,0u);for(uint f=0u;f<16u;f++)if(E[pubColumn()+f*128u+c]!=v[f])return false;}
 uint h=pubHeader();if(E[h]!=1347637831u||E[h+1u]!=2u||E[h+2u]!=256u||E[h+3u]!=3235823838u||E[h+31u]!=1u-uBaseBank||E[h+32u]!=C[uHeader+72u]||E[h+33u]!=C[uHeader+73u]||E[h+34u]!=sandRank||E[h+35u]!=waterRank||E[h+36u]!=128u||E[h+37u]!=uFrame.x||E[h+38u]!=uFrame.y||E[h+39u]!=C[uHeader+72u]||E[h+40u]!=C[uHeader+73u])return false;
 for(uint i=0u;i<8u;i++)if(E[h+4u+i]!=C[uHeader+4u+i])return false;for(uint i=0u;i<19u;i++)if(E[h+12u+i]!=C[uHeader+14u+i])return false;for(uint i=0u;i<24u;i++)if(E[h+41u+i]!=C[uHeader+88u+i])return false;for(uint i=65u;i<256u;i++)if(E[h+i]!=0u)return false;
 W[S+15u]|=512u;return W[S+15u]==1023u;
}

