bool inventory(){
 if(C[uHeader]!=1213420103u||C[uHeader+1u]!=2u||C[uHeader+2u]!=256u||C[uHeader+31u]!=3u||C[uHeader+32u]!=0u||C[uHeader+34u]>4096u){qfail(1u);return false;}
 uint massByColumn[256],liveSlot[640];
 uint live[2]=uint[2](0u,0u);
 bool malformed=false;
 for(uint i=0u;i<256u;++i)massByColumn[i]=0u;
 for(uint ns=0u;ns<2u;++ns) {
  uint start=ns==0u?0u:512u;
  for(uint i=0u;i<slots(ns);++i){
   uint mass=C[at(ns,0u,i)];if(mass==0u)continue;
   uint x=C[at(ns,1u,i)],y=C[at(ns,2u,i)];
   int vx=int(C[at(ns,3u,i)]),vy=int(C[at(ns,4u,i)]);
   if(mass>CQ||x>=128u*Q||y>CQ||vx< -1073741824||vx>1073741824||vy< -1073741824||vy>1073741824||(ns==1u&&vy>0)||C[at(ns,5u,i)]==0u||(tag(ns,i)&15u)<1u||(tag(ns,i)&15u)>3u){malformed=true;continue;}
   uint index=ns*128u+(x>>20u);
   massByColumn[index]=min(CQ+1u,massByColumn[index]+mass);
   liveSlot[start+live[ns]++]=i;
  }
 }
 uvec2 sand=uvec2(0u),water=uvec2(0u);
 for(uint c=0u;c<128u;++c){
  if(col(0u,c)!=0u){qfail(22u);return false;}
  if(col(1u,c)!=0u||col(3u,c)!=0u||col(4u,c)!=0u||col(5u,c)!=0u||col(9u,c)!=0u||col(11u,c)!=0u||col(10u,c)!=col(2u,c)){qfail(15u);return false;}
  uint s=col(10u,c),w=col(6u,c),p=massByColumn[c],j=massByColumn[128u+c];
  if(malformed||s>CQ||w>CQ-s||p>CQ-s-w||j>CQ-s-w-p||p!=col(8u,c)||j!=col(7u,c)||col(12u,c)!=s+w||col(13u,c)!=s+w+p+j){qfail(15u);return false;}
  if(!grsAdd64(sand,uvec2(s+p,0u),sand)||!grsAdd64(water,uvec2(w+j,0u),water)){qfail(13u);return false;}
 }
 for(uint ns=0u;ns<2u;++ns){
  uint start=ns==0u?0u:512u,origin=ns==0u?8u:6u,gen=ns==0u?10u:11u,src=ns==0u?7u:12u;
  for(uint n=0u;n<live[ns];++n){
   uint i=liveSlot[start+n],o=C[at(ns,origin,i)],id=C[at(ns,5u,i)];
   if(o==0u||C[at(ns,origin+1u,i)]!=0u||o>C[uHeader+34u]||C[at(ns,gen,i)]==0u||C[at(ns,src,i)]==0u){qfail(15u);return false;}
   uint se=uSourceEvents+(o-1u)*16u;
   if(W[se+4u]!=ns||W[se+9u]!=C[at(ns,src,i)]||id<W[se+7u]||id-W[se+7u]>=W[se+8u]){qfail(15u);return false;}
   for(uint next=n+1u;next<live[ns];++next)if(C[at(ns,5u,liveSlot[start+next])]==id){qfail(15u);return false;}
  }
 }
 uvec2 es,ew;
 if(!grsAdd64(uvec2(C[uHeader+64u],C[uHeader+65u]),uvec2(C[uHeader+60u],C[uHeader+61u]),es)||!grsAdd64(uvec2(C[uHeader+66u],C[uHeader+67u]),uvec2(C[uHeader+62u],C[uHeader+63u]),ew)||grsCompare64(es,sand)!=0||grsCompare64(ew,water)!=0||grsCompare64(sand,uvec2(C[uHeader+56u],C[uHeader+57u]))!=0||grsCompare64(water,uvec2(C[uHeader+58u],C[uHeader+59u]))!=0){qfail(15u);return false;}
 return true;
}
