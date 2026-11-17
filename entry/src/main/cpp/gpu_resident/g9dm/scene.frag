#version 310 es
precision highp float;precision highp int;
layout(std430,binding=0)readonly buffer State{uint S[];};
uniform vec2 uResolution;uniform vec4 uBox;uniform int uLayer;uniform sampler2D uFrame;
out vec4 color;
void main(){
 if(uLayer==2){color=texelFetch(uFrame,ivec2(gl_FragCoord.xy),0);return;}
 vec2 q=vec2(gl_FragCoord.x,uResolution.y-gl_FragCoord.y)-uBox.xy;color=vec4(.012,.016,.025,1.);
 if(any(lessThan(q,vec2(0.)))||any(greaterThanEqual(q,uBox.zw)))return;
 uint column=uint(clamp(floor(q.x/uBox.z*128.),0.,127.)),at=512u+column*32u;float y=(1.-q.y/uBox.w)*256.;
 float bed=float(S[at])/1048576.,flow=float(S[at+1u])/1048576.;
 if(y<bed){color=vec4(.48,.34,.17,1.);return;}
 if(y<bed+flow){float momentum=float(int(S[at+3u]))*4294967296.+float(S[at+2u]);float speed=S[at+1u]==0u?0.:momentum/float(S[at+1u])/1048576.;float streak=sin((q.x+q.y*.17)*.8+speed*.05)*.025;color=vec4(vec3(.66,.49,.25)+streak,1.);}
}
