#version 310 es
precision highp float;precision highp int;
layout(location=0)in float aX;layout(location=1)in float aY;layout(location=2)in uint aMass;
uniform vec2 uResolution;uniform vec4 uBox;uniform bool uMirror;uniform float uRadius,uMassReference;out vec2 local;flat out uint mass;
void main(){vec2 corners[6]=vec2[6](vec2(-1.,-1.),vec2(1.,-1.),vec2(-1.,1.),vec2(-1.,1.),vec2(1.,-1.),vec2(1.,1.));local=corners[gl_VertexID];mass=aMass;float radius=uRadius*sqrt(float(aMass)/1048576./uMassReference);vec2 uv=vec2(aX/128.,1.-aY/256.);if(uMirror)uv=1.-uv;vec2 screen=uBox.xy+uv*uBox.zw+local*radius;gl_Position=mass==0u?vec4(2.,2.,0.,1.):vec4(screen.x/uResolution.x*2.-1.,1.-screen.y/uResolution.y*2.,0.,1.);}
