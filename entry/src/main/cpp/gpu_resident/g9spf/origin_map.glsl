// Bed: bottom-to-top stack. Flow: sorted uniformly mixed sparse map.
uint g9MapAddress(bool flow,uint column,uint rank){return (flow?uFlow:uBed)+(column*64u+rank)*4u;}
uvec3 g9Key(bool flow,uint column,uint rank){uint at=g9MapAddress(flow,column,rank);return uvec3(C[at],C[at+1u],C[at+2u]);}
int g9KeyCompare(uvec3 a,uvec3 b){if(a.y!=b.y)return a.y<b.y?-1:1;if(a.x!=b.x)return a.x<b.x?-1:1;return a.z==b.z?0:a.z<b.z?-1:1;}
bool g9KeyValid(uvec3 key){if(key.x==0u||key.x>C[19u]||key.y!=0u||key.z==0u)return false;uint at=uSources+(key.x-1u)*16u;return C[at]==key.x&&C[at+1u]==0u&&C[at+2u]==key.z;}
uint g9MapCount(bool flow,uint c){return g9Column(c,flow?7u:6u);}
void g9SetMapCount(bool flow,uint c,uint n){g9PutColumn(c,flow?7u:6u,n);}
bool g9MapCanAdd(bool flow,uint c,uvec3 key){
 uint count=g9MapCount(flow,c);if(count>64u)return false;
 if(!flow)return count==0u||g9KeyCompare(g9Key(false,c,count-1u),key)==0||count<64u;
 for(uint i=0u;i<count;++i)if(g9KeyCompare(g9Key(true,c,i),key)==0)return true;return count<64u;
}
bool g9MapAdd(bool flow,uint c,uvec3 key,uint q){
 if(q==0u||!g9KeyValid(key)||!g9MapCanAdd(flow,c,key))return false;
 uint count=g9MapCount(flow,c),rank=count;
 if(!flow&&count>0u&&g9KeyCompare(g9Key(false,c,count-1u),key)==0)rank=count-1u;
 if(flow)for(uint i=0u;i<count;++i){int cmp=g9KeyCompare(g9Key(true,c,i),key);if(cmp>=0){rank=i;break;}}
 bool merge=rank<count&&g9KeyCompare(g9Key(flow,c,rank),key)==0;
 if(merge){uint at=g9MapAddress(flow,c,rank),next;if(!g9AddMass(C[at+3u],q,next))return false;C[at+3u]=next;return true;}
 for(uint i=count;i>rank;--i)for(uint f=0u;f<4u;++f)C[g9MapAddress(flow,c,i)+f]=C[g9MapAddress(flow,c,i-1u)+f];
 uint at=g9MapAddress(flow,c,rank);C[at]=key.x;C[at+1u]=key.y;C[at+2u]=key.z;C[at+3u]=q;g9SetMapCount(flow,c,count+1u);return true;
}
bool g9MapTake(bool flow,uint c,uint rank,uint q){
 uint count=g9MapCount(flow,c);if(rank>=count||count>64u||q==0u)return false;
 uint at=g9MapAddress(flow,c,rank),mass=C[at+3u];if(q>mass)return false;
 if(q<mass){C[at+3u]=mass-q;return true;}
 for(uint i=rank;i+1u<count;++i)for(uint f=0u;f<4u;++f)C[g9MapAddress(flow,c,i)+f]=C[g9MapAddress(flow,c,i+1u)+f];
 for(uint f=0u;f<4u;++f)C[g9MapAddress(flow,c,count-1u)+f]=0u;g9SetMapCount(flow,c,count-1u);return true;
}
// Preflight a whole S->F conversion in the already-budgeted Hamilton scratch.
bool g9RecruitPreflight(uint c,uint q){
 uint bed=g9Column(c,0u),depth=g9MapCount(false,c),flowCount=g9MapCount(true,c);if(q>bed||depth>64u||flowCount>64u)return false;
 uint newCount=flowCount;
 for(uint n=0u;n<depth&&q>0u;++n){uint rank=depth-1u-n;uvec3 key=g9Key(false,c,rank);bool found=false;
  for(uint i=0u;i<flowCount;++i)if(g9KeyCompare(key,g9Key(true,c,i))==0)found=true;
  for(uint i=0u;i<n&&!found;++i)if(g9KeyCompare(key,g9Key(false,c,depth-1u-i))==0)found=true;
  if(!found&&++newCount>64u)return false;uint amount=min(q,C[g9MapAddress(false,c,rank)+3u]);q-=amount;
 }return q==0u;
}
bool g9RecruitMap(uint c,uint q){
 if(!g9RecruitPreflight(c,q))return false;
 uint left=q;for(uint i=0u;i<64u&&left>0u;++i){uint rank=g9MapCount(false,c)-1u;uvec3 key=g9Key(false,c,rank);uint amount=min(left,C[g9MapAddress(false,c,rank)+3u]);
  if(!g9MapTake(false,c,rank,amount)||!g9MapAdd(true,c,key,amount))return false;left-=amount;
 }return left==0u;
}
bool g9DepositPreflight(uint c){
 uint depth=g9MapCount(false,c),count=g9MapCount(true,c);if(depth>64u||count>64u)return false;
 uvec3 top=depth==0u?uvec3(0u):g9Key(false,c,depth-1u);
 for(uint i=0u;i<count;++i){uvec3 key=g9Key(true,c,i);if(depth==0u||g9KeyCompare(top,key)!=0){if(depth==64u)return false;++depth;top=key;}}return true;
}
bool g9DepositMap(uint c){
 if(!g9DepositPreflight(c))return false;
 uint count=g9MapCount(true,c);for(uint i=0u;i<count;++i){uint at=g9MapAddress(true,c,i);if(!g9MapAdd(false,c,g9Key(true,c,i),C[at+3u]))return false;}
 for(uint i=0u;i<64u*4u;++i)C[uFlow+c*256u+i]=0u;g9SetMapCount(true,c,0u);return true;
}
// Left then right callers consume the remaining map, never the original M.
bool g9Hamilton(uint donor,uint q,uint scratch){
 uint mass=g9Column(donor,1u),count=g9MapCount(true,donor);if(q>mass||mass==0u||count>64u)return false;uint sum=0u;
 for(uint i=0u;i<count;++i){uint at=g9MapAddress(true,donor,i);uvec4 product,quotient,remainder;
  if(!g9UMul(g9U32(q),g9U32(C[at+3u]),product)||!g9UDiv(product,g9U32(mass),quotient,remainder))return false;
  W[scratch+i*4u]=quotient.x;W[scratch+i*4u+1u]=remainder.x;W[scratch+i*4u+2u]=0u;sum+=quotient.x;
 }
 for(uint n=0u;n<q-sum;++n){uint best=count;for(uint i=0u;i<count;++i)if(W[scratch+i*4u+2u]==0u&&(best==count||W[scratch+i*4u+1u]>W[scratch+best*4u+1u]))best=i;
  if(best==count)return false;W[scratch+best*4u]++;W[scratch+best*4u+2u]=1u;
 }return true;
}
