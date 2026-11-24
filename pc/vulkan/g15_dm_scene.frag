#version 450
layout(set=0,binding=0,std430) readonly buffer State { uint S[]; };
layout(set=0,binding=1,std430) readonly buffer Witness { uint W[]; };
layout(location=0) in vec2 uv;
layout(location=0) out vec4 color;

void main(){
    const float Q=1048576.;
    vec2 pos=vec2(uv.x*128.,(1.-uv.y)*128.);
    uint c=min(127u,uint(pos.x)),a=512u+32u*c;
    float bed=float(S[a])/Q,flow=float(S[a+1u])/Q;
    vec3 rgb=vec3(.018,.027,.05);
    if(pos.y<=bed) rgb=vec3(.47,.28,.11);
    else if(pos.y<=bed+flow) rgb=vec3(.83,.56,.22);
    for(uint i=0u;i<192u;++i){
        uint packet=70144u+64u*i;
        if(S[packet]==0u) continue;
        vec2 p=vec2(S[packet+1u],S[packet+2u])/vec2(Q,2.*Q);
        if(length(pos-p)<.36) rgb=vec3(.96,.75,.35);
    }
    uint face=min(128u,uint(round(pos.x)));
    if(W[0]==0x35314753u && W[1]==S[16] && W[2]==S[17]){
        int q=int(W[4u+face]);
        float x=(pos.x-float(face))*sign(float(q)),height=pos.y-bed-flow-.7;
        if(q!=0 && abs(x)<.48 && abs(height)<.32 && abs(height)<.48-x)
            rgb=q>0 ? vec3(.30,.9,.9) : vec3(.93,.52,.8);
        float slope=float(int(W[3]))/11520.;
        if(pos.y>118. && abs(pos.y-121.+slope*(pos.x-64.)*.12)<.24 && abs(pos.x-64.)<20.)
            rgb=vec3(.49,.71,.93);
    }
    color=vec4(rgb,1.);
}
