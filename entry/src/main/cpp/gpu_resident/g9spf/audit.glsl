bool g9SumOrigin(uint origin,uvec3 key,uint mass){
 if(origin==0u||origin>C[19u]||key.x!=origin||!g9KeyValid(key))return false;
 uint at=uOriginAudit+(origin-1u)*2u;uvec4 sum;if(!g9UAdd(g9U64(uvec2(W[at],W[at+1u])),g9U32(mass),sum)||any(notEqual(sum.zw,uvec2(0u))))return false;W[at]=sum.x;W[at+1u]=sum.y;return true;
}
bool g9AuditMaps(bool flow,uint c){
 uint count=g9MapCount(flow,c),sum=0u;if(count>64u)return false;uvec3 previous=uvec3(0u);
 for(uint rank=0u;rank<64u;++rank){uint at=g9MapAddress(flow,c,rank);uvec3 key=g9Key(flow,c,rank);uint mass=C[at+3u];
  if(rank>=count){for(uint f=0u;f<4u;++f)if(C[at+f]!=0u)return false;continue;}
  if(mass==0u||mass>268435456u-sum||!g9SumOrigin(key.x,key,mass))return false;
  if(rank>0u&&(flow?g9KeyCompare(previous,key)>=0:g9KeyCompare(previous,key)==0))return false;sum+=mass;previous=key;
 }return sum==g9Column(c,flow?1u:0u);
}
bool g9AuditMassGeometry(){
 if(C[0]!=G9_STATE_MAGIC||C[1]!=1u||C[2]!=G9_PROFILE||C[24]!=uK||C[25]!=128u||C[26]!=64u||C[27]!=64u||C[19u]>4096u)return false;
 for(uint i=106u;i<256u;++i)if(C[i]!=0u)return false;
 for(uint i=0u;i<8192u;++i)W[uOriginAudit+i]=0u;
 uvec4 total=uvec4(0u),declared=uvec4(0u),nextCommit;if(!g9UAdd(g9U64(uBaseCommit),g9U32(1u),nextCommit))return false;
 for(uint c=0u;c<128u;++c){
  if(!g9AuditMaps(false,c)||!g9AuditMaps(true,c)||g9Occupied(c)>268435456u||g9Column(c,9u)!=g9Material(c))return false;
  uvec4 p=g9Momentum(c),bound;if(!g9UMul(g9U32(g9Column(c,1u)),g9U32(1073741824u),bound)||g9UCmp(g9Magnitude(p),bound)>0||g9Column(c,4u)!=(g9Zero(p)?0u:1u)||g9Column(c,1u)==0u&&(!g9Zero(p)||g9Column(c,5u)!=0u))return false;
  for(uint f=16u;f<32u;++f)if(g9Column(c,f)!=0u)return false;
  uint packets=0u;for(uint slot=0u;slot<uK;++slot)if(g9Packet(slot,0u)!=0u&&g9Packet(slot,5u)==c){uint next;if(!g9AddMass(packets,g9Packet(slot,0u),next))return false;packets=next;}
  if(packets!=g9Column(c,8u)||!g9UAdd(total,g9U32(g9Occupied(c)),total))return false;
 }
 for(uint slot=0u;slot<uK;++slot){uint at=uPacket+slot*64u,mass=C[at],generation=C[at+7u];
  if(mass==0u){for(uint f=0u;f<64u;++f)if(f!=7u&&C[at+f]!=0u)return false;continue;}
  uint c=C[at+5u],id=C[at+6u],x=C[at+1u],y2=C[at+2u];int vx=int(C[at+3u]),vy=int(C[at+4u]);uvec3 key=uvec3(C[at+8u],C[at+9u],C[at+10u]);
  if(mass>244666u||c>=128u||x>=134217728u||x>>20u!=c||y2<2u*g9Material(c)+mass||y2>536870912u-mass||id==0u||generation==0u||abs(vx)>1073741824||abs(vy)>1073741824||!g9SumOrigin(key.x,key,mass))return false;
  if(C[18u]!=0u&&id>=C[18u])return false;for(uint other=slot+1u;other<uK;++other)if(g9Packet(other,0u)!=0u&&g9Packet(other,6u)==id)return false;
  if(C[at+17u]!=id||C[at+18u]!=generation||C[at+19u]!=key.x||C[at+20u]!=key.y||C[at+21u]!=key.z||C[at+22u]>1u||C[at+25u]==0u||C[at+26u]==0u||C[at+26u]>244666u||C[at+27u]>=128u||C[at+30u]!=1u)return false;
  uvec4 birth=g9U64(uvec2(C[at+23u],C[at+24u]));if(g9Zero(birth)||g9UCmp(birth,nextCommit)>0)return false;
  if(C[at+22u]==0u&&(C[at+28u]!=0u||C[at+29u]!=0u))return false;
  if(C[at+22u]==1u&&(C[at+28u]>=64u||C[at+29u]<C[at+26u]))return false;
  for(uint f=31u;f<64u;++f)if(C[at+f]!=0u)return false;
 }
 for(uint origin=0u;origin<4096u;++origin){uint at=uSources+origin*16u;
  if(origin>=C[19u]){for(uint f=0u;f<16u;++f)if(C[at+f]!=0u)return false;continue;}
  if(C[at]!=origin+1u||C[at+1u]!=0u||C[at+2u]==0u||C[at+14u]!=1u)return false;
  uvec4 admitted=g9U64(uvec2(C[at+4u],C[at+5u])),sum=g9U64(uvec2(W[uOriginAudit+origin*2u],W[uOriginAudit+origin*2u+1u]));
  if(g9UCmp(admitted,sum)!=0||!g9UAdd(declared,admitted,declared))return false;
 }
 if(g9UCmp(total,declared)!=0||any(notEqual(total.zw,uvec2(0u))))return false;C[40u]=total.x;C[41u]=total.y;return true;
}
bool g9Cfl(){
 uvec4 maximum=uvec4(0u);
 for(uint c=0u;c<128u;++c){uint f=g9Column(c,1u);if(f==0u){if(!g9Zero(g9Momentum(c))){g9Reject(7u,c,0u);return false;}continue;}
  uvec4 speed,product;uint wave;if(!g9SDiv(g9Magnitude(g9Momentum(c)),g9U32(f),3u,speed)||!g9UMul(g9U32(C[23u]),g9U32(f),product)||!g9CeilSqrt64(product.xy,wave)||!g9UAdd(speed,g9U32(wave),speed)){g9Fault(29u,c,0u);return false;}if(g9UCmp(speed,maximum)>0)maximum=speed;
 }
 for(uint slot=0u;slot<uK;++slot)if(g9Packet(slot,0u)!=0u){uint speed=max(uint(abs(int(g9Packet(slot,3u)))),uint(abs(int(g9Packet(slot,4u)))));if(g9UCmp(g9U32(speed),maximum)>0)maximum=g9U32(speed);}
 uint g=W[uConfig+7u],extra=g/120u+uint(g%120u!=0u);if(!g9UAdd(maximum,g9U32(extra),maximum)){g9Fault(29u,0u,1u);return false;}
 if(g9UCmp(maximum,g9U32(60u*uJ*1048576u))>0){g9Reject(5u,0u,0u);return false;}return true;
}
bool g9MomentumTotal(bool base,out uvec4 px,out uvec4 py){
 px=py=uvec4(0u);
 for(uint c=0u;c<128u;++c){uint at=uColumn+c*32u;uvec4 p=g9S64(base?uvec2(B[at+2u],B[at+3u]):uvec2(C[at+2u],C[at+3u]));if(!g9SAdd(px,p,px))return false;}
 for(uint slot=0u;slot<uK;++slot){uint at=uPacket+slot*64u,mass=base?B[at]:C[at];if(mass==0u)continue;
  uvec4 x,y;if(!g9SMul(g9U32(mass),g9S32(int(base?B[at+3u]:C[at+3u])),x)||!g9SMul(g9U32(mass),g9S32(int(base?B[at+4u]:C[at+4u])),y)||!g9SAdd(px,x,px)||!g9SAdd(py,y,py))return false;
 }return true;
}
bool g9AuditMomentum(){
 uvec4 beforeX,beforeY,afterX,afterY,expectedX=uvec4(0u),expectedY=uvec4(0u),delta;
 if(!g9MomentumTotal(true,beforeX,beforeY)||!g9MomentumTotal(false,afterX,afterY))return false;
 for(uint category=0u;category<4u;++category){uint at=50u+category*8u;uvec4 before=uvec4(B[at],B[at+1u],B[at+2u],B[at+3u]);if(!g9SSub(g9HeaderWide(at),before,delta)||!g9SAdd(expectedX,delta,expectedX))return false;
  at+=4u;before=uvec4(B[at],B[at+1u],B[at+2u],B[at+3u]);if(!g9SSub(g9HeaderWide(at),before,delta)||!g9SAdd(expectedY,delta,expectedY))return false;
 }
 if(!g9SSub(afterX,beforeX,delta)||any(notEqual(delta,expectedX)))return false;
 return g9SSub(afterY,beforeY,delta)&&all(equal(delta,expectedY));
}
bool g9AuditStream(){
 if(g9Ctl(6u)!=g9Ctl(7u)||g9Ctl(8u)!=g9Ctl(9u)||g9Ctl(10u)!=0u||g9Ctl(7u)>uEventCapacity||g9Ctl(9u)>uPathCapacity)return false;
 for(uint row=0u;row<g9Ctl(7u);++row){uint at=row*64u,kind=E[at+2u];uvec2 mask=g9EventMask(kind);
  if(all(equal(mask,uvec2(0u)))||E[at]!=G9_EVENT_MAGIC||E[at+1u]!=1u||E[at+3u]!=g9Ctl(13u)||E[at+4u]!=uNonce.x||E[at+5u]!=uNonce.y||E[at+6u]!=row+1u||E[at+63u]!=G9_SEAL||(E[at+14u]&~15u)!=0u)return false;
  for(uint i=0u;i<64u;++i)if(((mask[i/32u]>>(i%32u))&1u)==0u&&E[at+i]!=0u)return false;
 }
 uint previousParent=0u,previousTau=0u;
 for(uint row=0u;row<g9Ctl(9u);++row){uint at=uPathWord+row*16u,parent=E[at],t0=E[at+4u],t1=E[at+5u];if(parent==0u||parent>g9Ctl(7u)||E[at+2u]>=uK||t0>t1||t1>65536u||E[at+15u]!=G9_SEAL||parent<previousParent||parent==previousParent&&t0<previousTau)return false;previousParent=parent;previousTau=t1;}
 return true;
}
