#version 300 es
precision highp float;
precision highp int;
layout(location=0) in vec2 position;
layout(location=1) in uint tag;
uniform int layer;
uniform vec2 viewport;
uniform vec4 box;
uniform float capacity, radius;
uniform bool inverted;
out vec2 disc;
flat out vec2 center;
flat out uint state;
void main(){
  if(layer!=2){
    gl_Position=vec4(gl_VertexID==1?3.:-1.,gl_VertexID==2?3.:-1.,0.,1.);
    return;
  }
  const vec2 corners[6]=vec2[6](vec2(-1.,-1.),vec2(1.,-1.),vec2(-1.,1.),vec2(-1.,1.),vec2(1.,-1.),vec2(1.,1.));
  disc=corners[gl_VertexID];
  center=vec2(position.x/128.,1.-position.y/capacity)*box.zw;
  state=tag>>8;
  vec2 q=(inverted?box.zw-center:center)+box.xy+disc*radius;
  gl_Position=vec4(q.x/viewport.x*2.-1.,1.-q.y/viewport.y*2.,0.,1.);
}
