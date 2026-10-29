#version 300 es
precision highp float;precision highp int;
uniform sampler2D uBed,uFrame;uniform vec2 uResolution;uniform vec4 uBox;uniform bool uMirror;uniform int uPass;out vec4 color;
vec3 linearize(vec3 c){return mix(c/12.92,pow((c+.055)/1.055,vec3(2.4)),step(vec3(.04045),c));}
void main(){
 color=vec4(0.);vec2 screen=vec2(gl_FragCoord.x,uResolution.y-gl_FragCoord.y);
 if(uPass==0){color=vec4(.006,.008,.014,1.);return;}
 if(uPass==3){color=texelFetch(uFrame,ivec2(gl_FragCoord.xy),0);return;}
 if(uPass==2){vec2 d=abs(screen-uBox.xy-uBox.zw*.5)-uBox.zw*.5;if(abs(max(d.x,d.y))<=1.)color=vec4(vec3(.2),.2);return;}
 vec2 q=screen-uBox.xy;if(any(lessThan(q,vec2(0.)))||any(greaterThanEqual(q,uBox.zw)))return;if(uMirror)q=uBox.zw-q;
 int column=int(clamp(floor(q.x/uBox.z*128.),0.,127.));float s=texelFetch(uBed,ivec2(column,0),0).r,height=(1.-q.y/uBox.w)*256.;if(height<s)color=vec4(linearize(vec3(.72,.57,.35)),1.);
}
