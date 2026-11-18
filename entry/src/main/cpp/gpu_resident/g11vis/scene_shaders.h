// T566 (problem T563): g11vis scene shader sources. The base scene and
// packet shaders mirror the parent g9spf ABI (scene keeps State@0; packet
// uses three uint instance attributes with NO vertex SSBO) so the composite
// scene shares the parent draw contract; glyph shaders are g11vis-specific
// and are compiled inline in scene.cpp.
#pragma once
namespace grs::g11vis {
static constexpr const char* kG11CompositeSceneVertex=R"G11SC(#version 310 es
precision highp int;
uniform vec2 uResolution;
uniform vec4 uBox;
const vec2 kCorners[3]=vec2[3](vec2(0,0),vec2(1,0),vec2(1,1));
void main(){vec2 c=kCorners[gl_VertexID];gl_Position=vec4(c*2.0-1.0,0.0,1.0);}
)G11SC";
static constexpr const char* kG11CompositeSceneFragment=R"G11SF(#version 310 es
precision highp int;
layout(std430,binding=0)readonly buffer State{uint S[];};
uniform vec2 uResolution;
uniform vec4 uBox;
uniform int uN;
layout(location=0) out vec4 color;
void main(){
 // Base bed/flow shading reads the real column32 words (S/F) of CURRENT.
 vec2 pixel=gl_FragCoord.xy/uResolution;
 uint column=uint(clamp(pixel.x,0.0,0.999)*128.0);
 uint at=512u+column*32u;
 uint bed=S[at+0u],flow=S[at+1u];
 uint total=bed+flow;
 float fill=total==0u?0.0:float(flow)/float(total);
 vec3 bedColor=vec3(0.72,0.56,0.32);
 vec3 flowColor=vec3(0.93,0.85,0.62);
 color=vec4(mix(bedColor,flowColor,fill),1.0);
}
)G11SF";
static constexpr const char* kG11CompositePacketVertex=R"G11PV(#version 310 es
precision highp int;
layout(location=0) in uint aMass;
layout(location=1) in uint aXQ20;
layout(location=2) in uint aYQ21;
uniform vec2 uResolution;
uniform vec4 uBox;
out float vShade;
void main(){
 // Packet instance: mass/xQ20/y2Q21 integers -> pixel quad via gl_VertexID.
 vec2 center=vec2(float(aXQ20)/1048576.0,float(aYQ21)/2097152.0);
 const vec2 kCorners[6]=vec2[6](vec2(0,0),vec2(1,0),vec2(1,1),vec2(0,0),vec2(1,1),vec2(0,1));
 vec2 corner=kCorners[gl_VertexID];
 vShade=clamp(float(aMass)/268435456.0,0.25,1.0);
 gl_Position=vec4((center+corner*0.01)*2.0-1.0,0.0,1.0);
}
)G11PV";
static constexpr const char* kG11CompositePacketFragment=R"G11PF(#version 310 es
precision highp int;
in float vShade;
layout(location=0) out vec4 color;
void main(){color=vec4(vec3(0.35,0.50,0.75)*vShade,1.0);}
)G11PF";
}
