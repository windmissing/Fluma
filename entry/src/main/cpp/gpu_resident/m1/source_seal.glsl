uint sourceEvents(){return spare()+uWords;}
uint pubWater(){return 16u*uK;}uint pubColumn(){return pubWater()+20u*uKw;}uint pubHeader(){return pubColumn()+16u*128u;}
bool auditSourceDomains();bool auditPublication();
void finalizeSource(){
 if(W[S]==4u)return;
 if(W[S+14u]!=31u){fail(15u);return;}
 if(!addHeader(72u,1u))return;
 C[uHeader+12u]=uNonce.x;C[uHeader+13u]=uNonce.y;C[uHeader+18u]=uControl.x;C[uHeader+19u]=uControl.y;C[uHeader+49u]=uFrame.x;C[uHeader+50u]=uFrame.y;
 C[uHeader+76u]=1u-uBaseBank;C[uHeader+77u]=uBaseBank;C[uHeader+78u]=2u;C[uHeader+79u]=1u;C[uHeader+80u]=W[S];C[uHeader+81u]=2u;C[uHeader+47u]=uCarry;C[uHeader+38u]=W[S+9u];
 if(W[S]==1u){uint at=sourceEvents()+B[uHeader+34u]*16u;for(uint i=0u;i<16u;i++)W[at+i]=0u;
  W[at]=W[6];W[at+1u]=W[7];W[at+2u]=C[uHeader+14u];W[at+3u]=C[uHeader+15u];W[at+4u]=W[2];W[at+5u]=W[S+5u];W[at+6u]=W[S+3u];W[at+7u]=W[T+7u];W[at+8u]=W[T+6u];W[at+9u]=W[14];W[at+10u]=W[S+6u];W[at+11u]=W[S+7u];W[at+12u]=W[T+8u];W[at+13u]=W[T+9u];
 }
 if(!auditSourceDomains()){fail(15u);return;}W[S+14u]|=32u;
}
void packPublication(){
 if(W[S]==4u)return;uint end=pubHeader()+256u;for(uint i=0u;i<end;i++)E[i]=0u;
 uint sandRank=0u,waterRank=0u;
 for(uint i=0u;i<uK;i++)if(C[i]!=0u){uint a=sandRank++;E[a]=C[5u*uK+i];E[uK+a]=C[10u*uK+i];E[2u*uK+a]=C[7u*uK+i];E[3u*uK+a]=C[i];E[4u*uK+a]=C[uK+i];E[5u*uK+a]=C[2u*uK+i];E[6u*uK+a]=C[3u*uK+i];E[7u*uK+a]=C[4u*uK+i];E[8u*uK+a]=grsColumnPreservingFloatBits(C[uK+i]);E[9u*uK+a]=grsQ20FloatBits(C[2u*uK+i]);E[10u*uK+a]=0x3fc00000u;E[11u*uK+a]=C[uK+i]>>20u;E[12u*uK+a]=C[6u*uK+i]&15u;E[14u*uK+a]=C[8u*uK+i];E[15u*uK+a]=C[9u*uK+i];}
 for(uint i=0u;i<uKw;i++)if(C[uWater+i]!=0u){uint a=waterRank++;for(uint f=0u;f<16u;f++)E[pubWater()+f*uKw+a]=C[uWater+f*uKw+i];E[pubWater()+16u*uKw+a]=grsColumnPreservingFloatBits(C[uWater+uKw+i]);E[pubWater()+17u*uKw+a]=grsQ20FloatBits(C[uWater+2u*uKw+i]);E[pubWater()+18u*uKw+a]=1u;}
 for(uint c=0u;c<128u;c++){uint s=C[uColumn+10u*128u+c],w=C[uColumn+6u*128u+c],eta=s+w,j=C[uColumn+7u*128u+c],p=C[uColumn+8u*128u+c],r=eta+j+p;uint values[16]=uint[16](s,w,eta,j,p,C[uColumn+c],C[uColumn+2u*128u+c],r,grsQ20FloatBits(s),grsQ20FloatBits(eta),grsQ20FloatBits(w),grsQ20FloatBits(r),w>0u?1u:0u,c,0u,0u);for(uint f=0u;f<16u;f++)E[pubColumn()+f*128u+c]=values[f];}
 uint h=pubHeader();E[h]=1347637831u;E[h+1u]=2u;E[h+2u]=256u;E[h+3u]=3235823838u;
 for(uint i=0u;i<8u;i++)E[h+4u+i]=C[uHeader+4u+i];for(uint i=0u;i<19u;i++)E[h+12u+i]=C[uHeader+14u+i];E[h+31u]=1u-uBaseBank;E[h+32u]=C[uHeader+72u];E[h+33u]=C[uHeader+73u];E[h+34u]=C[uHeader+41u];E[h+35u]=C[uHeader+42u];E[h+36u]=128u;E[h+37u]=uFrame.x;E[h+38u]=uFrame.y;E[h+39u]=C[uHeader+72u];E[h+40u]=C[uHeader+73u];for(uint i=0u;i<24u;i++)E[h+41u+i]=C[uHeader+88u+i];
 // All words are verified by a separate dispatch before completion seal.
}
void sealCompletion(){
 if(W[S]!=4u&&(W[S+14u]!=127u||W[S+12u]!=1u||W[S+15u]!=1023u))fail(15u);
 for(uint i=0u;i<256u;i++)E[i]=0u;E[0]=1381192263u;E[1]=2u;E[2]=256u;E[4]=uNonce.x;E[5]=uNonce.y;
 for(uint i=0u;i<8u;i++)E[6u+i]=C[uHeader+4u+i];for(uint i=0u;i<4u;i++){E[14u+i]=B[uHeader+14u+i];E[18u+i]=C[uHeader+14u+i];}
 E[22]=uControl.x;E[23]=uControl.y;E[24]=uFrame.x;E[25]=uFrame.y;E[27]=1u;E[28]=W[S];E[29]=W[S+1u];E[30]=W[S]==4u?9u:0u;E[32]=uBaseBank;E[33]=1u-uBaseBank;E[34]=2u;E[35]=uPublicationSlot;E[36]=C[uHeader+72u];E[37]=C[uHeader+73u];for(uint i=0u;i<13u;i++)E[38u+i]=C[uHeader+20u+i];
 E[51]=uEventArena;E[53]=W[S+9u];E[54]=uK+1u;E[55]=B[uHeader+34u];E[56]=C[uHeader+34u];E[57]=B[uHeader+33u];E[58]=C[uHeader+33u];E[59]=W[14];E[60]=W[15];E[61]=W[T+3u];E[62]=W[T+4u];E[63]=W[T];E[64]=W[T+1u];E[65]=W[T+2u];E[66]=W[T+6u];E[67]=W[T+7u];E[68]=W[T+9u];
 for(uint j=0u;j<2u;j++){E[69u+j]=B[uHeader+56u+j];E[71u+j]=C[uHeader+56u+j];E[73u+j]=B[uHeader+58u+j];E[75u+j]=C[uHeader+58u+j];E[77u+j]=C[uHeader+60u+j];E[79u+j]=C[uHeader+62u+j];E[83u+j]=C[uHeader+54u+j];E[138u+j]=B[uHeader+112u+j];E[140u+j]=C[uHeader+112u+j];E[142u+j]=B[uHeader+114u+j];E[144u+j]=C[uHeader+114u+j];E[146u+j]=B[uHeader+116u+j];E[148u+j]=C[uHeader+116u+j];E[150u+j]=C[uHeader+118u+j];E[152u+j]=C[uHeader+120u+j];}
 E[88]=C[uHeader+41u];E[89]=C[uHeader+43u];E[90]=C[uHeader+42u];E[91]=C[uHeader+44u];E[92]=C[uHeader+39u];E[93]=C[uHeader+40u];E[96]=W[S+9u];E[97]=W[S]==4u?W[S+14u]:255u;E[99]=W[S+15u];E[101]=W[S]==4u?W[S+11u]:0xffffffffu;E[106]=W[S]==2u||W[S]==3u?1u:0u;E[107]=C[uHeader+45u];E[108]=uCarry;E[110]=8u;E[111]=9u;E[112]=uUploaded;for(uint i=0u;i<24u;i++)E[114u+i]=C[uHeader+88u+i];E[154]=W[2]==1u?W[S+9u]:0u;E[158]=W[T+30u];E[159]=W[2]==1u?W[T+7u]:0u;
 for(uint c=0u;c<128u;c++)E[87]=max(E[87],C[uColumn+13u*128u+c]);
 E[3]=3235823838u;
}
