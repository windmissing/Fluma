#version 310 es
precision highp float;precision highp int;
layout(std430,binding=0)readonly buffer State{uint S[];};
uniform vec2 uResolution;
uniform vec4 uBox;
uniform int uN;
uniform highp uint uForeground;
layout(location=0) out vec4 color;
void main(){
 vec2 uv=(gl_FragCoord.xy-uBox.xy)/uBox.zw;
 if(any(lessThan(uv,vec2(0.0)))||any(greaterThanEqual(uv,vec2(1.0)))){color=vec4(0.04,0.05,0.06,1.0);return;}
 uint column=uint(clamp(floor(uv.x*float(uN)),0.0,float(uN-1)));uint at=512u+column*32u;
 float y=uv.y*256.0,bed=float(S[at])/1048576.0,top=bed+float(S[at+1u])/1048576.0;
 vec3 shade=y<bed?vec3(0.72,0.56,0.32):y<top?vec3(0.78,0.64,0.39):vec3(0.08,0.10,0.12);
 // Complete foreground boundary is part of the same shader frame.
 bool border=min(min(uv.x,uv.y),min(1.0-uv.x,1.0-uv.y))<0.003;
 if(uForeground!=0u&&!border)discard;
 if(border)shade=vec3(0.15,0.17,0.20);
 color=vec4(shade,1.0);
}
