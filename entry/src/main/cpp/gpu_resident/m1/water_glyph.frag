#version 310 es
precision highp float;precision highp int;
in vec2 local;flat in uint mass;uniform vec2 uResolution;uniform vec4 uBox;uniform vec4 uOptics[4];out vec4 color;
void main(){vec2 p=vec2(gl_FragCoord.x,uResolution.y-gl_FragCoord.y)-uBox.xy;if(mass==0u||any(lessThan(p,vec2(0.)))||any(greaterThanEqual(p,uBox.zw)))discard;float r=length(local),a=1.-smoothstep(.8,1.,r);if(a==0.)discard;float g=pow(max(0.,1.-length(local-vec2(-.25,-.25))),8.);color=vec4(a*mix(uOptics[0].yzw,uOptics[2].yzw,g)*.8,a*.8);}
