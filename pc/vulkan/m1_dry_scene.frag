#version 450
layout(set=0,binding=0,std430) readonly buffer Publication { uint P[]; };
layout(push_constant) uniform WindowFrame { vec2 extent; uint encode; };
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
    const uint columnBase = 16u*sandCapacity + 20u*128u;
    const uint header = columnBase + 16u*128u;
    const vec3 background = vec3(.006,.008,.014);
    vec3 rgb = background;
    if (P[header] == 1347637831u && P[header+1u] == 2u && P[header+3u] == 3235823838u) {
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
        vec2 edge = abs(uv*extent-extent*.5)-extent*.5;
        if (abs(max(edge.x,edge.y))<=1.) rgb = vec3(.08)+rgb*.8;
    }
    color = vec4(encode!=0u ? srgb(rgb) : rgb,1);
}
