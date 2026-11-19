#version 310 es
precision highp float;precision highp int;
layout(location=0) in uvec4 aGlyph0;
layout(location=1) in uvec4 aGlyph1;
uniform vec2 uResolution;
uniform vec4 uBox;
out vec2 vCenter;flat out vec2 vOld;flat out vec2 vNew;
flat out uint vKindFlags;
flat out float vBlend;
flat out uint vGhostMass;
flat out uint vAppearance;
const vec2 kCorners[6]=vec2[6](vec2(0,0),vec2(1,0),vec2(1,1),vec2(0,0),vec2(1,1),vec2(0,1));
void main(){
 vec2 oldPx=vec2(aGlyph0.xy)/vec2(16777216.0,8388608.0);
 vec2 newPx=vec2(aGlyph0.zw)/vec2(16777216.0,8388608.0);
 vec2 corner=kCorners[gl_VertexID];
 float ghostRadius=clamp(sqrt(float(aGlyph1.z)/1048576.0)*3.0,0.75,3.0);
 vec2 extent=((aGlyph1.x&3u)==3u)?max(vec2(0.015625),vec2(ghostRadius)*vec2(128.0,256.0)/uBox.zw):vec2(0.015625);
 vec2 center=mix(min(oldPx,newPx)-extent,max(oldPx,newPx)+extent,corner);
 vKindFlags=aGlyph1.x&127u;vBlend=float(aGlyph1.y)/65536.0;vGhostMass=aGlyph1.z;vAppearance=aGlyph1.w;
 vOld=oldPx;vNew=newPx;vCenter=center;
 vec2 pixel=uBox.xy+center/vec2(128.0,256.0)*uBox.zw;
 gl_Position=vec4(pixel/uResolution*2.0-1.0,0.0,1.0);
}
