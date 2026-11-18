#pragma once
inline constexpr const char kG13Vertex[]=R"G13SCENE(#version 310 es
precision highp float;
precision highp int;
layout(location=0) in uvec4 a0;
layout(location=1) in uvec4 a1;
layout(location=2) in uvec4 a2;
layout(location=3) in uvec4 a3;
layout(location=4) in uvec4 a4;
layout(location=5) in uvec4 a5;
uniform uint uDraw,uMode;
uniform vec2 uGrid;
uniform vec2 uBoxSize;
uniform vec4 uBox;
uniform vec2 uPose;
uniform vec4 uRange;
out vec2 uv;
out vec3 tint;
void main(){uint id=uint(gl_VertexID);vec2 p=vec2((id<<1u)&2u,id&2u);uv=p;float value=uintBitsToFloat(a1.w);if(uMode==2u)value=uintBitsToFloat(a1.y);if(uMode==3u)value=uintBitsToFloat(a0.z);if(uMode==4u)value=uintBitsToFloat(a1.x);float fraction=clamp((value-uRange.x)/(uRange.y-uRange.x),0.,1.);tint=mix(vec3(.02,.12,.34),vec3(.98,.48,.12),fraction);
 if(uDraw==1u){vec2 corner=vec2(id&1u,id>>1u);p=uintBitsToFloat(a2.xy)/uBoxSize+(corner-.5)/uGrid;if(a3.x==0u)p=vec2(-10.);}
 else if(uDraw==2u){vec2 xy=(vec2(float(a3.y%uint(uGrid.x)),float(a3.y/uint(uGrid.x)))+.5)/uGrid;vec2 velocity=uintBitsToFloat(a0.xy);vec2 tip=xy+velocity*uRange.z/uBoxSize;vec2 normal=normalize(vec2(-velocity.y,velocity.x)+vec2(1e-20,0.));p=id==0u?xy:(id==1u?tip:(id==2u?tip-normal*.003:(id==3u||id==4u?tip:tip+normal*.003)));tint=vec3(.94);}
 else if(uDraw==3u){vec2 xy=uintBitsToFloat(a4.xy)/uBoxSize;vec2 corner=vec2(id&1u,id>>1u)-.5;p=xy+corner*uRange.w/uBoxSize;if(a5.y!=1u||a5.w!=0u)p=vec2(-10.);tint=vec3(.92,.95,.42);}
 else if(uDraw==4u){p=vec2(id==1u||id==2u?1.:0.,id>=2u?1.:0.);tint=vec3(.7);}
 if(uDraw>=1u&&uDraw<=4u){vec2 q=(p-.5)*uBoxSize;p=vec2(uPose.x*q.x-uPose.y*q.y,uPose.y*q.x+uPose.x*q.y)/uBoxSize+.5;p=uBox.xy+p*(uBox.zw-uBox.xy);}gl_Position=vec4(p*2.-1.,0.,1.);}
)G13SCENE";
inline constexpr const char kG13Fragment[]=R"G13SCENE(#version 310 es
precision highp float;
precision highp int;
uniform uint uDraw;
uniform sampler2D uFrame;
uniform vec4 uRange;
in vec2 uv;
in vec3 tint;
layout(location=0) out vec4 color;
uint digitMask(int n){const uint digits[10]=uint[10](31599u,11415u,29671u,29647u,23497u,31183u,31215u,29257u,31727u,31695u);return digits[clamp(n,0,9)];}
bool numberGlyph(vec2 at,float value){int cell=int(floor(at.x));vec2 local=vec2(fract(at.x),at.y);if(cell<0||cell>=8||local.x>=.75||local.y<0.||local.y>=1.)return false;ivec2 pixel=ivec2(floor(vec2(local.x/.75,1.-local.y)*vec2(3.,5.)));if(cell==0)return value<0.&&pixel.y==2;float a=abs(value);if(a>9999.)return pixel.x==1||pixel.y==2;int digit=0;if(cell>=1&&cell<=4){int divisor=cell==1?1000:cell==2?100:cell==3?10:1;digit=(int(a)/divisor)%10;if(cell<4&&int(a)<divisor)return false;}else if(cell==5)return pixel.y==4&&pixel.x==1;else {int divisor=cell==6?10:100;digit=int(fract(a)*float(divisor))%10;}return (digitMask(digit)&(1u<<uint(pixel.y*3+pixel.x)))!=0u;}
void main(){if(uDraw==0u){vec3 background=vec3(.015,.02,.035);if(uv.y>.015&&uv.y<.035&&uv.x>.28&&uv.x<.72){float fraction=(uv.x-.28)/.44;background=mix(vec3(.02,.12,.34),vec3(.98,.48,.12),fraction);if(fract(fraction*4.)<.01)background=vec3(.94);}if(numberGlyph(vec2((uv.x-.04)/.018,(uv.y-.01)/.025),uRange.x)||numberGlyph(vec2((uv.x-.78)/.018,(uv.y-.01)/.025),uRange.y))background=vec3(.94);color=vec4(background,1.);return;}color=uDraw==5u?texture(uFrame,uv):(uDraw==0u?vec4(.015,.02,.035,1.):vec4(tint,1.));}
)G13SCENE";
