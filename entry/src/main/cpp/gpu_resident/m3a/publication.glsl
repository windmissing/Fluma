// T522 V4 publication. Source template is retained in M1 history.
uint m3PubWater(){return 16u*uK;}
uint m3PubColumn(){return m3PubWater()+20u*uKw;}
uint m3PubHeader(){return m3PubColumn()+2048u;}
void m3PackPublication(){
 if(W[0]!=0u)return;uint end=m3PubHeader()+256u;for(uint i=0u;i<end;i++)E[i]=0u;
 uint sandRank=0u,waterRank=0u;
 for(uint i=0u;i<uK;i++)if(C[i]!=0u){uint a=sandRank++;E[a]=C[5u*uK+i];E[uK+a]=C[10u*uK+i];E[2u*uK+a]=C[7u*uK+i];E[3u*uK+a]=C[i];E[4u*uK+a]=C[uK+i];E[5u*uK+a]=C[2u*uK+i];E[6u*uK+a]=C[3u*uK+i];E[7u*uK+a]=C[4u*uK+i];E[8u*uK+a]=grsColumnPreservingFloatBits(C[uK+i]);E[9u*uK+a]=grsQ20FloatBits(C[2u*uK+i]);E[10u*uK+a]=0x3fc00000u;E[11u*uK+a]=C[uK+i]>>20u;E[12u*uK+a]=C[6u*uK+i]&15u;E[14u*uK+a]=C[8u*uK+i];E[15u*uK+a]=C[9u*uK+i];}
 for(uint i=0u;i<uKw;i++)if(C[uWater+i]!=0u){uint a=waterRank++;for(uint f=0u;f<16u;f++)E[m3PubWater()+f*uKw+a]=C[uWater+f*uKw+i];E[m3PubWater()+16u*uKw+a]=grsColumnPreservingFloatBits(C[uWater+uKw+i]);E[m3PubWater()+17u*uKw+a]=grsQ20FloatBits(C[uWater+2u*uKw+i]);E[m3PubWater()+18u*uKw+a]=1u;}
 for(uint c=0u;c<128u;c++){uint s=C[uColumn+10u*128u+c],w=C[uColumn+6u*128u+c],j=C[uColumn+7u*128u+c],p=C[uColumn+8u*128u+c],eta=s+p+w,r=eta+j;uint values[16]=uint[16](s,w,eta,j,p,C[uColumn+c],C[uColumn+2u*128u+c],r,grsQ20FloatBits(s),grsQ20FloatBits(eta),grsQ20FloatBits(w),grsQ20FloatBits(r),w>0u?1u:0u,c,grsQ20FloatBits(s+p),0u);for(uint f=0u;f<16u;f++)E[m3PubColumn()+f*128u+c]=values[f];}
 uint h=m3PubHeader();E[h]=1347637831u;E[h+1u]=4u;E[h+2u]=256u;E[h+3u]=3235823838u;
 for(uint i=0u;i<8u;i++)E[h+4u+i]=C[uHeader+4u+i];for(uint i=0u;i<19u;i++)E[h+12u+i]=C[uHeader+14u+i];E[h+31u]=uM3CandidateBank;E[h+32u]=C[uHeader+72u];E[h+33u]=C[uHeader+73u];E[h+34u]=C[uHeader+41u];E[h+35u]=C[uHeader+42u];E[h+36u]=128u;E[h+37u]=uFrame.x;E[h+38u]=uFrame.y;E[h+39u]=C[uHeader+72u];E[h+40u]=C[uHeader+73u];for(uint i=0u;i<24u;i++)E[h+41u+i]=C[uHeader+88u+i];
 // All words are verified by a separate dispatch before completion seal.
}
bool m3AuditPublication(){
 if(W[0]!=0u)return false;
 uint sandRank=0u,waterRank=0u;
 for(uint i=0u;i<uK;i++)if(C[i]!=0u){uint s=sandRank++;uint v[16]=uint[16](C[5u*uK+i],C[10u*uK+i],C[7u*uK+i],C[i],C[uK+i],C[2u*uK+i],C[3u*uK+i],C[4u*uK+i],grsColumnPreservingFloatBits(C[uK+i]),grsQ20FloatBits(C[2u*uK+i]),0x3fc00000u,C[uK+i]>>20u,C[6u*uK+i]&15u,0u,C[8u*uK+i],C[9u*uK+i]);for(uint f=0u;f<16u;f++)if(E[f*uK+s]!=v[f])return false;}
 for(uint i=sandRank;i<uK;i++)for(uint f=0u;f<16u;f++)if(E[f*uK+i]!=0u)return false;
 for(uint i=0u;i<uKw;i++)if(C[uWater+i]!=0u){uint s=waterRank++;for(uint f=0u;f<16u;f++)if(E[m3PubWater()+f*uKw+s]!=C[uWater+f*uKw+i])return false;if(E[m3PubWater()+16u*uKw+s]!=grsColumnPreservingFloatBits(C[uWater+uKw+i])||E[m3PubWater()+17u*uKw+s]!=grsQ20FloatBits(C[uWater+2u*uKw+i])||E[m3PubWater()+18u*uKw+s]!=1u||E[m3PubWater()+19u*uKw+s]!=0u)return false;}
 for(uint i=waterRank;i<uKw;i++)for(uint f=0u;f<20u;f++)if(E[m3PubWater()+f*uKw+i]!=0u)return false;
 for(uint c=0u;c<128u;c++){uint s=C[uColumn+10u*128u+c],w=C[uColumn+6u*128u+c],j=C[uColumn+7u*128u+c],p=C[uColumn+8u*128u+c],eta=s+p+w,r=eta+j;uint v[16]=uint[16](s,w,eta,j,p,C[uColumn+c],C[uColumn+2u*128u+c],r,grsQ20FloatBits(s),grsQ20FloatBits(eta),grsQ20FloatBits(w),grsQ20FloatBits(r),w>0u?1u:0u,c,grsQ20FloatBits(s+p),0u);for(uint f=0u;f<16u;f++)if(E[m3PubColumn()+f*128u+c]!=v[f])return false;}
 uint h=m3PubHeader();if(E[h]!=1347637831u||E[h+1u]!=4u||E[h+2u]!=256u||E[h+3u]!=3235823838u||E[h+31u]!=uM3CandidateBank||E[h+32u]!=C[uHeader+72u]||E[h+33u]!=C[uHeader+73u]||E[h+34u]!=sandRank||E[h+35u]!=waterRank||E[h+36u]!=128u||E[h+37u]!=uFrame.x||E[h+38u]!=uFrame.y||E[h+39u]!=C[uHeader+72u]||E[h+40u]!=C[uHeader+73u])return false;
 for(uint i=0u;i<8u;i++)if(E[h+4u+i]!=C[uHeader+4u+i])return false;for(uint i=0u;i<19u;i++)if(E[h+12u+i]!=C[uHeader+14u+i])return false;for(uint i=0u;i<24u;i++)if(E[h+41u+i]!=C[uHeader+88u+i])return false;for(uint i=65u;i<256u;i++)if(E[h+i]!=0u)return false;
 return true;
}
