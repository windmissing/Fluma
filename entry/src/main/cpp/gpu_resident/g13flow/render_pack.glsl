void packCell(uint i,out float fields[12],out uint flags){
 uint x=i%nx(),y=i/nx();vec2 pos=(vec2(x,y)+.5)*h();
 float u=.5*(uintBitsToFloat(baseState[256u+y*(nx()+1u)+x])+uintBitsToFloat(baseState[256u+y*(nx()+1u)+x+1u]));
 float v=.5*(uintBitsToFloat(baseState[256u+nuFaces()+y*nx()+x])+uintBitsToFloat(baseState[256u+nuFaces()+(y+1u)*nx()+x]));
 float pi=sfloat(s64(uvec2(baseState[uL[10]+2u*i],baseState[uL[10]+2u*i+1u])),-32);
 vec2 box=vec2(uintBitsToFloat(c[8]),uintBitsToFloat(c[9])),r=pos-.5*box;
 float om=uintBitsToFloat(c[625]);float phi=dot(vec2(uintBitsToFloat(c[627]),uintBitsToFloat(c[628])),r)+.5*om*om*dot(r,r);
 float meanPhi=om*om*(dot(box,box)-2.*h()*h())/24.;
 float curl=(linearRead(0xfffffffeu,true,pos+vec2(.5*h(),0.))-linearRead(0xfffffffeu,true,pos-vec2(.5*h(),0.))-linearRead(0xfffffffeu,false,pos+vec2(0.,.5*h()))+linearRead(0xfffffffeu,false,pos-vec2(0.,.5*h())))/h();
 float div=(fieldRead(0xfffffffeu,false,int(x+1u),int(y))-fieldRead(0xfffffffeu,false,int(x),int(y))+fieldRead(0xfffffffeu,true,int(x),int(y+1u))-fieldRead(0xfffffffeu,true,int(x),int(y)))/h();
 fields[0]=u;fields[1]=v;fields[2]=pi;fields[3]=phi;fields[4]=pi+phi-meanPhi+uintBitsToFloat(c[34]);fields[5]=curl;fields[6]=div;fields[7]=length(vec2(u,v));fields[8]=pos.x;fields[9]=pos.y;fields[10]=uintBitsToFloat(baseState[132]);fields[11]=c[520]>=50u?0.:sfloat(loadQ(work(13u),i),-32);
 // SOURCE has no pressure solve: rootKind=SOURCE makes this diagnostic
 // NOT_APPLICABLE, rather than borrowing a rejected QUANTUM WORK residual.
 flags=1u;
}
void packHeader(out uint header[64]){
 for(uint i=0u;i<64u;++i)header[i]=0u;
 header[0]=1446195527u;header[1]=1u;header[2]=1177760071u;header[3]=64u+16u*nc();header[4]=nx();header[5]=ny();header[6]=c[6];header[7]=c[514]==0u?1u:0u;
 for(uint i=0u;i<10u;++i)header[8u+i]=baseState[8u+i];
 header[18]=c[136];header[19]=c[137];header[20]=c[162];header[21]=c[163];header[22]=c[150];header[23]=c[151];
 for(uint i=0u;i<8u;++i){header[24u+i]=c[76u+i];header[32u+i]=c[44u+i];}
 header[40]=floatBitsToUint(0.);header[41]=floatBitsToUint(2.);header[42]=floatBitsToUint(-64.);header[43]=floatBitsToUint(64.);header[44]=floatBitsToUint(-256.);header[45]=floatBitsToUint(256.);header[46]=c[624];header[47]=baseState[150];header[48]=c[34];header[63]=0xc0de1301u;
}
void renderPack(){
 for(uint i=lane;i<nc();i+=64u){float fields[12];uint flags;packCell(i,fields,flags);uint p=64u+16u*i;
  for(uint j=0u;j<12u;++j){if(isnan(fields[j])||isinf(fields[j]))fail(16u);outState[p+j]=floatBitsToUint(fields[j]);}
  outState[p+12u]=flags;outState[p+13u]=i;uint x=i%nx(),y=i/nx();outState[p+14u]=uint(x==0u||x+1u==nx()||y==0u||y+1u==ny());outState[p+15u]=0u;
 }
 syncAll();if(lane==0u){uint header[64];packHeader(header);for(uint i=0u;i<63u;++i)outState[i]=header[i];memoryBarrierBuffer();outState[63]=header[63];}
}
void renderAudit(){
 uint expected[64];packHeader(expected);for(uint i=lane;i<64u;i+=64u)if(outState[i]!=expected[i])fail(2u);
 for(uint i=lane;i<nc();i+=64u){uint p=64u+16u*i;float fields[12];uint flags;packCell(i,fields,flags);for(uint j=0u;j<12u;++j)if(outState[p+j]!=floatBitsToUint(fields[j])||isnan(fields[j])||isinf(fields[j]))fail(16u);uint x=i%nx(),y=i/nx();if(outState[p+12u]!=flags||outState[p+13u]!=i||outState[p+14u]!=uint(x==0u||x+1u==nx()||y==0u||y+1u==ny())||outState[p+15u]!=0u)fail(35u);}
}
