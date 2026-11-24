// Each invocation owns a subset of slots. Only lane zero writes the
// audit result, retaining the legacy column and reason precedence.
shared uint slotKey[640];
shared uint slotError[640];
shared uint slotColumnMass[256];

void inventoryExtract(bool enabled) {
 uint lane=gl_LocalInvocationIndex;
 for(uint i=lane;i<256u;i+=64u)slotColumnMass[i]=0u;
 barrier();
 for(uint n=lane;n<640u;n+=64u){
  slotKey[n]=0u;
  slotError[n]=0u;
  uint ns=n<512u?0u:1u, i=ns==0u?n:n-512u;
  if(!enabled||i>=slots(ns))continue;
  uint mass=C[at(ns,0u,i)];
  if(mass==0u)continue;
  uint x=C[at(ns,1u,i)],y=C[at(ns,2u,i)],id=C[at(ns,5u,i)];
  int vx=int(C[at(ns,3u,i)]),vy=int(C[at(ns,4u,i)]);
  uint phase=tag(ns,i)&15u;
  if(mass>CQ||x>=128u*Q||y>CQ||vx< -1073741824||vx>1073741824||
     vy< -1073741824||vy>1073741824||(ns==1u&&vy>0)||id==0u||phase<1u||phase>3u){
   slotError[n]=1u;
   continue;
  }
  uint column=ns*128u+(x>>20u);
  uint old=atomicAdd(slotColumnMass[column],0u), prior;
  // Nonnegative terms make saturated accumulation order independent.
  do {
   prior=old;
   old=atomicCompSwap(slotColumnMass[column],prior,min(CQ+1u,prior+mass));
  } while(old!=prior && old<CQ+1u);
  slotKey[n]=id;
  uint origin=ns==0u?8u:6u,gen=ns==0u?10u:11u,src=ns==0u?7u:12u;
  uint o=C[at(ns,origin,i)], serial=C[at(ns,src,i)];
  if(o==0u||C[at(ns,origin+1u,i)]!=0u||o>C[uHeader+34u]||C[at(ns,gen,i)]==0u||serial==0u){
   slotError[n]=2u;
   continue;
  }
  uint se=uSourceEvents+(o-1u)*16u;
  if(W[se+4u]!=ns||W[se+9u]!=serial||id<W[se+7u]||id-W[se+7u]>=W[se+8u])
   slotError[n]=2u;
 }
 barrier();
}

bool inventoryHeader() {
 return C[uHeader]==1213420103u&&C[uHeader+1u]==2u&&C[uHeader+2u]==256u&&
        C[uHeader+31u]==3u&&C[uHeader+32u]==0u&&C[uHeader+34u]<=4096u;
}
