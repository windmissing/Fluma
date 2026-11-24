#version 450
layout(set=0,binding=0,std430) readonly buffer Publication { uint P[]; };
layout(push_constant) uniform WindowFrame { vec2 extent; uint encode; uint empty; int gx; int gy; int gravityState; };
layout(location=0) in vec2 uv;
layout(location=0) out vec4 color;

vec3 linearize(vec3 c) {
    return mix(c/12.92, pow((c+.055)/1.055,vec3(2.4)), step(vec3(.04045),c));
}

vec3 srgb(vec3 c) {
    return mix(c*12.92,1.055*pow(c,vec3(1.0/2.4))-.055,step(vec3(.0031308),c));
}

void main() {
    const uint sandCapacity = 192u;
#ifdef M1_FULL_SCENE
    const uint waterCapacity = 128u;
    const uint waterBase = 16u*sandCapacity;
    const uint columnBase = waterBase + 20u*waterCapacity;
#else
    const uint columnBase = 16u*sandCapacity + 20u*128u;
#endif
    const uint header = columnBase + 16u*128u;
    const vec3 background = vec3(.006,.008,.014);
    vec3 rgb = background;
    if (empty!=0u) {
        vec2 edge = abs(uv*extent-extent*.5)-extent*.5;
        if (abs(max(edge.x,edge.y))<=1.) rgb = vec3(.08)+rgb*.8;
    } else if (P[header] == 1347637831u && P[header+1u] == 2u && P[header+3u] == 3235823838u) {
        const vec3 sandColor = vec3(.72,.57,.35);
        vec2 point = vec2(uv.x,1.0-uv.y)*vec2(128,256);
        uint column = min(uint(point.x),127u);
        if (point.y < uintBitsToFloat(P[columnBase+8u*128u+column])) rgb = linearize(sandColor);
        uint live = min(P[header+34u],sandCapacity);
        for (uint i=0u; i<live; ++i) {
            vec2 position = vec2(uintBitsToFloat(P[8u*sandCapacity+i]),uintBitsToFloat(P[9u*sandCapacity+i]));
            float radius = uintBitsToFloat(P[10u*sandCapacity+i]);
            vec2 local = (vec2(position.x/128.,1.-position.y/256.)-uv)*extent/radius;
            if (P[3u*sandCapacity+i]!=0u && radius>0. && dot(local,local)<=1.) {
                rgb = linearize(sandColor);
                break;
            }
        }
#ifdef M1_FULL_SCENE
        float bed = uintBitsToFloat(P[columnBase+8u*128u+column]);
        const vec3 waterColor = vec3(.04,.13,.18);
        float water = uintBitsToFloat(P[columnBase+10u*128u+column]);
        float eta = uintBitsToFloat(P[columnBase+9u*128u+column]);
        if (water > 0. && point.y <= eta) {
            float path = clamp(eta-max(point.y,bed),0.,water);
            rgb = mix(rgb,waterColor,1.-exp(-.03*path));
        }
        uint packets = min(P[header+35u],waterCapacity);
        for (uint i=0u; i<packets; ++i) {
            uint mass = P[waterBase+i];
            if (mass==0u) continue;
            vec2 position = vec2(uintBitsToFloat(P[waterBase+16u*waterCapacity+i]),
                                 uintBitsToFloat(P[waterBase+17u*waterCapacity+i]));
            float radius = 2.*sqrt(float(mass)/131072.);
            vec2 local = (uv-vec2(position.x/128.,1.-position.y/256.))*extent/radius;
            float alpha = .8*(1.-smoothstep(.8,1.,length(local)));
            float highlight = pow(max(0.,1.-length(local-vec2(-.25,-.25))),8.);
            rgb = mix(rgb,mix(waterColor,vec3(.8,.9,1.),highlight),alpha);
        }
#endif
        vec2 edge = abs(uv*extent-extent*.5)-extent*.5;
        if (abs(max(edge.x,edge.y))<=1.) rgb = vec3(.08)+rgb*.8;
    }
    if (gravityState!=0 && extent.x>=112. && extent.y>=42.) {
        vec2 pixel=uv*extent;
        vec2 button=pixel-vec2(extent.x-42.,8.);
        if (all(greaterThanEqual(button,vec2(0.))) && all(lessThan(button,vec2(34.,30.)))) {
            rgb=mix(rgb,vec3(.08,.14,.21),.92);
            vec2 p=button-vec2(11.,7.);
            bool letter=(p.x>=0. && p.x<2. && p.y>=0. && p.y<16.) ||
                (p.x>=0. && p.x<10. && (abs(p.y-1.)<1. || abs(p.y-7.)<1.)) ||
                (p.x>=8. && p.x<10. && p.y>=0. && p.y<8.) ||
                (p.y>=7. && p.y<16. && abs(p.x-(p.y-5.))<1.3);
            if (letter) rgb=vec3(.8,.86,.92);
        }
        if (gravityState>=2) {
            vec2 p=pixel-vec2(extent.x-75.,22.);
            vec2 direction=vec2(gx,gy)/1048576.;
            float along=dot(p,direction),side=abs(dot(p,vec2(direction.y,-direction.x)));
            if ((along>=-11. && along<=8. && side<1.1) ||
                (along>3. && along<=11. && abs(side-(11.-along)*.65)<1.1))
                rgb=gravityState==3 ? vec3(.95,.47,.09) : vec3(.38,.82,.96);
        }
    }
    color = vec4(encode!=0u ? srgb(rgb) : rgb,1);
}
