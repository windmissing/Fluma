// T907 dry expansion. The immutable angle table starts after the parent WORK.
uint g15TableBase(){return uDmBase+6144u;}
uint g15Tick(uint micro,bool middle){
 uint accepted=dmReadCandidateWord(12u);
 return min(7680u,min(accepted,120u)*64u+min(micro,32u)*(64u/uJ)+(middle?32u/uJ:0u));
}
bool g15Pose(uint tick,out int angle,out int sine,out int cosine){
 int start=int(W[uInput+34u]),end=int(W[uInput+35u]);
 angle=0;sine=0;cosine=1048576;
 if(start< -11520||start>11520||end< -11520||end>11520||tick>7680u)return false;
 int delta=end-start;
 uint size=uint(abs(delta)),d=7680u,m=d-tick;
 if(15u*size>8u*85u*256u)return false;
 uvec4 n,den,q;
 if(!g9UMul(g9U32(tick*tick),g9U32(tick),n)||
    !g9UMul(n,g9U32(d*d+3u*d*m+6u*m*m),n)||
    !g9SMul(n,g9S32(delta),n)||!g9UMul(g9U32(d*d),g9U32(d*d),den)||!g9UMul(den,g9U32(d),den)||
    !g9SMul(g9S32(start),den,q)||!g9SAdd(n,q,n)||
    !g9SDiv(n,den,1u,q)||!g9I32(q,angle))return false;
 if(angle< -11520||angle>11520)return false;
 uint at=g15TableBase()+2u*uint(angle+11520);
 sine=int(W[at]);cosine=int(W[at+1u]);
 return abs(sine)<=741455&&cosine>=741455&&cosine<=1048576;
}
bool g15Support(uint edge,out int64_t drive,out int64_t normal){
 int64_t gx=int(C[22u]),gn=C[23u],dx=1048576l;
 int64_t slope=int64_t(g9Material(edge+1u))-int64_t(g9Material(edge));
 drive=gx*dx-gn*slope;normal=gn*dx+gx*slope;
 if(abs(slope)*5l>3l*dx){g9Reject(G9_REASON_SUPPORT_MOVED_DEFER,edge,1u);return false;}
 if(normal*4l<int64_t(W[uConfig+7u])*dx){g9Reject(G9_REASON_SUPPORT_MOVED_DEFER,edge,2u);return false;}
 return true;
}
bool g15StaticFace(uint edge){
 int64_t drive,normal;
 if(!g15Support(edge,drive,normal))return false;
 uint64_t magnitude=uint64_t(drive<0l?-drive:drive);
 uvec4 d,n;
 if(!g9UMul(g9U64(uvec2(uint(magnitude),uint(magnitude>>32))),g9U32(65536u),d)||
    !g9UMul(g9U64(uvec2(uint(normal),uint(uint64_t(normal)>>32))),g9U32(W[uConfig]),n)){
  g9Fault(29u,edge,6u);return false;
 }
 return g9UCmp(d,n)<=0;
}
bool g15SupportAll(){
 for(uint edge=0u;edge<127u;++edge){
  int64_t drive,normal;
  if(!g15Support(edge,drive,normal))return false;
 }
 return true;
}
bool g15Geometry(uint c,out uvec4 chi,out uvec4 hg,out uvec4 og){
 uint b=g9Material(c),maximum=0u;
 for(uint side=0u;side<2u;++side){
  if((side==0u&&c==0u)||(side==1u&&c==127u))continue;
  int64_t drive,normal;
  if(!g15Support(side==0u?c-1u:c,drive,normal))return false;
  if(side==0u)drive=-drive;
  if(drive<=0l)continue;
  uvec4 scaled,limit,den=g9U64(uvec2(uint(normal),uint(uint64_t(normal)>>32))),q,rem;
  if(!g9UMul(g9U64(uvec2(uint(drive),uint(uint64_t(drive)>>32))),g9U32(65536u),scaled)||
     !g9UMul(den,g9U32(W[uConfig]),limit))return false;
  if(g9UCmp(scaled,limit)>0){
   if(!g9USub(scaled,limit,scaled)||!g9UMul(scaled,g9U32(16u),scaled)||
      !g9UDiv(scaled,den,q,rem)||any(notEqual(q.yzw,uvec3(0u))))return false;
   maximum=max(maximum,q.x);
  }
 }
 chi=g9U32(maximum);
 if(!dmFloorMul(g9U32(dmParam(0u)),chi,16u,hg)||!dmFloorMul(g9U32(dmParam(4u)),chi,20u,og))return false;
 if(g9UCmp(hg,g9U32(b))>0)hg=g9U32(b);return true;
}
uint g15FaceDepth(int64_t n,int64_t d){
 if(n<=0l)return 0u;
 int64_t q=n/d,r=n%d;
 return uint(q+int64_t(r>d-r||(r==d-r&&(q&1l)!=0l)));
}
bool g15Reconstruct(uint c,uint depth,out uvec4 p,out int velocity){
 uint original=g9Column(c,1u);p=uvec4(0u);velocity=0;
 if(depth==0u)return true;if(original==0u)return false;
 if(!g9RneProduct(uOp==G9_OP_CORRECTOR_FACE?dmRead64(c,40u):g9Momentum(c),g9U32(depth),original,p))return false;
 uvec4 u;if(!g9SDiv(p,g9U32(depth),1u,u)||!g9I32(u,velocity))return false;
 return velocity>= -1073741824&&velocity<=1073741824;
}
bool g15Hydro(uint edge,uint left,uint right,out uint fl,out uint fr,out uvec4 pl,out uvec4 pr,out int ul,out int ur){
 uint gn=C[23u];if(gn==0u)return false;
 int64_t shift=edge==0u||edge==128u?0l:int64_t(int(C[22u]))*1048576l;
 int64_t l=int64_t(g9Column(left,0u))*gn,r=int64_t(g9Column(right,0u))*gn-shift,effective=max(l,r);
 fl=g15FaceDepth(l+int64_t(g9Column(left,1u))*gn-effective,int64_t(gn));
 fr=g15FaceDepth(r+int64_t(g9Column(right,1u))*gn-effective,int64_t(gn));
 return g15Reconstruct(left,fl,pl,ul)&&g15Reconstruct(right,fr,pr,ur);
}
void g15ClipDonors(){
 for(uint c=0u;c<128u;++c){
  uint l=uFaces+c*32u,r=l+32u,available=g9Column(c,1u);
  int left=int(W[l]),right=int(W[r]);
  if(left<0){uint take=min(uint(-int64_t(left)),available);W[l]=uint(-int(take));available-=take;}
  if(right>0)W[r]=min(uint(right),available);
 }
}
bool g15QuietColumn(uint c){
 uint l=uFaces+c*32u;
 return W[l]==0u&&W[l+32u]==0u;
}
uint g15Normal(uint c){
 int64_t a=int64_t(C[23u])*1048576l,b=a,d;
 if(c>0u&&!g15Support(c-1u,d,a))return 0u;
 if(c<127u&&!g15Support(c,d,b))return 0u;
 if(c==0u)a=b;if(c==127u)b=a;
 return uint((a+b)/2097152l);
}
void g15Scene(){
 uint at=g15TableBase()+46082u;
 W[at]=0x35314753u;W[at+1u]=uNonce.x;W[at+2u]=uNonce.y;W[at+3u]=C[20u];
 for(uint i=0u;i<129u;++i)W[at+4u+i]=W[uFaces+i*32u];
}
