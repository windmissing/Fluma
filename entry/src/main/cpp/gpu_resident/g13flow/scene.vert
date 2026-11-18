#version 310 es
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
