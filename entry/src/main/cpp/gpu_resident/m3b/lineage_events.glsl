/* m3b/capture_schema_generated.glsl */
// One paired stream in binding3. WORK's cursor retains planned and sealed
// lengths separately; a failed body is forensic data, never a readable prefix.
uniform uint uV5Cursor,uV5AuxWord,uV5AuxCapacity;
bool v5Replay=false;
uint v5Cursor(uint field){return W[uV5Cursor+field];}
void v5SetCursor(uint field,uint value){W[uV5Cursor+field]=value;}
void v5AuxRow(out uint row[32],uint kind,uint physical){
 for(uint i=0u;i<32u;++i)row[i]=0u;
 row[0]=894456396u;row[1]=5u;row[2]=kind;row[3]=uM3Role;row[4]=uNonce.x;row[5]=uNonce.y;
 row[6]=v5Cursor(1u)+1u;row[7]=physical;row[31]=3235774469u;
}
uint v5AuxMask(uint kind){
 if(kind==27u)return 2751414271u;if(kind==28u)return 3218079743u;if(kind==29u)return 3207856127u;
 if(kind==30u)return 3222217215u;if(kind==31u)return 2147549183u;return 0u;
}
bool v5Credit(uint physical,uint aux){
 uint p=v5Cursor(0u),a=v5Cursor(1u);
 if(v5Cursor(7u)!=0u||v5Cursor(4u)!=0u||p>uEventCapacity||physical>uEventCapacity-p||a>uV5AuxCapacity||aux>uV5AuxCapacity-a){qfail(39u);return false;}
 v5SetCursor(4u,1u);v5SetCursor(5u,p);v5SetCursor(6u,a);v5SetCursor(8u,a);v5SetCursor(0u,p+physical);v5SetCursor(1u,a+aux);return true;
}
bool v5WriteAuxAt(uint row[32],uint at){
 uint mask=v5AuxMask(row[2]);
 if(mask==0u||at>=uV5AuxCapacity||at>=v5Cursor(1u)||v5Cursor(4u)==0u){qfail(39u);return false;}
 row[6]=at+1u;
 for(uint i=0u;i<32u;++i)if(((mask>>i)&1u)==0u)row[i]=0u;
 if(v5Replay){for(uint i=0u;i<32u;++i)if(E[uV5AuxWord+at*32u+i]!=row[i]){qfail(38u);return false;}}
 else{for(uint i=0u;i<31u;++i)E[uV5AuxWord+at*32u+i]=row[i];memoryBarrierBuffer();E[uV5AuxWord+at*32u+31u]=row[31];}
 return true;
}
bool v5WriteAux(uint row[32]){
 uint at=v5Cursor(8u);if(!v5WriteAuxAt(row,at))return false;v5SetCursor(8u,at+1u);return true;
}
bool v5SealCompound(){
 if(W[0]!=0u){if(v5Cursor(7u)==0u)v5SetCursor(7u,W[0]);return false;}
 if(v5Cursor(4u)==0u||v5Cursor(8u)!=v5Cursor(1u)){qfail(39u);return false;}
 v5SetCursor(2u,v5Cursor(0u));v5SetCursor(3u,v5Cursor(1u));v5SetCursor(4u,0u);return true;
}
uint m3RoleAddress(uint role);
void v5ClearScopeEntry(){for(uint i=0u;i<32u;++i)v5SetCursor(i,0u);}
void v5ResetCompoundMutable(uint physicalStart,uint auxStart){
 for(uint i=0u;i<9u;++i)v5SetCursor(i,0u);
 v5SetCursor(0u,physicalStart);v5SetCursor(2u,physicalStart);v5SetCursor(1u,auxStart);v5SetCursor(3u,auxStart);v5SetCursor(8u,auxStart);
}
bool v5BeginScope(uint physicalStart,uint auxStart){
 uint role=m3RoleAddress(uM3Role);uvec2 branch=uvec2(W[role+5u],W[role+6u]);
 if(v5Replay){
  if(v5Cursor(11u)!=1u||v5Cursor(17u)!=V5_ENTRY_REVISION||v5Cursor(31u)!=3235775745u||v5Cursor(9u)!=physicalStart||v5Cursor(10u)!=auxStart||v5Cursor(12u)!=uNonce.x||v5Cursor(13u)!=uNonce.y||v5Cursor(14u)!=uM3Role||v5Cursor(15u)!=branch.x||v5Cursor(16u)!=branch.y){qfail(38u);return false;}
  v5ResetCompoundMutable(physicalStart,auxStart);return true;
 }
 v5ClearScopeEntry();v5ResetCompoundMutable(physicalStart,auxStart);
 v5SetCursor(9u,physicalStart);v5SetCursor(10u,auxStart);v5SetCursor(12u,uNonce.x);v5SetCursor(13u,uNonce.y);v5SetCursor(14u,uM3Role);v5SetCursor(15u,branch.x);v5SetCursor(16u,branch.y);v5SetCursor(17u,V5_ENTRY_REVISION);v5SetCursor(11u,1u);
 memoryBarrierBuffer();v5SetCursor(31u,3235775745u);return true;
}

bool v5Plus(uint header,uint amount){
 uvec2 next;if(!grsAdd64(uvec2(C[uHeader+header],C[uHeader+header+1u]),uvec2(amount,0u),next)){qfail(13u);return false;}
 C[uHeader+header]=next.x;C[uHeader+header+1u]=next.y;return true;
}
