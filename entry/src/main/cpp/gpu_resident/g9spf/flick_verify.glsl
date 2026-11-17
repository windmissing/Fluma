// T543 integer-only verification; no FP64, new dispatch or binding.
bool g9FlickVelocity(uvec4 magnitude,uvec4 duration,uint cap,out uint velocity,out bool saturated){
 uvec4 product,denominator,value;
 if(!g9UMul(magnitude,g9U32(1000000000u),product)||!g9UMul(duration,g9U32(4u),denominator)||!g9SDiv(product,denominator,1u,value))return false;
 saturated=g9UCmp(value,g9U32(cap))>0;velocity=saturated?cap:value.x;return true;
}
bool g9FlickVerify(){
 uint kind=g9Input(40u),adapter=g9Input(77u),flags=g9Input(45u);
 if(kind!=2u)return flags==0u;
 if(adapter==0u)return flags==0u;
 if(adapter!=53401u||(flags&4u)==0u||(flags&~7u)!=0u||g9Input(48u)!=0u||g9Input(49u)!=0x40100000u)return false;
 uvec2 xb=g9InputPair(50u),yb=g9InputPair(52u);uvec2 ax=uvec2(xb.x,xb.y&0x7fffffffu),ay=uvec2(yb.x,yb.y&0x7fffffffu);
 if(ax.y>0x40600000u||(ax.y==0x40600000u&&ax.x!=0u)||ay.y>0x40700000u||(ay.y==0x40700000u&&ay.x!=0u))return false;
 uvec4 dx,dy,xx,yy,norm,duration=g9U64(g9InputPair(54u));
 if(g9UCmp(duration,g9U32(8333334u))<0||g9UCmp(duration,g9U32(1000000000u))>0||!g9DecodeBinary64(xb,20u,false,dx)||!g9DecodeBinary64(yb,20u,false,dy)||!g9UMul(g9Magnitude(dx),g9Magnitude(dx),xx)||!g9UMul(g9Magnitude(dy),g9Magnitude(dy),yy)||!g9UAdd(xx,yy,norm)||g9UCmp(norm,uvec4(0u,16u,0u,0u))<0)return false;
 uint vx,vy;bool sx,sy;if(!g9FlickVelocity(g9Magnitude(dx),duration,25165824u,vx,sx)||!g9FlickVelocity(g9Negative(dy)?uvec4(0u):dy,duration,33554432u,vy,sy))return false;
 if(flags!=(4u|(sx?1u:0u)|(sy?2u:0u)))return false;
 uvec4 jx,jy,work,squares;if(!g9UMul(g9U32(vx),g9U32(65536u),jx)||!g9FromMagnitude(jx,g9Negative(dx),jx)||!g9UMul(g9U32(vy),g9U32(65536u),jy)||g9Zero(jx)&&g9Zero(jy))return false;
 if(any(notEqual(jx,g9S64(g9InputPair(56u))))||any(notEqual(jy,g9U64(g9InputPair(58u)))))return false;
 if(!g9UMul(g9U32(vx),g9U32(vx),xx)||!g9UMul(g9U32(vy),g9U32(vy),yy)||!g9UAdd(xx,yy,squares)||!g9UShift(squares,43u,work))return false;
 return all(equal(work,uvec4(g9Input(60u),g9Input(61u),g9Input(62u),g9Input(63u))));
}
