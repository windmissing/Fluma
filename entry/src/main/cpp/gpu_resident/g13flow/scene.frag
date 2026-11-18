#version 310 es
precision highp float;
precision highp int;
uniform uint uDraw;
uniform sampler2D uFrame;
uniform vec4 uRange;
in vec2 uv;
in vec3 tint;
layout(location=0) out vec4 color;
uint digitMask(int n){const uint digits[10]=uint[10](31599u,11415u,29671u,29647u,23497u,31183u,31215u,29257u,31727u,31695u);return digits[clamp(n,0,9)];}
bool numberGlyph(vec2 at,float value){int cell=int(floor(at.x));vec2 local=vec2(fract(at.x),at.y);if(cell<0||cell>=8||local.x>=.75||local.y<0.||local.y>=1.)return false;ivec2 pixel=ivec2(floor(vec2(local.x/.75,1.-local.y)*vec2(3.,5.)));if(cell==0)return value<0.&&pixel.y==2;float a=abs(value);if(a>9999.)return pixel.x==1||pixel.y==2;int digit=0;if(cell>=1&&cell<=4){int divisor=cell==1?1000:cell==2?100:cell==3?10:1;digit=(int(a)/divisor)%10;if(cell<4&&int(a)<divisor)return false;}else if(cell==5)return pixel.y==4&&pixel.x==1;else {int divisor=cell==6?10:100;digit=int(fract(a)*float(divisor))%10;}return (digitMask(digit)&(1u<<uint(pixel.y*3+pixel.x)))!=0u;}
void main(){if(uDraw==0u){vec3 background=vec3(.015,.02,.035);if(uv.y>.015&&uv.y<.035&&uv.x>.28&&uv.x<.72){float fraction=(uv.x-.28)/.44;background=mix(vec3(.02,.12,.34),vec3(.98,.48,.12),fraction);if(fract(fraction*4.)<.01)background=vec3(.94);}if(numberGlyph(vec2((uv.x-.04)/.018,(uv.y-.01)/.025),uRange.x)||numberGlyph(vec2((uv.x-.78)/.018,(uv.y-.01)/.025),uRange.y))background=vec3(.94);color=vec4(background,1.);return;}color=uDraw==5u?texture(uFrame,uv):(uDraw==0u?vec4(.015,.02,.035,1.):vec4(tint,1.));}
