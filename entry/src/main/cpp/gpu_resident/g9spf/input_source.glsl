bool g9DecodeBinary64(uvec2 raw,uint fractionBits,bool roundEven,out uvec4 result){
 result=uvec4(0u);uint exponent=(raw.y>>20u)&2047u,fraction=raw.y&0xfffffu;bool negative=(raw.y>>31u)!=0u;
 if(exponent==2047u)return false;if(exponent==0u)return true;
 uvec4 magnitude=uvec4(raw.x,fraction|0x100000u,0u,0u);int shift=int(exponent)-1023-52+int(fractionBits);
 if(shift>=0){if(!g9UShift(magnitude,uint(shift),magnitude))return false;}
 else{
  uint right=uint(-shift);uvec4 quotient=uvec4(0u);bool halfway=false,below=false;
  for(uint bit=0u;bit<64u;++bit)if(((magnitude[int(bit/32u)]>>(bit%32u))&1u)!=0u){
   if(bit>=right){uint target=bit-right;quotient[int(target/32u)]|=1u<<(target%32u);}
   else if(bit+1u==right)halfway=true;else below=true;
  }
  if(roundEven&&halfway&&(below||(quotient.x&1u)!=0u)&&!g9UAdd(quotient,g9U32(1u),quotient))return false;
  magnitude=quotient;
 }
 return g9FromMagnitude(magnitude,negative,result);
}
uint g9Input(uint word){return W[uInput+word];}
uvec2 g9InputPair(uint word){return uvec2(g9Input(word),g9Input(word+1u));}
bool g9InputBodyWord(uint kind,uint word){
 if(kind==0u)return false;
 if(word>=40u&&word<=45u)return true;
 if(kind==1u)return word>=64u&&word<=76u;
 return kind==2u&&((word>=46u&&word<=63u)||word==77u);
}
bool g9FlickVerify();
bool g9ValidateInput(uint rootKind){
 if(g9Input(0u)!=G9_INPUT_MAGIC||(g9Input(1u)!=1u&&g9Input(1u)!=2u)||g9Input(2u)!=G9_PROFILE||g9Input(38u)!=0u||g9Input(39u)!=0u||g9Input(40u)>2u)return false;
 if(any(notEqual(g9InputPair(4u),uRun))||any(notEqual(g9InputPair(6u),uOwner))||any(notEqual(g9InputPair(8u),uContext))||any(notEqual(g9InputPair(10u),uNonce))||any(notEqual(g9InputPair(12u),uInputGeneration)))return false;
 if(rootKind==1u&&g9Input(40u)!=1u||rootKind==2u&&g9Input(40u)==1u)return false;
 for(uint word=40u;word<128u;++word)if(!g9InputBodyWord(g9Input(40u),word)&&g9Input(word)!=0u)return false;
 for(uint i=0u;i<8u;++i)if(g9Input(26u+i)!=W[uConfig+16u+i])return false;
 if(g9Input(1u)==2u&&!g9FlickVerify())return false;
 if(g9Input(1u)==1u&&W[uConfig+6u]==2u)return false;
 return g9Input(25u)==W[uConfig+6u]&&g9Input(24u)==uint(uAcceptedAngle);
}
bool g9CommandCompare(uint capsule){
 for(uint i=40u;i<128u;++i)if(C[capsule+i]!=g9Input(i))return false;
 const uint also[14]=uint[14](4u,5u,12u,13u,26u,27u,28u,29u,30u,31u,32u,33u,0u,1u);
 for(uint i=0u;i<12u;++i)if(C[capsule+also[i]]!=g9Input(also[i]))return false;return true;
}
bool g9Dedup(){
 uint kind=g9Input(40u);g9Set(18u,0u);if(kind==0u)return true;
 uint seqAt=kind==1u?28u:30u,validAt=kind==1u?104u:105u,capsule=kind==1u?256u:384u;
 uvec4 seq=g9U64(g9InputPair(41u)),old=g9U64(uvec2(C[seqAt],C[seqAt+1u]));
 if(g9Zero(seq)){g9Fault(22u,0u,41u);return false;}
 if(C[validAt]!=0u){int cmp=g9UCmp(seq,old);if(cmp<0){g9Fault(20u,0u,41u);return false;}if(cmp==0){if(!g9CommandCompare(capsule)){g9Fault(19u,0u,41u);return false;}g9Set(18u,1u);}}
 return true;
}
bool g9ConsumeCommand(uint outcome,uint admitted){
 uint kind=g9Input(40u);if(kind==0u||g9Ctl(18u)!=0u)return true;
 uint seqAt=kind==1u?28u:30u,commitAt=kind==1u?32u:34u,capsule=kind==1u?256u:384u;
 C[seqAt]=g9Input(41u);C[seqAt+1u]=g9Input(42u);uvec4 next;if(!g9UAdd(g9U64(uBaseCommit),g9U32(1u),next)){g9Fault(29u,0u,commitAt);return false;}
 C[commitAt]=next.x;C[commitAt+1u]=next.y;C[kind==1u?36u:37u]=outcome;C[kind==1u?104u:105u]=1u;
 if(kind==1u){C[38u]=admitted;C[39u]=0u;}
 for(uint i=0u;i<128u;++i)C[capsule+i]=g9Input(i);return true;
}
void g9RetirePacket(uint slot){uint generation=g9Packet(slot,7u);for(uint f=0u;f<64u;++f)g9PutPacket(slot,f,0u);g9PutPacket(slot,7u,generation);}
bool g9PacketCertificate(uint slot,uint kind,uint origin,uint sourceId,uint q,uint column,uint rank,uint parent,uint ordinal){
 uint at=uPacket+slot*64u;uvec4 commit;if(!g9UAdd(g9U64(uBaseCommit),g9U32(1u),commit))return false;
 C[at+17u]=C[at+6u];C[at+18u]=C[at+7u];C[at+19u]=origin;C[at+20u]=0u;C[at+21u]=sourceId;C[at+22u]=kind;C[at+23u]=commit.x;C[at+24u]=commit.y;C[at+25u]=ordinal;C[at+26u]=q;C[at+27u]=column;C[at+28u]=rank;C[at+29u]=parent;C[at+30u]=1u;return true;
}
bool g9SourceDecode(){
 if(g9Ctl(18u)!=0u)return true;
 uvec4 mass,x,y2,vx,vy;
 if((g9Input(65u)&0x80000000u)!=0u||g9Input(65u)>0x40700000u||(g9Input(65u)==0x40700000u&&g9Input(64u)!=0u)||
    !g9DecodeBinary64(g9InputPair(64u),20u,false,mass)||!g9DecodeBinary64(g9InputPair(66u),20u,true,x)||!g9DecodeBinary64(g9InputPair(68u),21u,true,y2)||!g9DecodeBinary64(g9InputPair(70u),20u,true,vx)||!g9DecodeBinary64(g9InputPair(72u),20u,true,vy)){
  g9Fault(22u,0u,64u);return false;
 }
 uint requested,px,py;int vxx,vyy;
 if(!g9Uint(mass,requested)||!g9Uint(x,px)||!g9Uint(y2,py)||!g9I32(vx,vxx)||!g9I32(vy,vyy)||px>=134217728u||abs(vxx)>1073741824||abs(vyy)>1073741824||g9Input(74u)==0u){g9Fault(22u,0u,66u);return false;}
 uint c=px>>20u,q=min(requested,244666u);if(g9Occupied(c)>268435456u){g9Fault(24u,c,8u);return false;}q=min(q,268435456u-g9Occupied(c));
  if(py<2u*g9Material(c)+q||py>536870912u-q){g9Fault(22u,c,68u);return false;}
 uint slot=uK;for(uint i=0u;i<uK;++i)if(g9Packet(i,0u)==0u){slot=i;break;}
 uint reason=requested==0u?4u:0u;
 if(slot==uK){q=0u;reason=13u;}else if(g9Packet(slot,7u)==0xffffffffu){g9Fault(29u,slot,7u);return false;}
 if(C[18u]==0u){q=0u;reason=14u;}if(C[19u]>=4096u){q=0u;reason=15u;}
 g9Set(48u,requested);g9Set(49u,q);g9Set(50u,slot);g9Set(51u,px);g9Set(52u,py);g9Set(53u,uint(vxx));g9Set(54u,uint(vyy));g9Set(55u,reason);
 return true;
}
bool g9SourceBirth(){
 if(g9Ctl(18u)!=0u)return true;uint q=g9Ctl(49u);if(q==0u)return true;
 uint slot=g9Ctl(50u),x=g9Ctl(51u),c=x>>20u,id=C[18u],origin=C[19u]+1u,generation=g9Packet(slot,7u)+1u;
 uint event,path;if(!g9Reserve(1u,0u,event,path))return false;
 uint src=uSources+(origin-1u)*16u;C[src]=origin;C[src+1u]=0u;C[src+2u]=g9Input(74u);C[src+3u]=g9Input(75u);C[src+4u]=q;C[src+5u]=0u;C[src+6u]=g9Input(41u);C[src+7u]=g9Input(42u);C[src+8u]=uInputGeneration.x;C[src+9u]=uInputGeneration.y;C[src+14u]=1u;C[19u]=origin;
 // Four independent FNV1A32_LE locators. They never authorize dedup; the last
 // command capsule comparison above always compares original full body words.
 uvec4 digest=uvec4(2166136261u,2166136261u^1u,2166136261u^2u,2166136261u^3u);
 for(uint word=40u;word<128u;++word){uint v=g9Input(word);for(uint byte=0u;byte<4u;++byte)for(uint lane=0u;lane<4u;++lane)digest[lane]=(digest[lane]^((v>>(8u*byte))&255u))*16777619u;}
 for(uint i=0u;i<4u;++i)C[src+10u+i]=digest[i];
 g9RetirePacket(slot);g9PutPacket(slot,0u,q);g9PutPacket(slot,1u,x);g9PutPacket(slot,2u,g9Ctl(52u));g9PutPacket(slot,3u,g9Ctl(53u));g9PutPacket(slot,4u,g9Ctl(54u));g9PutPacket(slot,5u,c);g9PutPacket(slot,6u,id);g9PutPacket(slot,7u,generation);g9PutPacket(slot,8u,origin);g9PutPacket(slot,10u,g9Input(74u));g9PutPacket(slot,11u,0xffffffffu);
 if(!g9PacketCertificate(slot,0u,origin,g9Input(74u),q,c,0u,0u,event+1u)){g9Fault(29u,slot,23u);return false;}
 C[18u]=id+1u;g9PutColumn(c,8u,g9Column(c,8u)+q);
 uvec4 px,py,energy;if(!g9SMul(g9U32(q),g9S32(int(g9Ctl(53u))),px)||!g9SMul(g9U32(q),g9S32(int(g9Ctl(54u))),py)||!g9PacketEnergy(q,x,g9Ctl(52u),int(g9Ctl(53u)),int(g9Ctl(54u)),energy)||!g9Accumulate(66u,px)||!g9Accumulate(70u,py)||!g9Accumulate(90u,energy)){g9Fault(29u,slot,0u);return false;}
 uint row[64];g9Event(row,G9_EVENT_SOURCE_BIRTH,slot,c,c);row[12]=id;row[13]=generation;row[14]|=4u;row[15]=q;row[16]=origin;row[18]=g9Input(74u);row[19]=x;row[20]=g9Ctl(52u);row[21]=g9Ctl(53u);row[22]=g9Ctl(54u);row[23]=generation-1u;row[24]=generation;for(uint i=0u;i<4u;++i)row[25u+i]=energy[i];return g9WriteEvent(event,row)&&g9Seal();
}
