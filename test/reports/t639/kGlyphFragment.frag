#version 310 es
precision highp float;precision highp int;
layout(std430,binding=0)readonly buffer State{uint S[];};
uniform vec2 uResolution;
uniform vec4 uBox;
uniform int uN;
in vec2 vCenter;flat in vec2 vOld;flat in vec2 vNew;
flat in uint vKindFlags;
flat in float vBlend;
flat in uint vGhostMass;
flat in uint vAppearance;
layout(location=0) out vec4 color;
void main(){
 vec2 point=(gl_FragCoord.xy-uBox.xy)/uBox.zw*vec2(128.0,256.0);
 uint oldKind=vKindFlags&3u,newKind=(vKindFlags>>2u)&3u;
 float ghostRadius=clamp(sqrt(float(vGhostMass)/1048576.0)*3.0,0.75,3.0);
 bool oldDisk=(vKindFlags&16u)!=0u&&(oldKind==3u?distance((point-vOld)*uBox.zw/vec2(128.0,256.0),vec2(0))<=ghostRadius:distance(point,vOld)<=0.015625);
 bool newDisk=(vKindFlags&32u)!=0u&&distance(point,vNew)<=0.015625;
 uint column=uint(clamp(floor(point.x),0.0,127.0));uint at=512u+column*32u;
 float bed=float(S[at])/1048576.0,flow=float(S[at+1u])/1048576.0;
 if(oldKind==1u)oldDisk=oldDisk&&point.y>=max(0.0,bed-0.125)&&point.y<bed&&flow==0.0;
 if(newKind==1u)newDisk=newDisk&&point.y>=max(0.0,bed-0.125)&&point.y<bed&&flow==0.0;
 if(oldKind==2u)oldDisk=oldDisk&&point.y>=bed&&point.y<bed+flow;
 if(newKind==2u)newDisk=newDisk&&point.y>=bed&&point.y<bed+flow;
 if(oldKind==3u&&(vKindFlags&64u)==0u)oldDisk=false;
 if(newKind==3u)newDisk=false; // true physical P body is drawn by the packet consumer.
 float alpha=oldKind==3u?(oldDisk?1.0-vBlend:0.0):min(0.5,(oldDisk?1.0-vBlend:0.0)+(newDisk?vBlend:0.0));if(alpha==0.0)discard;
 color=vec4(vec3(0.82,0.72,0.48)*alpha,alpha);
}
