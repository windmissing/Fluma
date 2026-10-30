// Full M1 SOURCE: prior packets may have moved, partially landed or retired.
// The existing source replay proves unchanged old packets and exact new births.
bool auditSourceDomains(){
 if(!audit(false)||W[S+12u]!=1u)return false;
 uvec2 physical;if(!grsAdd64(pairAt(false,uHeader+14u),pairAt(false,uHeader+118u),physical)||physical!=pairAt(false,uHeader+16u))return false;
 for(uint i=24u;i<=32u;i++)if(C[uHeader+i]!=B[uHeader+i])return false;for(uint i=84u;i<112u;i++)if(C[uHeader+i]!=B[uHeader+i])return false;for(uint i=132u;i<256u;i++)if(C[uHeader+i]!=0u)return false;
 if(any(notEqual(pairAt(false,uHeader+64u),uvec2(0u)))||any(notEqual(pairAt(false,uHeader+66u),uvec2(0u))))return false;
 W[S+15u]=267u;
 for(uint kind=0u;kind<2u;kind++){uint n=kind==0u?uK:uKw,base=kind==0u?0u:uWater,g=kind==0u?10u:11u,next=C[uHeader+39u+kind],ex=C[uHeader+126u+kind];if(ex>1u||(kind==0u?((next==0u)!=(ex==1u)):(next==0u||(ex==1u&&next!=0xffffffffu))))return false;
  for(uint i=0u;i<n;i++){uint m=C[base+i];if(m==0u){for(uint f=0u;f<16u;f++)if(f!=g&&C[base+f*n+i]!=0u)return false;continue;}uint id=C[base+5u*n+i],t=C[base+(kind==0u?6u:10u)*n+i];int vx=int(C[base+3u*n+i]),vy=int(C[base+4u*n+i]);if(m>CQ||C[base+n+i]>=134217728u||C[base+2u*n+i]>CQ||vx< -1073741824||vx>1073741824||vy< -1073741824||vy>1073741824||(kind==1u&&vy>0)||id==0u||C[base+g*n+i]==0u||(next!=0u&&id>=next&&!(kind==1u&&ex==1u&&id==0xffffffffu))||(t&15u)<1u||(t&15u)>3u||(t&0xffff0ff0u)!=0u||(t>>12u)>6u)return false;
   if(kind==0u){if(C[base+7u*n+i]==0u||C[base+11u*n+i]!=(t>>12u)||C[base+12u*n+i]!=0u||C[base+13u*n+i]!=0u||C[base+14u*n+i]>1u)return false;}
   else if(C[base+12u*n+i]==0u||C[base+15u*n+i]!=0u||grsCompare64(uvec2(C[base+13u*n+i],C[base+14u*n+i]),pairAt(false,uHeader+118u))>0)return false;
   for(uint j=i+1u;j<n;j++)if(C[base+j]!=0u&&C[base+5u*n+j]==id)return false;
  }
 }W[S+15u]|=4u;
 for(uint c=0u;c<128u;c++){if(C[uColumn+c]!=0u||C[uColumn+128u+c]!=0u||C[uColumn+3u*128u+c]!=0u||C[uColumn+4u*128u+c]!=0u||C[uColumn+5u*128u+c]!=0u||C[uColumn+9u*128u+c]!=0u||C[uColumn+10u*128u+c]!=C[uColumn+2u*128u+c])return false;for(uint f=0u;f<16u;f++)if(f!=6u&&f!=7u&&f!=8u&&f!=12u&&f!=13u&&C[uColumn+f*128u+c]!=B[uColumn+f*128u+c])return false;}
 for(uint i=45u;i<=46u;i++)if(C[uHeader+i]!=B[uHeader+i])return false;for(uint i=68u;i<=71u;i++)if(C[uHeader+i]!=B[uHeader+i])return false;for(uint i=116u;i<=125u;i++)if(C[uHeader+i]!=B[uHeader+i])return false;for(uint i=128u;i<=131u;i++)if(C[uHeader+i]!=B[uHeader+i])return false;W[S+15u]|=96u;
 uint prefix=C[uHeader+33u],events=C[uHeader+34u],successes=0u;if(prefix>4096u||events>prefix||C[uHeader+74u]!=prefix||C[uHeader+75u]!=0u)return false;uvec2 sumS=uvec2(0u),sumW=uvec2(0u),previousPhysics=uvec2(0u);
 for(uint i=0u;i<prefix;i++){uint a=DEDUP+64u*i,t=a+32u,status=W[t],q=W[t+3u],kind=W[a+2u];if(W[a]!=1129534023u||W[a+1u]!=2u||W[a+4u]!=C[uHeader+4u]||W[a+5u]!=C[uHeader+5u]||W[t+2u]!=0u||status<1u||status>3u||W[t+31u]>3u)return false;for(uint j=0u;j<i;j++)if(W[DEDUP+64u*j+6u]==W[a+6u]&&W[DEDUP+64u*j+7u]==W[a+7u])return false;
  uvec2 physics=uvec2(W[t+12u],W[t+13u]);if(grsCompare64(physics,previousPhysics)<0||grsCompare64(physics,pairAt(false,uHeader+16u))>0)return false;
  if(status==3u){if(q!=0u||W[t+14u]!=0u||W[t+15u]!=0u||W[t+10u]!=successes||W[t+11u]!=0u)return false;previousPhysics=physics;continue;}
  if(q==0u||kind>1u||successes>=events||W[t+14u]!=successes+1u||W[t+15u]!=0u||W[t+10u]!=successes+1u||W[t+11u]!=0u||grsCompare64(physics,previousPhysics)<=0)return false;
  if(grsCompare64(sumS,uvec2(W[t+16u],W[t+17u]))!=0||grsCompare64(sumW,uvec2(W[t+20u],W[t+21u]))!=0)return false;if(kind==0u){if(!grsAdd64(sumS,uvec2(q,0u),sumS))return false;}else if(!grsAdd64(sumW,uvec2(q,0u),sumW))return false;
  if(grsCompare64(sumS,uvec2(W[t+18u],W[t+19u]))!=0||grsCompare64(sumW,uvec2(W[t+22u],W[t+23u]))!=0||grsCompare64(sumS,uvec2(W[t+24u],W[t+25u]))!=0||grsCompare64(sumW,uvec2(W[t+26u],W[t+27u]))!=0)return false;
  uint e=sourceEvents()+successes*16u,x,y;if(!grsFloorBinary64Q20(uvec2(W[a+16u],W[a+17u]),134217727u,x)||!grsFloorBinary64Q20(uvec2(W[a+18u],W[a+19u]),CQ,y))return false;uint v[16]=uint[16](W[a+6u],W[a+7u],W[t+14u],W[t+15u],kind,W[t+5u],q,W[t+7u],W[t+6u],W[a+14u],x,y,W[t+8u],W[t+9u],0u,0u);for(uint j=0u;j<16u;j++)if(W[e+j]!=v[j])return false;successes++;previousPhysics=physics;
 }
 if(successes!=events||C[uHeader+14u]!=successes||C[uHeader+15u]!=0u||grsCompare64(sumS,pairAt(false,uHeader+60u))!=0||grsCompare64(sumW,pairAt(false,uHeader+62u))!=0)return false;W[S+15u]|=128u;
 for(uint kind=0u;kind<2u;kind++){uint n=kind==0u?uK:uKw,base=kind==0u?0u:uWater;for(uint i=0u;i<n;i++)if(C[base+i]!=0u){uint origin=kind==0u?8u:6u,src=kind==0u?7u:12u,success=C[base+origin*n+i];if(success==0u||success>events||C[base+(origin+1u)*n+i]!=0u)return false;uint e=sourceEvents()+(success-1u)*16u,first=W[e+7u],births=W[e+8u],id=C[base+5u*n+i];if(W[e+4u]!=kind||W[e+9u]!=C[base+src*n+i]||first==0u||births==0u||id<first||id-first>=births)return false;uint original=kind==0u?min(PACKAGE,W[e+6u]-(id-first)*PACKAGE):W[e+6u];if(C[base+i]>original)return false;
  if(kind==1u){bool found=false;for(uint j=0u;j<prefix;j++){uint t=DEDUP+j*64u+32u;if(W[t+14u]==success&&W[t+15u]==0u){if(C[base+8u*n+i]!=W[t+12u]||C[base+9u*n+i]!=W[t+13u])return false;found=true;break;}}if(!found)return false;}
 }}W[S+15u]|=16u;return W[S+15u]==511u;
}
