// V5 semantic state operations. All offsets are words in the bound WRITE bank.
// No host mirror and no geometry-driven reclassification of accepted medium.
uniform uint uV5Cohort,uV5Depth,uV5Certificate,uV5Aux;
uint v5Layer(uint column,uint rank){return uV5Cohort+(column*64u+rank)*4u;}
uint v5Cert(uint slot){return uV5Certificate+slot*16u;}
uint v5Aux(uint slot){return uV5Aux+slot*4u;}
bool v5Same(uvec3 a,uvec3 b){return all(equal(a,b));}
uvec3 v5Origin(uint column,uint rank){uint a=v5Layer(column,rank);return uvec3(C[a],C[a+1u],C[a+2u]);}
bool v5CanAppend(uint column,uvec3 origin){
 uint depth=C[uV5Depth+column];
 return depth<=64u&&(depth==0u||v5Same(v5Origin(column,depth-1u),origin)||depth<64u);
}
bool v5Append(uint column,uvec3 origin,uint mass){
 if(column>=128u||mass==0u||origin.x==0u||origin.y!=0u||origin.z==0u||!v5CanAppend(column,origin))return false;
 uint depth=C[uV5Depth+column];
 if(depth>0u&&v5Same(v5Origin(column,depth-1u),origin)){
  uint a=v5Layer(column,depth-1u);if(C[a+3u]>268435456u-mass)return false;C[a+3u]+=mass;
 }else{
  uint a=v5Layer(column,depth);C[a]=origin.x;C[a+1u]=origin.y;C[a+2u]=origin.z;C[a+3u]=mass;C[uV5Depth+column]=depth+1u;
 }return true;
}
bool v5Pop(uint column,uint mass){
 uint depth=C[uV5Depth+column];if(depth==0u||depth>64u||mass==0u)return false;
 uint a=v5Layer(column,depth-1u),beforeMass=C[a+3u];if(mass>beforeMass)return false;
 if(mass==beforeMass){for(uint f=0u;f<4u;++f)C[a+f]=0u;C[uV5Depth+column]=depth-1u;}
 else C[a+3u]-=mass;
 return true;
}
// Capacity simulation reads no mutation and returns a whole-fragment prefix.
// At most 64 top-first fragments; partial last fragment is allowed by q.
uint v5BedPrefix(uint donor,uint receiver,uint requested){
 if(donor>=128u||receiver>=128u||donor==receiver)return 0u;
 uint sourceDepth=C[uV5Depth+donor],depth=C[uV5Depth+receiver];
 if(sourceDepth>64u||depth>64u)return 0u;
 uvec3 top=depth==0u?uvec3(0u):v5Origin(receiver,depth-1u);uint granted=0u;
 for(uint n=0u;n<sourceDepth&&granted<requested;++n){
  uint rank=sourceDepth-1u-n,a=v5Layer(donor,rank);uvec3 origin=v5Origin(donor,rank);
  if(depth==0u||!v5Same(top,origin)){if(depth==64u)break;++depth;top=origin;}
  uint q=min(requested-granted,C[a+3u]);granted+=q;
 }return granted;
}
void v5ClearCertificate(uint slot){for(uint f=0u;f<16u;++f)C[v5Cert(slot)+f]=0u;for(uint f=0u;f<4u;++f)C[v5Aux(slot)+f]=0u;}
bool v5BirthCertificate(uint slot,uint kind,uvec2 commit,uint ordinal,uint rank,uint parentMass){
 if(slot>=uK||C[slot]==0u||kind>1u||ordinal==0u)return false;
 uint a=v5Cert(slot);for(uint f=0u;f<16u;++f)C[a+f]=0u;
 C[a]=C[5u*uK+slot];C[a+1u]=C[10u*uK+slot];C[a+2u]=C[8u*uK+slot];C[a+3u]=C[9u*uK+slot];
 C[a+4u]=C[7u*uK+slot];C[a+5u]=kind;C[a+6u]=commit.x;C[a+7u]=commit.y;
 C[a+8u]=ordinal;C[a+9u]=C[slot];C[a+10u]=C[uK+slot]>>20u;C[a+11u]=rank;C[a+12u]=parentMass;C[a+13u]=1u;
 uint b=v5Aux(slot);C[b]=kind==1u?C[slot]:0u;C[b+1u]=kind;C[b+2u]=1u;C[b+3u]=0u;return true;
}
bool v5MergeCertificate(uint survivor,uint removed,uint oldMassA,uint oldMassB){
 uint a=v5Aux(survivor),b=v5Aux(removed),ta=C[a],tb=C[b];
 if(ta>oldMassA||tb>oldMassB||oldMassA>244666u-oldMassB||ta>oldMassA+oldMassB-tb)return false;
 // Birth facts of survivor stay byte-identical, including original birthMass.
 C[a]=ta+tb;v5ClearCertificate(removed);return true;
}
bool v5ReturnTag(uint slot,uint oldMass,uint deposited,out uint returned){
 uint a=v5Aux(slot),tag=C[a];if(oldMass==0u||deposited>oldMass||tag>oldMass)return false;
 if(deposited==oldMass)returned=tag;
 else{uvec4 q;uint remainder;if(!grsDivide128(uvec4(grsMul32(deposited,tag),0u,0u),oldMass,q,remainder)||any(notEqual(q.yzw,uvec3(0u))))return false;returned=q.x;}
 if(deposited==oldMass)v5ClearCertificate(slot);else C[a]=tag-returned;
 return true;
}
bool v5AuditLayers(uint column){
 uint depth=C[uV5Depth+column],sum=0u;if(depth>64u)return false;
 for(uint rank=0u;rank<64u;++rank){
  uint a=v5Layer(column,rank),mass=C[a+3u];
  if(rank>=depth){for(uint f=0u;f<4u;++f)if(C[a+f]!=0u)return false;continue;}
  uvec3 origin=v5Origin(column,rank);if(mass==0u||mass>268435456u-sum||origin.x==0u||origin.y!=0u||origin.z==0u||rank>0u&&v5Same(origin,v5Origin(column,rank-1u)))return false;sum+=mass;
 }return sum==C[uColumn+2u*128u+column];
}
