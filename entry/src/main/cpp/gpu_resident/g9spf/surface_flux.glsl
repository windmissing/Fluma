// Face records are frozen before any apply. Both columns consume the same q
// and jp, including pressure transmission with zero transported mass.
bool g9FlowTerms(uint c,uint z,out uint f,out uvec4 p,out int velocity){
 uint original=g9Column(c,1u);f=g9Material(c)>z?g9Material(c)-z:0u;p=uvec4(0u);velocity=0;
 if(f==0u)return true;if(original==0u)return false;
 if(!g9RneProduct(g9Momentum(c),g9U32(f),original,p))return false;uvec4 u;if(!g9SDiv(p,g9U32(f),1u,u)||!g9I32(u,velocity))return false;return abs(velocity)<=1073741824;
}
bool g9Pressure(uvec4 p,uint flow,out uvec4 result){
 result=uvec4(0u);if(flow==0u)return g9Zero(p);
 uvec4 square,kinetic,pressure;if(!g9SMul(p,p,square)||!g9SDiv(square,g9U32(flow),1u,kinetic)||!g9UMul(g9U32(flow),g9U32(flow),square)||!g9SMul(square,g9U32(C[23u]),pressure)||!g9SDiv(pressure,g9U32(2u),1u,pressure)||!g9SAdd(kinetic,pressure,result))return false;return true;
}
bool g9BedCorrection(uint original,uint reconstructed,out uvec4 result){
 uvec4 a,b,difference;if(!g9UMul(g9U32(original),g9U32(original),a)||!g9UMul(g9U32(reconstructed),g9U32(reconstructed),b)||!g9USub(a,b,difference)||!g9SMul(difference,g9U32(C[23u]),difference)||!g9SDiv(difference,g9U32(2u),1u,result))return false;return true;
}
bool g9FaceFlux(){
 for(uint edge=0u;edge<129u;++edge){uint at=uFaces+edge*32u;for(uint i=0u;i<32u;++i)W[at+i]=0u;
  uint left=edge==0u?0u:edge-1u,right=edge==128u?127u:edge,z=max(g9Column(left,0u),g9Column(right,0u));
  uint fl,fr;uvec4 pl,pr;int ul,ur;
  if(!g9FlowTerms(left,z,fl,pl,ul)||!g9FlowTerms(right,z,fr,pr,ur)){g9Fault(29u,edge,0u);return false;}
  if(edge==0u){fl=fr;ul=-ur;if(!g9SNeg(pr,pl)){g9Fault(29u,edge,1u);return false;}}
  if(edge==128u){fr=fl;ur=-ul;if(!g9SNeg(pl,pr)){g9Fault(29u,edge,2u);return false;}}
  uvec4 waveL,waveR;if(!g9UMul(g9U32(C[23u]),g9U32(fl),waveL)||!g9UMul(g9U32(C[23u]),g9U32(fr),waveR)){g9Fault(29u,edge,3u);return false;}
  uint sl,sr;if(!g9CeilSqrt64(waveL.xy,sl)||!g9CeilSqrt64(waveR.xy,sr)){g9Fault(29u,edge,4u);return false;}uint speed=max(uint(abs(ul))+sl,uint(abs(ur))+sr);
  uvec4 fm,fp,al,ar,difference,viscosity;
  if(!g9SSub(g9U32(fr),g9U32(fl),difference)||!g9SMul(g9U32(speed),difference,viscosity)||!g9SAdd(pl,pr,fm)||!g9SSub(fm,viscosity,fm)||!g9SDiv(fm,g9U32(2u),1u,fm)||
     !g9Pressure(pl,fl,al)||!g9Pressure(pr,fr,ar)||!g9SSub(pr,pl,difference)||!g9SMul(g9U32(speed),difference,viscosity)||!g9SAdd(al,ar,fp)||!g9SSub(fp,viscosity,fp)||!g9SDiv(fp,g9U32(2u),1u,fp)){g9Fault(29u,edge,5u);return false;}
  uint denominator=1048576u*120u*uJ;uvec4 q,jp,cl,cr;
  if(!g9SDiv(fm,g9U32(denominator),0u,q)||!g9SDiv(fp,g9U32(denominator),1u,jp)||!g9BedCorrection(g9Column(left,1u),fl,cl)||!g9BedCorrection(g9Column(right,1u),fr,cr)||!g9SDiv(cl,g9U32(denominator),1u,cl)||!g9SDiv(cr,g9U32(denominator),1u,cr)){g9Fault(29u,edge,6u);return false;}
  int narrow;if(!g9I32(q,narrow)){g9Reject(8u,edge,0u);return false;}if(edge==0u||edge==128u)narrow=0;
  W[at]=uint(narrow);W[at+1u]=speed;W[at+2u]=fl;W[at+3u]=fr;g9WriteWide(at+4u,jp);g9WriteWide(at+8u,cl);g9WriteWide(at+12u,cr);g9WriteWide(at+16u,fm);g9WriteWide(at+20u,fp);
 }
 for(uint c=0u;c<128u;++c){uint left=uFaces+c*32u,right=left+32u;uvec4 p=g9Momentum(c),change,bed,gravity,product;
  if(!g9SSub(g9ReadWide(left+4u),g9ReadWide(right+4u),change)||!g9SSub(g9ReadWide(left+12u),g9ReadWide(right+8u),bed)||!g9SMul(g9S32(int(C[22u])),g9U32(g9Column(c,1u)),product)||!g9SDiv(product,g9U32(120u*uJ),1u,gravity)||!g9SAdd(p,change,p)||!g9SAdd(p,bed,p)||!g9SAdd(p,gravity,p)){g9Fault(29u,c,7u);return false;}
  uint stage=uColumnStage+c*64u;g9WriteWide(stage,p);g9WriteWide(stage+4u,bed);g9WriteWide(stage+8u,gravity);W[stage+12u]=g9Column(c,1u);W[stage+13u]=g9Column(c,4u);
  uvec4 limit;if(!g9UMul(g9U32(W[uConfig]),g9U32(C[23u]),product)||!g9UMul(product,g9U32(g9Column(c,1u)),product)||!g9SDiv(product,g9U32(65536u*120u*uJ),0u,limit)){g9Fault(29u,c,8u);return false;}
  W[stage+14u]=g9Column(c,4u)==0u&&g9UCmp(g9Magnitude(p),limit)<=0?1u:0u;
 }
 for(uint edge=1u;edge<128u;++edge)if(W[uColumnStage+(edge-1u)*64u+14u]!=0u&&W[uColumnStage+edge*64u+14u]!=0u)W[uFaces+edge*32u]=0u;
 return true;
}
// Provisional maps live in each column's 2048-word Hamilton partition. The
// original C maps stay unchanged until every donor/receiver is representable.
uint g9Provisional(uint c,uint rank){return uHamilton+c*2048u+rank*4u;}
bool g9ProvisionalAdd(uint c,uvec3 key,uint mass){
 if(mass==0u)return true;uint count=W[uHamilton+c*2048u+512u],rank=count;
 for(uint i=0u;i<count;++i){uint at=g9Provisional(c,i);uvec3 old=uvec3(W[at],W[at+1u],W[at+2u]);int cmp=g9KeyCompare(old,key);if(cmp>=0){rank=i;break;}}
 bool merge=false;if(rank<count){uint at=g9Provisional(c,rank);merge=g9KeyCompare(uvec3(W[at],W[at+1u],W[at+2u]),key)==0;}
 if(merge){uint at=g9Provisional(c,rank),next;if(!g9AddMass(W[at+3u],mass,next))return false;W[at+3u]=next;return true;}
 if(count==64u)return false;
 for(uint i=count;i>rank;--i)for(uint f=0u;f<4u;++f)W[g9Provisional(c,i)+f]=W[g9Provisional(c,i-1u)+f];uint at=g9Provisional(c,rank);W[at]=key.x;W[at+1u]=key.y;W[at+2u]=key.z;W[at+3u]=mass;W[uHamilton+c*2048u+512u]=count+1u;return true;
}
bool g9FlowReserve(){
 for(uint c=0u;c<128u;++c){uint base=uHamilton+c*2048u,count=g9MapCount(true,c);W[base+512u]=0u;
  int l=int(W[uFaces+c*32u]),r=int(W[uFaces+(c+1u)*32u]);uint outgoing=(l<0?uint(-l):0u)+(r>0?uint(r):0u);
  if(outgoing>g9Column(c,1u)){g9Reject(8u,c,0u);return false;}
  uint incoming=(l>0?uint(l):0u)+(r<0?uint(-r):0u);if(incoming>268435456u-g9Occupied(c)+outgoing){g9Reject(8u,c,1u);return false;}
  // Remaining donor maps retain their exact source keys for left/right split.
  for(uint i=0u;i<64u;++i)for(uint f=0u;f<4u;++f)W[base+1024u+i*4u+f]=C[g9MapAddress(true,c,i)+f];W[base+1536u]=count;W[base+1537u]=g9Column(c,1u);
 }
 // Reserve Hamilton shares from remaining donor sources, left face first.
 for(uint donor=0u;donor<128u;++donor){uint base=uHamilton+donor*2048u,count=W[base+1536u];
  for(uint side=0u;side<2u;++side){int signedQ=int(W[uFaces+(donor+side)*32u]);uint q=side==0u?(signedQ<0?uint(-signedQ):0u):(signedQ>0?uint(signedQ):0u);if(q==0u)continue;
   uint receiver=side==0u?donor-1u:donor+1u,total=W[base+1537u],sum=0u;
   for(uint i=0u;i<count;++i){uint at=base+1024u+i*4u,mass=W[at+3u];uvec4 product,quotient,remainder;
    if(!g9UMul(g9U32(q),g9U32(mass),product)||!g9UDiv(product,g9U32(total),quotient,remainder)){g9Fault(29u,donor,i);return false;}W[base+256u+i*4u]=quotient.x;W[base+256u+i*4u+1u]=remainder.x;W[base+256u+i*4u+2u]=0u;sum+=quotient.x;
   }
   for(uint n=0u;n<q-sum;++n){uint best=count;for(uint i=0u;i<count;++i)if(W[base+256u+i*4u+2u]==0u&&(best==count||W[base+256u+i*4u+1u]>W[base+256u+best*4u+1u]))best=i;
    if(best==count){g9Fault(24u,donor,n);return false;}W[base+256u+best*4u]++;W[base+256u+best*4u+2u]=1u;
   }
   for(uint i=0u;i<count;++i){uint at=base+1024u+i*4u,share=W[base+256u+i*4u];if(share>W[at+3u]){g9Fault(24u,donor,i);return false;}
    if(!g9ProvisionalAdd(receiver,uvec3(W[at],W[at+1u],W[at+2u]),share)){g9Reject(11u,receiver,i);return false;}W[at+3u]-=share;
   }W[base+1537u]-=q;
  }
  for(uint i=0u;i<count;++i){uint at=base+1024u+i*4u;if(!g9ProvisionalAdd(donor,uvec3(W[at],W[at+1u],W[at+2u]),W[at+3u])){g9Reject(11u,donor,i);return false;}}
 }return true;
}
bool g9FlowApply(){
 // All source maps/face operands have been preflighted before the first write.
 for(uint edge=1u;edge<128u;++edge){uint at=uFaces+edge*32u;int q=int(W[at]);uvec4 jp=g9ReadWide(at+4u);if(q==0&&g9Zero(jp))continue;
  uint row[64];g9Event(row,G9_EVENT_EDGE_FLUX,edge,edge-1u,edge);row[14]|=4u;row[15]=uint(abs(q));row[16]=q<0?1u:0u;row[17]=(W[uColumnStage+(edge-1u)*64u+14u]!=0u?1u:0u)|(W[uColumnStage+edge*64u+14u]!=0u?2u:0u);
  row[18]=g9Column(edge-1u,1u);row[19]=g9Column(edge,1u);row[20]=g9Column(edge-1u,0u);row[21]=g9Column(edge,0u);row[22]=W[at+2u];row[23]=W[at+3u];row[24]=W[at+1u];
  for(uint i=0u;i<4u;++i){row[25u+i]=W[at+16u+i];row[29u+i]=W[at+20u+i];}row[33]=jp.x;row[34]=jp.y;row[35]=W[at+8u];row[36]=W[at+9u];row[37]=W[at+12u];row[38]=W[at+13u];row[39]=g9MapCount(true,edge-1u);row[40]=g9MapCount(true,edge);if(!g9OneEvent(row))return false;
 }
 for(uint c=0u;c<128u;++c){uint base=uHamilton+c*2048u,count=W[base+512u],oldF=g9Column(c,1u),sum=0u;
  for(uint i=0u;i<64u;++i){uint src=g9Provisional(c,i),dst=g9MapAddress(true,c,i);for(uint f=0u;f<4u;++f)C[dst+f]=i<count?W[src+f]:0u;if(i<count)sum+=W[src+3u];}
  uvec4 p=g9ReadWide(uColumnStage+c*64u),gravity=g9ReadWide(uColumnStage+c*64u+8u),bed=g9ReadWide(uColumnStage+c*64u+4u),oldP=g9Momentum(c);
  if(sum==0u&&!g9Zero(p)){g9Reject(7u,c,0u);return false;}uvec4 bound;if(!g9UMul(g9U32(sum),g9U32(1073741824u),bound)){g9Fault(29u,c,0u);return false;}if(g9UCmp(g9Magnitude(p),bound)>0){g9Reject(6u,c,0u);return false;}
  g9SetMapCount(true,c,count);g9PutColumn(c,1u,sum);if(!g9PutMomentum(c,p)||!g9Accumulate(50u,gravity)||!g9Accumulate(74u,bed))return false;g9Derived(c);
  // The two container walls contribute their actual shared pressure impulses.
  uvec4 wall=uvec4(0u);if(c==0u)wall=g9ReadWide(uFaces+4u);if(c==127u&&!g9SNeg(g9ReadWide(uFaces+128u*32u+4u),wall)){g9Fault(29u,c,1u);return false;}if(!g9Accumulate(74u,wall))return false;
  if(!g9Zero(p)||!g9Zero(oldP)||!g9Zero(gravity)||!g9Zero(bed)||!g9Zero(wall)){uint row[64];g9Event(row,G9_EVENT_MOMENTUM_FORCE,c,c,c);row[15]=oldF;row[16]=sum;row[17]=W[uColumnStage+c*64u+13u];row[18]=oldP.x;row[19]=oldP.y;row[20]=p.x;row[21]=p.y;
   uvec4 left=g9ReadWide(uFaces+c*32u+4u),right=g9ReadWide(uFaces+(c+1u)*32u+4u);row[22]=left.x;row[23]=left.y;row[24]=right.x;row[25]=right.y;row[26]=gravity.x;row[27]=gravity.y;row[28]=bed.x;row[29]=bed.y;row[30]=wall.x;row[31]=wall.y;if(!g9OneEvent(row))return false;}
 }return true;
}
