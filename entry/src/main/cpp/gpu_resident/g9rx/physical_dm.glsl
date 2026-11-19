// T644/T647: genuine DM48 implementation, private to the GNR1 variant.
uniform uint uDmBase;
uint dmWord(uint c,uint f){return W[uDmBase+c*48u+f];}
void dmPut(uint c,uint f,uint v){W[uDmBase+c*48u+f]=v;}
uint dmParam(uint f){return W[uConfig+76u+f];}
uvec4 dmRead64(uint c,uint f){return g9S64(uvec2(dmWord(c,f),dmWord(c,f+1u)));}
bool dmStore64(uint c,uint f,uvec4 v){uint sign=(v.y&0x80000000u)!=0u?0xffffffffu:0u;if(v.z!=sign||v.w!=sign){g9Reject(6u,c,f);return false;}dmPut(c,f,v.x);dmPut(c,f+1u,v.y);return true;}
uvec4 dmShr(uvec4 a,uint n){return uvec4((a.x>>n)|(a.y<<(32u-n)),(a.y>>n)|(a.z<<(32u-n)),(a.z>>n)|(a.w<<(32u-n)),a.w>>n);}
bool dmFloorMul(uvec4 a,uvec4 b,uint bits,out uvec4 result){uvec4 v;if(!g9UMul(a,b,v))return false;result=dmShr(v,bits);return true;}
bool dmRneMul(uvec4 a,uvec4 b,uint bits,out uvec4 result){uvec4 v;if(!g9UMul(a,b,v))return false;uint mask=(1u<<bits)-1u,midpointBit=1u<<(bits-1u),rem=v.x&mask;result=dmShr(v,bits);return rem<midpointBit||(rem==midpointBit&&(result.x&1u)==0u)||g9UAdd(result,g9U32(1u),result);}
uint dmBound(uvec4 v,uint bound){return g9UCmp(v,g9U32(bound))>=0?bound:v.x;}
struct DmEval {uint omega;uvec4 h;uvec4 hs;int velocity;bool feasible;};
bool dmEval(uint q,uvec4 p,uint f0,uint b,uvec4 hg,uvec4 og,out DmEval e){
 e.omega=0u;e.h=e.hs=uvec4(0u);e.velocity=0;e.feasible=false;uint f=f0+q;if(f<f0)return false;
 uvec4 u=uvec4(0u);if(f==0u){if(!g9Zero(p))return false;}else if(!g9SDiv(p,g9U32(f),1u,u))return false;
 if(!g9I32(u,e.velocity))return false;
 uint r=max(f,dmParam(2u));if(r==0u||C[23u]==0u)return false;
 uvec4 square,num,den,head,rem,yieldDepth,excess=uvec4(0u),hs,hSum,chiS,os,raw,deficit=uvec4(0u),lhs,rhs;
 if(!g9UMul(g9Magnitude(u),g9Magnitude(u),square)||!g9UMul(square,g9U32(W[uConfig+4u]),num)||!g9UMul(g9U32(65536u),g9U32(C[23u]),den)||!g9UDiv(num,den,head,rem)||!dmRneMul(g9U32(W[uConfig]),g9U32(r),16u,yieldDepth))return false;
 if(g9UCmp(head,yieldDepth)>0&&!g9USub(head,yieldDepth,excess))return false;
 if(!dmFloorMul(g9U32(dmParam(1u)),excess,16u,hs)||!g9UAdd(g9U32(f0),hs,hSum))return false;e.hs=hs;
 e.h=g9UCmp(hg,hSum)>0?hg:hSum;if(g9UCmp(e.h,g9U32(b))>0)e.h=g9U32(b);
 if(!g9UMul(excess,g9U32(1048576u),num)||!g9UDiv(num,g9U32(r),chiS,rem)||!dmFloorMul(g9U32(dmParam(5u)),chiS,20u,os)||!g9UAdd(g9U32(dmParam(3u)),g9UCmp(og,os)>0?og:os,raw))return false;
 e.omega=dmBound(raw,dmParam(6u));if(g9UCmp(e.h,g9U32(f))>0&&!g9USub(e.h,g9U32(f),deficit))return false;
 if(!g9UMul(g9U32(q),g9U32(120u*uJ*65536u),lhs)||!g9UMul(g9U32(e.omega),deficit,rhs))return false;e.feasible=g9UCmp(lhs,rhs)<=0;return true;
}
bool dmGeometry(uint c,out uvec4 chi,out uvec4 hg,out uvec4 og){
 uint b=g9Material(c),gn=C[23u];if(gn==0u)return false;uvec4 drive=uvec4(0u),term,d,negativeG,yield,excess=uvec4(0u),num,rem;
 if(c<127u){if(!g9RneProduct(g9U32(gn),g9S32(int(g9Material(c+1u))-int(b)),1048576u,term)||!g9SSub(g9S32(int(C[22u])),term,d))return false;if(g9SCmp(d,drive)>0)drive=d;}
 if(c>0u){if(!g9SNeg(g9S32(int(C[22u])),negativeG)||!g9RneProduct(g9U32(gn),g9S32(int(g9Material(c-1u))-int(b)),1048576u,term)||!g9SSub(negativeG,term,d))return false;if(g9SCmp(d,drive)>0)drive=d;}
 if(!dmRneMul(g9U32(W[uConfig]),g9U32(gn),16u,yield))return false;if(g9SCmp(drive,yield)>0&&!g9USub(drive,yield,excess))return false;
 if(!g9UMul(excess,g9U32(1048576u),num)||!g9UDiv(num,g9U32(gn),chi,rem)||!dmFloorMul(g9U32(dmParam(0u)),chi,16u,hg)||!dmFloorMul(g9U32(dmParam(4u)),chi,20u,og))return false;
 if(g9UCmp(hg,g9U32(b))>0)hg=g9U32(b);return true;
}
bool dmOriginPrefix(uint c,uint q,out uint result){
 result=0u;uint depth=g9MapCount(false,c),count=g9MapCount(true,c),admitted=count;
 if(depth>64u||count>64u)return false;
 for(uint n=0u;n<depth&&result<q;++n){uint rank=depth-1u-n;uvec3 key=g9Key(false,c,rank);if(!g9KeyValid(key))return false;bool found=false;
  for(uint i=0u;i<count;++i)if(g9KeyCompare(key,g9Key(true,c,i))==0)found=true;
  for(uint i=0u;i<n&&!found;++i)if(g9KeyCompare(key,g9Key(false,c,depth-1u-i))==0)found=true;
  if(!found){if(admitted==64u)break;++admitted;}uint mass=C[g9MapAddress(false,c,rank)+3u];if(mass==0u)return false;result+=min(q-result,mass);
 }return true;
}
bool dmRequest(){
 for(uint c=0u;c<128u;++c){uint prior=dmWord(c,46u)&0xffff0000u;for(uint f=0u;f<48u;++f)dmPut(c,f,0u);dmPut(c,46u,prior);uint s=g9Column(c,0u),f0=g9Column(c,1u),b=g9Material(c);uvec4 p=g9Momentum(c),chi,hg,og;
  if(!dmGeometry(c,chi,hg,og)||any(notEqual(chi.yzw,uvec3(0u))))return false;
  dmPut(c,0u,s);dmPut(c,1u,f0);if(!dmStore64(c,2u,p))return false;dmPut(c,4u,b);dmPut(c,5u,C[22u]);dmPut(c,6u,C[23u]);dmPut(c,7u,chi.x);dmPut(c,8u,hg.x);dmPut(c,24u,g9MapCount(false,c));dmPut(c,25u,g9MapCount(true,c));
  DmEval e0,ep,en;if(!dmEval(0u,p,f0,b,hg,og,e0)||!e0.feasible)return false;
  uint deficit=e0.h.x>f0?e0.h.x-f0:0u;uvec4 numerator,denominator,quotient,remainder;
  if(!g9UMul(g9U32(e0.omega),g9U32(deficit),numerator)||!g9UAdd(g9U32(120u*uJ*65536u),g9U32(e0.omega),denominator)||!g9UDiv(numerator,denominator,quotient,remainder))return false;
  if(any(notEqual(quotient.yzw,uvec3(0u))))return false;uint upper=min(s,quotient.x),lo=0u,hi=upper+1u,iterations=0u,calls=1u;bool closed=g9Zero(p)||W[uConfig+4u]==0u;
  if(closed)lo=upper;else for(uint round=0u;round<32u;++round){if(hi-lo<=1u)continue;uint mid=lo+(hi-lo)/2u;DmEval em;if(!dmEval(mid,p,f0,b,hg,og,em))return false;++calls;++iterations;if(em.feasible)lo=mid;else hi=mid;}
  if(closed)hi=upper+1u;if(hi!=lo+1u)return false;
  if(!dmEval(lo,p,f0,b,hg,og,ep)||!ep.feasible)return false;++calls;uint next=lo+1u;bool nextFeasible=false;
  if(next<=s&&next<=upper){if(!dmEval(next,p,f0,b,hg,og,en))return false;++calls;nextFeasible=en.feasible;}if(nextFeasible)return false;
  uint origin;if(!dmOriginPrefix(c,lo,origin))return false;uint actual=min(lo,origin);
  uint reason=s==0u?1u:deficit==0u?2u:e0.omega==0u?3u:lo==0u?4u:origin==0u?5u:origin<lo?6u:0u;
  dmPut(c,9u,e0.h.x);dmPut(c,10u,e0.omega);dmPut(c,11u,dmBound(e0.hs,268435456u));dmPut(c,12u,upper);dmPut(c,13u,lo);dmPut(c,14u,hi);dmPut(c,15u,iterations);dmPut(c,16u,lo);dmPut(c,17u,origin);dmPut(c,18u,actual);dmPut(c,19u,ep.h.x);dmPut(c,20u,ep.omega);dmPut(c,21u,uint(ep.velocity));dmPut(c,22u,dmBound(ep.hs,268435456u));dmPut(c,23u,reason);dmPut(c,28u,calls);dmPut(c,29u,next);dmPut(c,30u,0u);dmPut(c,31u,closed?1u:0u);dmPut(c,46u,prior|1u);
 }return true;
}
bool dmApplyColumn(uint c){
 uint oldS=dmWord(c,0u),oldF=dmWord(c,1u),q=dmWord(c,18u);uvec4 p=dmRead64(c,2u);uint origin;
 if((dmWord(c,46u)&65535u)!=1u||g9Column(c,0u)!=oldS||g9Column(c,1u)!=oldF||any(notEqual(g9Momentum(c),p))||dmWord(c,4u)!=oldS+oldF||q!=min(dmWord(c,16u),dmWord(c,17u))||!dmOriginPrefix(c,dmWord(c,16u),origin)||origin!=dmWord(c,17u))return false;
 uint event,path;if(!g9Reserve(1u,0u,event,path))return false;
 if(q>0u){if(!g9RecruitMap(c,q))return false;g9PutColumn(c,0u,oldS-q);g9PutColumn(c,1u,oldF+q);g9PutColumn(c,5u,0u);g9PutColumn(c,13u,g9Column(c,13u)|1u);g9Derived(c);}
 if(any(notEqual(g9Momentum(c),p))||g9Column(c,0u)+g9Column(c,1u)!=dmWord(c,4u))return false;
 dmPut(c,26u,g9MapCount(false,c));dmPut(c,27u,g9MapCount(true,c));
 uint row[64];g9Event(row,15u,c,c,c);row[14]|=q>0u?4u:8u;
 const uint mapping[36]=uint[36](0u,1u,2u,3u,0xffffffffu,6u,7u,8u,0xfffffffeu,11u,9u,10u,16u,17u,18u,19u,20u,21u,29u,30u,15u,23u,24u,26u,25u,27u,0xfffffffdu,0xfffffffcu,2u,3u,22u,12u,13u,14u,0xfffffffbu,31u);
 for(uint i=0u;i<36u;++i){uint field=mapping[i];row[15u+i]=field<48u?dmWord(c,field):field==0xffffffffu?W[uConfig]:field==0xfffffffeu?dmParam(2u):field==0xfffffffdu?g9Column(c,0u):field==0xfffffffcu?g9Column(c,1u):W[uConfig+6u];}
 if(!g9WriteEvent(event,row)||!g9Seal())return false;dmPut(c,46u,(dmWord(c,46u)&0xffff0000u)|3u);return true;
}
bool dmProx(uint c,uvec4 trial,out uvec4 end,out bool supported){
 uint f=dmWord(c,32u);uvec4 product,staticBudget,dynamicBudget,num,den=g9U32(65536u*120u*uJ);
 if(!g9UMul(g9U32(C[23u]),g9U32(f),product)||!g9UMul(product,g9U32(W[uConfig]),num)||!g9UDiv(num,den,staticBudget,product)||!g9UMul(g9U32(C[23u]),g9U32(f),product)||!g9UMul(product,g9U32(W[uConfig+1u]),num)||!g9UDiv(num,den,dynamicBudget,product))return false;
 supported=dmWord(c,33u)==0u&&g9UCmp(g9Magnitude(trial),staticBudget)<=0;end=uvec4(0u);
 if(!supported&&g9UCmp(g9Magnitude(trial),dynamicBudget)>0){uvec4 diff;if(!g9USub(g9Magnitude(trial),dynamicBudget,diff)||!g9FromMagnitude(diff,g9Negative(trial),end))return false;}return true;
}
bool dmPredictor(){
 for(uint c=0u;c<128u;++c){dmPut(c,32u,g9Column(c,1u));dmPut(c,33u,g9Column(c,4u));uvec4 entry=g9Momentum(c),trial=g9ReadWide(uColumnStage+c*64u),end;bool supported;
  if(!dmStore64(c,34u,entry)||!dmStore64(c,36u,trial)||!dmProx(c,trial,end,supported)||!dmStore64(c,38u,end))return false;dmPut(c,42u,supported?1u:0u);dmPut(c,46u,dmWord(c,46u)|4u);
 }return true;
}
bool dmMidpoint(){
 uvec4 maximum=uvec4(0u);
 for(uint c=0u;c<128u;++c){uvec4 sum,mid;if(!g9SAdd(dmRead64(c,34u),dmRead64(c,38u),sum)||!g9SDiv(sum,g9U32(2u),1u,mid))return false;uint f=dmWord(c,32u);
  if(f==0u&&!g9Zero(mid)){g9Reject(7u,c,40u);return false;}uvec4 bound,product,speed=uvec4(0u);uint wave=0u;
  if(!g9UMul(g9U32(f),g9U32(1073741824u),bound))return false;if(g9UCmp(g9Magnitude(mid),bound)>0){g9Reject(6u,c,40u);return false;}if(!dmStore64(c,40u,mid))return false;
  if(f>0u&&(!g9SDiv(g9Magnitude(mid),g9U32(f),3u,speed)||!g9UMul(g9U32(C[23u]),g9U32(f),product)||!g9CeilSqrt64(product.xy,wave)||!g9UAdd(speed,g9U32(wave),speed)))return false;
  if(g9UCmp(speed,maximum)>0)maximum=speed;
 }
 for(uint slot=0u;slot<uK;++slot)if(g9Packet(slot,0u)!=0u){uint speed=max(uint(abs(int(g9Packet(slot,3u)))),uint(abs(int(g9Packet(slot,4u)))));if(g9UCmp(g9U32(speed),maximum)>0)maximum=g9U32(speed);}
 uint g=W[uConfig+7u],extra=g/120u+uint(g%120u!=0u);if(!g9UAdd(maximum,g9U32(extra),maximum))return false;
 if(g9UCmp(maximum,g9U32(60u*uJ*1048576u))>0){g9Reject(5u,0u,40u);return false;}for(uint c=0u;c<128u;++c)dmPut(c,44u,1u);return true;
}
bool dmPredictorEvents(){
 for(uint c=0u;c<128u;++c){if(g9Column(c,1u)!=dmWord(c,32u)||g9Column(c,4u)!=dmWord(c,33u)||any(notEqual(g9Momentum(c),dmRead64(c,34u)))||dmWord(c,44u)!=1u)return false;
  if(W[uColumnStage+c*64u+14u]!=(dmWord(c,43u)&dmWord(c,42u)))return false;uint row[64];g9Event(row,16u,c,c,c);row[15]=dmWord(c,32u);row[16]=dmWord(c,33u);for(uint i=0u;i<8u;++i)row[17u+i]=dmWord(c,34u+i);row[25]=dmWord(c,42u);row[26]=dmWord(c,43u);row[27]=1u;row[28]=1u;row[29]=uJ;dmPut(c,45u,g9Ctl(7u)+1u);if(!g9OneEvent(row))return false;dmPut(c,46u,dmWord(c,46u)|8u);
 }return true;
}

