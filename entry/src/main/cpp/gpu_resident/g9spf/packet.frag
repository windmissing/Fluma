#version 310 es
precision highp float;precision highp int;
in vec2 local;flat in uint mass;out vec4 color;
void main(){if(mass==0u||dot(local,local)>1.)discard;color=vec4(.73,.56,.30,1.);}
