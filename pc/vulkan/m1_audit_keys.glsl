shared uint auditKey[512];
shared uint auditScratch[512];

// Only the audit lane uses these arrays. Sort one material at a time so the
// same ID can still exist in the other namespace.
bool uniqueAuditKeys(uint size) {
 if(size<2u)return true;
 for(uint shift=0u;shift<32u;shift+=4u){
  uint bins[16];
  for(uint i=0u;i<16u;++i)bins[i]=0u;
  for(uint i=0u;i<size;++i)++bins[(auditKey[i]>>shift)&15u];
  uint prefix=0u;
  for(uint i=0u;i<16u;++i){uint count=bins[i];bins[i]=prefix;prefix+=count;}
  for(uint i=0u;i<size;++i){uint key=auditKey[i];auditScratch[bins[(key>>shift)&15u]++]=key;}
  for(uint i=0u;i<size;++i)auditKey[i]=auditScratch[i];
 }
 for(uint i=1u;i<size;++i)if(auditKey[i-1u]==auditKey[i])return false;
 return true;
}
