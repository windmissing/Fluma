// Same bitonic network and total order as sortIndex, confined to one workgroup.
// key = (arrival column, arrival y, id, slot); dead slots sort after live slots.
shared uvec4 sortKeys[256];

bool groupKeyLess(uvec4 a,uvec4 b) {
 if(a.x!=b.x)return a.x<b.x;
 if(a.y!=b.y)return a.y<b.y;
 if(a.z!=b.z)return a.z<b.z;
 return a.w<b.w;
}

void groupSort(uint offset,uint count,uint ns,bool arrival) {
 uint lane=gl_LocalInvocationID.x;
 uint slotCount=slots(ns);
 for(uint i=lane;i<count;i+=64u) {
  bool live=i<slotCount&&C[at(ns,0u,i)]!=0u;
  if(live) {
   uint column=arrival?C[at(ns,1u,i)]>>20u:0u;
   uint y=arrival?C[at(ns,2u,i)]:0u;
   sortKeys[i]=uvec4(column,y,C[at(ns,5u,i)],i);
  } else {
   sortKeys[i]=uvec4(0xffffffffu,0u,0u,i);
  }
 }
 barrier();
 for(uint k=2u;k<=count;k*=2u) {
  for(uint j=k/2u;j!=0u;j/=2u) {
   for(uint i=lane;i<count;i+=64u) {
    uint peer=i^j;
    if(peer<=i)continue;
    uvec4 a=sortKeys[i],b=sortKeys[peer];
    bool up=(i&k)==0u;
    if(up?groupKeyLess(b,a):groupKeyLess(a,b)) {
     sortKeys[i]=b;
     sortKeys[peer]=a;
    }
   }
   barrier();
  }
 }
 for(uint i=lane;i<count;i+=64u)W[offset+i]=sortKeys[i].w;
}
