#version 310 es
precision highp float;precision highp int;
uniform vec2 uResolution;
uniform vec4 uBox;
const vec2 kCorners[3]=vec2[3](vec2(0,0),vec2(2,0),vec2(0,2));
void main(){vec2 c=kCorners[gl_VertexID];gl_Position=vec4(c*2.0-1.0,0.0,1.0);}
