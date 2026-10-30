#version 310 es
precision highp float;precision highp int;
layout(std430,binding=0) readonly buffer Publication {uint P[];};
uniform uint uColumn;uniform sampler2D uFrame;uniform vec2 uResolution;uniform vec4 uBox;uniform bool uMirror;uniform int uLayer;
uniform vec4 uOptics[4];out vec4 color;
vec4 row(int c,int r){return vec4(uintBitsToFloat(P[uColumn+8u*128u+uint(c)]),uintBitsToFloat(P[uColumn+10u*128u+uint(c)]),uintBitsToFloat(P[uColumn+9u*128u+uint(c)]),0.);}
float footprint(int c,vec2 pixel){
 vec4 a=row(c,0);if(a.y<=0.)return 0.;float cw=uBox.z/128.;float lo=uBox.x+float(c)*cw,hi=lo+cw;
 float ys=a.x/256.*uBox.w,ye=a.z/256.*uBox.w,low=max(ys,ye-uOptics[2].x);
 float y0=uBox.y+uBox.w-ye,y1=uBox.y+uBox.w-low;
 if(uMirror){lo=uBox.x+uBox.z-float(c+1)*cw;hi=lo+cw;y0=uBox.y+low;y1=uBox.y+ye;}
 float A=max(0.,min(pixel.x+1.,hi)-max(pixel.x,lo))*max(0.,min(pixel.y+1.,y1)-max(pixel.y,y0));
 bool left=c>0&&row(max(c-1,0),0).y>0.,right=c<127&&row(min(c+1,127),0).y>0.;float slope=0.;
 if(left&&right)slope=(row(c+1,0).z-row(c-1,0).z)/2.;else if(left)slope=a.z-row(c-1,0).z;else if(right)slope=row(c+1,0).z-a.z;
 vec2 n=normalize(vec2(-slope,1.));float F=uOptics[3].x+(1.-uOptics[3].x)*pow(1.-clamp(n.y,0.,1.),5.);
 float G=pow(clamp(dot(n,uOptics[3].zw),0.,1.),uOptics[3].y);return uOptics[1].x*A*(F+(1.-F)*G);
}
vec3 linearize(vec3 c){return mix(c/12.92,pow((c+.055)/1.055,vec3(2.4)),step(vec3(.04045),c));}
void main(){
 vec2 screen=vec2(gl_FragCoord.x,uResolution.y-gl_FragCoord.y);color=vec4(0.);
 if(uLayer==6){color=texelFetch(uFrame,ivec2(gl_FragCoord.xy),0);return;}
 if(uLayer==0){color=vec4(.006,.008,.014,1.);return;}
 if(uLayer==5){vec2 d=abs(screen-uBox.xy-uBox.zw*.5)-uBox.zw*.5;if(abs(max(d.x,d.y))<=1.)color=vec4(vec3(.08),.2);return;}
 if(uLayer==4){
  vec2 pixel=floor(screen);float x0=max(pixel.x,uBox.x),x1=min(pixel.x+1.,uBox.x+uBox.z);if(x1<=x0)return;
  float left=(x0-uBox.x)/uBox.z*128.,right=(x1-uBox.x)/uBox.z*128.;
  int c0=int(clamp(floor(uMirror?128.-right:left),0.,127.));int c1=int(clamp(ceil(uMirror?128.-left:right)-1.,0.,127.));
  float alpha=footprint(c0,pixel);if(c1!=c0)alpha+=footprint(c1,pixel);color=vec4(alpha*uOptics[2].yzw,alpha);return;
 }
 vec2 q=screen-uBox.xy;if(any(lessThan(q,vec2(0.)))||any(greaterThanEqual(q,uBox.zw)))return;
 if(uMirror)q=uBox.zw-q;int c=int(clamp(floor(q.x/uBox.z*128.),0.,127.));vec4 a=row(c,0);float y=(1.-q.y/uBox.w)*256.;
 if(uLayer==1&&y<a.x){color=vec4(linearize(vec3(.72,.57,.35)),1.);return;}
 if(uLayer==3&&a.y>0.&&y<=a.z){float path=clamp(a.z-max(y,a.x),0.,a.y);float alpha=1.-exp(-uOptics[0].x*path);color=vec4(alpha*uOptics[0].yzw,alpha);}
}
