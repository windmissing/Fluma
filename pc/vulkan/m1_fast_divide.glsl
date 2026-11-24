bool grsDivide128(uvec4 n,uint d,out uvec4 quotient,out uint remainder) {
 if(d==0u)return false;
 quotient=uvec4(0u);
 int top=n.w!=0u?3:n.z!=0u?2:n.y!=0u?1:0;
 quotient[top]=n[top]/d;
 uint r=n[top]%d;
 // r<d. The radix leaves enough high bits for r and the next digit.
 int radix=31-findMSB(d);
 for(int limb=top-1;limb>=0;--limb) {
  if(r==0u){quotient[limb]=n[limb]/d;r=n[limb]%d;continue;}
  if(radix==0){
   for(int bit=31;bit>=0;--bit){
    bool high=(r>>31u)!=0u;
    r=(r<<1u)|((n[limb]>>uint(bit))&1u);
    if(high||r>=d){r-=d;quotient[limb]|=1u<<uint(bit);}
   }
   continue;
  }
  for(int left=32;left>0;){
   int width=min(radix,left);
   left-=width;
   uint digit=(n[limb]>>uint(left))&((1u<<uint(width))-1u);
   uint value=(r<<uint(width))|digit;
   quotient[limb]|=(value/d)<<uint(left);
   r=value%d;
  }
 }
 remainder=r;
 return true;
}
