#version 310 es
precision highp float;precision highp int;
in vec2 local;flat in float opacity;flat in uint mass;
layout(location=0) out vec4 color;
void main(){if(mass==0u||dot(local,local)>1.0)discard;color=vec4(vec3(0.78,0.64,0.39)*opacity,opacity);}
