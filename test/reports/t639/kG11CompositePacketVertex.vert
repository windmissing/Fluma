#version 310 es
precision highp float;precision highp int;
layout(location=0) in uint aMass;
layout(location=1) in uint aXQ20;
layout(location=2) in uint aYQ21;
layout(location=3) in uvec4 aStyle;
layout(location=4) in uint aId;
layout(location=5) in uint aGeneration;
uniform vec2 uResolution;
uniform vec4 uBox;
out vec2 local;flat out float opacity;flat out uint mass;
void main(){
 const vec2 corners[6]=vec2[6](vec2(-1,-1),vec2(1,-1),vec2(-1,1),vec2(-1,1),vec2(1,-1),vec2(1,1));local=corners[gl_VertexID];mass=aMass;
 opacity=aStyle.x==aId&&aStyle.y==aGeneration?float(min(aStyle.z,65536u))/65536.0:1.0;
 // Missing sample/style cannot hide a real physical packet.
 float radius=clamp(sqrt(float(aMass)/1048576.0)*3.0,0.75,3.0);
 vec2 center=vec2(float(aXQ20)/134217728.0,float(aYQ21)/536870912.0);
 vec2 pixel=uBox.xy+center*uBox.zw+local*radius;
 gl_Position=aMass==0u?vec4(2,2,0,1):vec4(pixel/uResolution*2.0-1.0,0,1);
}
