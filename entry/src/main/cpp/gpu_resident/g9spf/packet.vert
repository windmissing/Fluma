#version 310 es
// G9_PACKET_ATTR_U32_V2 (problem T560 §2): the same CURRENT state buffer that
// compute writes is bound as GL_ARRAY_BUFFER by the host; three unsigned
// integer instance attributes carry mass (loc0, +0), xQ20 (loc1, +4) and
// y2Q21 (loc2, +8) with stride 256. No vertex-stage SSBO is declared, so
// SCENE_ATTR eligibility needs zero vertex shader-storage blocks.
precision highp float;precision highp int;
layout(location=0)in uint aMass;layout(location=1)in uint aXQ20;layout(location=2)in uint aYQ21;
uniform vec2 uResolution;uniform vec4 uBox;
out vec2 local;flat out uint mass;
void main(){
 const vec2 corners[6]=vec2[6](vec2(-1.,-1.),vec2(1.,-1.),vec2(-1.,1.),vec2(-1.,1.),vec2(1.,-1.),vec2(1.,1.));
 local=corners[gl_VertexID];mass=aMass;
 vec2 uv=vec2(float(aXQ20)/134217728.,1.-float(aYQ21)/536870912.);
 float radius=clamp(sqrt(float(mass)/1048576.)*3.,.75,3.);
 vec2 pixel=uBox.xy+uv*uBox.zw+local*radius;
 gl_Position=mass==0u?vec4(2.,2.,0.,1.):vec4(pixel.x/uResolution.x*2.-1.,1.-pixel.y/uResolution.y*2.,0.,1.);
}
