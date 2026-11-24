#version 450
layout(location=0) in vec2 coordinate;
layout(location=1) flat in uvec2 originMaterial;
layout(location=2) flat in float spin;
layout(push_constant) uniform View { vec2 extent; vec2 rotation; uint pass; uint encode; } view;
layout(location=0) out vec4 rgba;

void main() {
    vec3 rgb;
    if(view.pass==0u) {
        vec2 cs=view.rotation;
        vec2 p=vec2(cs.x*coordinate.x-cs.y*coordinate.y,cs.y*coordinate.x+cs.x*coordinate.y);
        vec2 edge=abs(p)-vec2(.05,.10);
        float rim=max(edge.x,edge.y);
        rgb=rim>0.?vec3(.020,.028,.041):vec3(.049,.065,.085);
        if(abs(rim)<.0008)rgb=vec3(.48,.65,.78);
        if(abs(p.y+.10)<.001 && abs(p.x)<.05)rgb=vec3(.76,.39,.13);
    } else {
        float radius=length(coordinate);
        if(radius>1. || originMaterial.y!=1u)discard;
        float hue=float((originMaterial.x*13u)%11u)/10.;
        rgb=mix(vec3(.54,.27,.071),vec3(.92,.69,.32),hue);
        rgb*=mix(1.,.54,smoothstep(.82,1.,radius));
        vec2 axis=vec2(cos(spin),sin(spin));
        if(dot(axis,coordinate)>.18 && abs(dot(vec2(-axis.y,axis.x),coordinate))<.09)rgb*=.57;
    }
    if(view.encode!=0u)rgb=pow(rgb,vec3(1./2.2));
    rgba=vec4(rgb,1.);
}
