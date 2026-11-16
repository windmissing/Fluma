#version 310 es
precision highp float;precision highp int;
layout(std430,binding=0)readonly buffer State{uint S[];};
uniform vec2 uResolution;uniform vec4 uBox;
out vec2 local;flat out uint mass;
void main(){
 const vec2 corners[6]=vec2[6](vec2(-1.,-1.),vec2(1.,-1.),vec2(-1.,1.),vec2(-1.,1.),vec2(1.,-1.),vec2(1.,1.));local=corners[gl_VertexID];uint at=70144u+uint(gl_InstanceID)*64u;mass=S[at];
 vec2 uv=vec2(float(S[at+1u])/134217728.,1.-float(S[at+2u])/536870912.);float radius=clamp(sqrt(float(mass)/1048576.)*3.,.75,3.);vec2 pixel=uBox.xy+uv*uBox.zw+local*radius;
 gl_Position=mass==0u?vec4(2.,2.,0.,1.):vec4(pixel.x/uResolution.x*2.-1.,1.-pixel.y/uResolution.y*2.,0.,1.);
}
