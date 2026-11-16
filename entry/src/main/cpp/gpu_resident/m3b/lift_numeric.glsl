// T506 exact old-energy request. No floating point or shader int64 extension.
bool v5Energy(uint E0,uint signal,uint mode,uint ramp,uint decay,out uint E1){
 if(E0>65536u||signal>65536u||mode>2u||ramp==0u||ramp>7864320u||decay==0u||decay>62914560u)return false;
 if(mode==0u){E1=E0;return true;}
 if(signal!=0u){uint step=ramp/120u;E1=signal>E0?E0+min(signal-E0,step):E0-min(E0-signal,step);return true;}
 uvec4 quotient;uint remainder;
 if(!grsDivide128(uvec4(grsMul32(E0,7864320u),0u,0u),7864320u+decay,quotient,remainder)||any(notEqual(quotient.yzw,uvec3(0u))))return false;
 E1=quotient.x;return true;
}
bool v5Request(uint E0,uint entryBed,uint band,uint rate,out uint q){
 if(E0>65536u||entryBed>268435456u||band==0u||band>1048576u||rate>62914560u)return false;
 uvec4 numerator=grsMul64(grsMul32(E0,min(entryBed,band)),uvec2(rate,0u)),quotient;uint remainder;
 if(!grsDivide128(numerator,120u,quotient,remainder)||any(notEqual(quotient.zw,uvec2(0u))))return false;
 // floor(floor(n/120)/2^32) = floor(n/(120*2^32)).
 q=quotient.y;return true;
}
bool v5LiftVelocity(uint speed,int sinQ,int cosQ,out int vx,out int vy){
 if(speed==0u||speed>67108864u||sinQ< -1048576||sinQ>1048576||cosQ<=0||cosQ>1048576)return false;
 int horizontal;if(!grsMulDivRNE(int(speed),sinQ,1048576u,horizontal)||!grsMulDivRNE(int(speed),cosQ,1048576u,vy))return false;
 vx=-horizontal;return true;
}
