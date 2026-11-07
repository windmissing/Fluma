#version 300 es
precision highp float;
precision highp int;
out vec4 color;
vec3 linearRgb(vec3 c){
 return mix(c/12.92,pow((c+.055)/1.055,vec3(2.4)),step(vec3(.04045),c));
}
void main(){
 uvec2 p=uvec2(gl_FragCoord.xy);
 uint h=(p.x*73u+p.y*151u+(p.x/17u)*31u+(p.y/23u)*47u);
 uvec3 code=uvec3(32u+h%192u,32u+(p.x*37u+p.y*19u)%192u,32u+(p.x*11u+p.y*97u)%192u);
 color=vec4(linearRgb(vec3(code)/255.),1.);
}
