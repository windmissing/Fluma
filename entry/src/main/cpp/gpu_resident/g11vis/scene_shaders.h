// T566 (problem T563): g11vis scene shader sources. The base scene and
// packet shaders mirror the parent g9spf ABI (scene keeps State@0; packet
// uses three uint instance attributes with NO vertex SSBO) so the composite
// scene shares the parent draw contract; glyph shaders are g11vis-specific
// and are compiled inline in scene.cpp.
#pragma once
namespace grs::g11vis {
static constexpr const char *kG11CompositeSceneVertex = R"G11SC(#version 310 es
precision highp float;precision highp int;
uniform vec2 uResolution;
uniform vec4 uBox;
const vec2 kCorners[3]=vec2[3](vec2(0,0),vec2(2,0),vec2(0,2));
void main(){vec2 c=kCorners[gl_VertexID];gl_Position=vec4(c*2.0-1.0,0.0,1.0);}
)G11SC";
static constexpr const char *kG11CompositeSceneFragment =
    R"G11SF(#version 310 es
precision highp float;precision highp int;
layout(std430,binding=0)readonly buffer State{uint S[];};
uniform vec2 uResolution;
uniform vec4 uBox;
uniform int uN;
uniform highp uint uForeground;
layout(location=0) out vec4 color;
void main(){
 vec2 uv=(gl_FragCoord.xy-uBox.xy)/uBox.zw;
 if(any(lessThan(uv,vec2(0.0)))||any(greaterThanEqual(uv,vec2(1.0)))){color=vec4(0.04,0.05,0.06,1.0);return;}
 uint column=uint(clamp(floor(uv.x*float(uN)),0.0,float(uN-1)));uint at=512u+column*32u;
 float y=uv.y*256.0,bed=float(S[at])/1048576.0,top=bed+float(S[at+1u])/1048576.0;
 vec3 shade=y<bed?vec3(0.72,0.56,0.32):y<top?vec3(0.78,0.64,0.39):vec3(0.08,0.10,0.12);
 // Complete foreground boundary is part of the same shader frame.
 bool border=min(min(uv.x,uv.y),min(1.0-uv.x,1.0-uv.y))<0.003;
 if(uForeground!=0u&&!border)discard;
 if(border)shade=vec3(0.15,0.17,0.20);
 color=vec4(shade,1.0);
}
)G11SF";
static constexpr const char *kG11CompositePacketVertex =
    R"G11PV(#version 310 es
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
)G11PV";
static constexpr const char *kG11CompositePacketFragment =
    R"G11PF(#version 310 es
precision highp float;precision highp int;
in vec2 local;flat in float opacity;flat in uint mass;
layout(location=0) out vec4 color;
void main(){if(mass==0u||dot(local,local)>1.0)discard;color=vec4(vec3(0.78,0.64,0.39)*opacity,opacity);}
)G11PF";
} // namespace grs::g11vis
