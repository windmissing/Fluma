#version 450
layout(location=0) in vec4 disk;
layout(location=1) in uvec4 identity;
layout(push_constant) uniform View { vec2 extent; vec2 rotation; uint pass; uint encode; } view;
layout(location=0) out vec2 coordinate;
layout(location=1) flat out uvec2 originMaterial;
layout(location=2) flat out float spin;

void main() {
    const vec2 corners[6]=vec2[6](vec2(-1,-1),vec2(1,-1),vec2(-1,1),
                                vec2(-1,1),vec2(1,-1),vec2(1,1));
    vec2 q=corners[gl_VertexIndex];
    if(view.pass==0u) {
        coordinate=vec2(q.x,-q.y)*view.extent;
        originMaterial=uvec2(0);
        spin=0.;
        gl_Position=vec4(q,0.,1.);
    } else {
        coordinate=q;
        originMaterial=identity.zw;
        spin=disk.w;
        vec2 p=(disk.xy+q*disk.z)/view.extent;
        gl_Position=vec4(p.x,-p.y,0.,1.);
    }
}
