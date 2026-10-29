#version 300 es
precision highp float;precision highp int;
in vec2 local;flat in uint mass;uniform vec2 uResolution;uniform vec4 uBox;out vec4 color;
vec3 linearize(vec3 c){return mix(c/12.92,pow((c+.055)/1.055,vec3(2.4)),step(vec3(.04045),c));}
void main(){vec2 q=vec2(gl_FragCoord.x,uResolution.y-gl_FragCoord.y)-uBox.xy;if(mass==0u||dot(local,local)>1.||any(lessThan(q,vec2(0.)))||any(greaterThanEqual(q,uBox.zw)))discard;color=vec4(linearize(vec3(.72,.57,.35)),1.);}
