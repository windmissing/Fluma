uint v5LandAllowed(uint slot,uint c,uint request){
 uvec3 origin=uvec3(C[8u*uK+slot],C[9u*uK+slot],C[7u*uK+slot]);
 return v5CanAppend(c,origin)?request:0u;
}
bool v5LandPatch(uint slot,uint c,uint q,uint physical,uint materialKind){
 uint mass=C[slot],tag=C[v5Aux(slot)],depth=C[uV5Depth+c],id=C[5u*uK+slot],generation=C[10u*uK+slot];
 uvec3 origin=uvec3(C[8u*uK+slot],C[9u*uK+slot],C[7u*uK+slot]);bool append=depth==0u||!v5Same(v5Origin(c,depth-1u),origin);
 if(q>mass||tag>mass||depth>64u||q>0u&&!v5CanAppend(c,origin)){qfail(34u);return false;}
 if(!v5Credit(q==0u?0u:1u,1u))return false;
 uint returned=0u;if(q>0u){if(!v5Append(c,origin,q)||!v5ReturnTag(slot,mass,q,returned)||!v5Plus(171u,returned)){qfail(36u);return false;}}
 uint row[32];v5AuxRow(row,28u,physical);row[8]=row[9]=c;row[10]=origin.x;row[11]=origin.y;row[12]=origin.z;row[13]=q;
 row[14]=tag;row[15]=tag-returned;row[16]=mass;row[17]=mass-q;row[18]=col(10u,c);row[19]=col(10u,c)+q;row[22]=depth;row[23]=C[uV5Depth+c];row[25]=materialKind;
 row[26]=id;row[27]=generation;row[28]=slot;row[29]=(q>0u&&append?2u:0u)|(q==mass?4u:0u)|(C[14u*uK+slot]==1u?8u:0u);
 return v5WriteAux(row);
}
uint v5FragmentCount(uint donor,uint q){
 uint depth=C[uV5Depth+donor],count=0u;
 for(uint n=0u;n<depth&&q>0u;++n){uint amount=min(q,C[v5Layer(donor,depth-1u-n)+3u]);q-=amount;++count;}
 return q==0u?count:65u;
}
bool v5ColorCredit(uint physical,uint aux){
 uint p=v5Cursor(0u),a=v5Cursor(1u);if(v5Cursor(4u)==0u||p>uEventCapacity||physical>uEventCapacity-p||a>uV5AuxCapacity||aux>uV5AuxCapacity-a){qfail(39u);return false;}
 v5SetCursor(0u,p+physical);v5SetCursor(1u,a+aux);return true;
}
// The same top-first semantic patch is used at each physical replay cut. It
// reads the replay bank, rebuilds each fragment, and compares the actual words.
bool v5ReplayBed(uint donor,uint receiver,uint q,uint physical){
 uint count=v5FragmentCount(donor,q);if(count>64u||v5BedPrefix(donor,receiver,q)!=q||!v5Credit(1u,count))return false;
 for(uint fragment=0u;fragment<count;++fragment){
  uint sd=C[uV5Depth+donor],dd=C[uV5Depth+receiver],rank=sd-1u,at=v5Layer(donor,rank),beforeMass=C[at+3u],amount=min(q,beforeMass);
  uvec3 origin=v5Origin(donor,rank);bool append=dd==0u||!v5Same(v5Origin(receiver,dd-1u),origin);uint destination=append?0u:C[v5Layer(receiver,dd-1u)+3u];
  if(!v5Pop(donor,amount)||!v5Append(receiver,origin,amount)){qfail(34u);return false;}
  uint row[32];v5AuxRow(row,27u,physical);row[8]=donor;row[9]=receiver;row[10]=origin.x;row[11]=origin.y;row[12]=origin.z;row[13]=amount;
  row[16]=beforeMass;row[17]=beforeMass-amount;row[18]=destination;row[19]=destination+amount;row[20]=sd;row[21]=C[uV5Depth+donor];row[22]=dd;row[23]=C[uV5Depth+receiver];row[24]=fragment;row[25]=9u;row[29]=(amount==beforeMass?1u:0u)|(append?2u:0u);
  if(!v5WriteAux(row))return false;q-=amount;
 }
 return v5SealCompound();
}
shared uint v5ColorErrors[64];
// Exactly one 64-lane group. No global-spin or shared array of 43*64 cohorts.
void v5ApplyBedColor(){
 uint lane=gl_LocalInvocationIndex,edge=3u*lane+uColor,a=uEdges+16u*min(edge,126u),amount=edge<127u?W[a+3u]:0u;
 uint donor=edge<127u?W[a]:0u,receiver=edge<127u?W[a+1u]:0u,left=amount;
 for(uint fragment=0u;fragment<64u;++fragment){
  uint error=0xffffffffu,sd=0u,dd=0u,mass=0u,q=0u,destination=0u;uvec3 origin=uvec3(0u);bool append=false;
  if(left>0u){
   sd=C[uV5Depth+donor];dd=C[uV5Depth+receiver];
   if(sd==0u||sd>64u||dd>64u)error=fragment*1024u+edge*8u;
   else{uint at=v5Layer(donor,sd-1u);mass=C[at+3u];origin=v5Origin(donor,sd-1u);q=min(left,mass);append=dd==0u||!v5Same(v5Origin(receiver,dd-1u),origin);destination=append?0u:C[v5Layer(receiver,dd-1u)+3u];
    if(q==0u||origin.x==0u||origin.y!=0u||origin.z==0u)error=fragment*1024u+edge*8u+1u;
    else if(append&&dd==64u||destination>268435456u-q)error=fragment*1024u+edge*8u+2u;
   }
  }
  v5ColorErrors[lane]=error;memoryBarrierShared();barrier();
  if(lane==0u){uint first=0xffffffffu;for(uint i=0u;i<64u;++i)first=min(first,v5ColorErrors[i]);if(first!=0xffffffffu){qfail(34u);W[19]=first;}}
  memoryBarrierBuffer();barrier();if(W[0]!=0u)return;
  if(left>0u){
   v5Pop(donor,q);v5Append(receiver,origin,q);
   uint row[32];v5AuxRow(row,27u,W[a+10u]+1u);row[8]=donor;row[9]=receiver;row[10]=origin.x;row[11]=origin.y;row[12]=origin.z;row[13]=q;
   row[16]=mass;row[17]=mass-q;row[18]=destination;row[19]=destination+q;row[20]=sd;row[21]=C[uV5Depth+donor];row[22]=dd;row[23]=C[uV5Depth+receiver];row[24]=fragment;row[25]=9u;row[29]=(q==mass?1u:0u)|(append?2u:0u);
   v5WriteAuxAt(row,W[a+9u]+fragment);left-=q;
  }
  memoryBarrierBuffer();barrier();
 }
 v5ColorErrors[lane]=left>0u?edge:0xffffffffu;memoryBarrierShared();barrier();
 if(lane==0u)for(uint i=0u;i<64u;++i)if(v5ColorErrors[i]!=0xffffffffu){qfail(34u);break;}
 memoryBarrierBuffer();barrier();if(W[0]!=0u)return;
 if(amount>0u)edgeApply(edge,true);
 memoryBarrierBuffer();barrier();
 if(lane==0u){v5SetCursor(8u,v5Cursor(1u));v5SealCompound();}
}
