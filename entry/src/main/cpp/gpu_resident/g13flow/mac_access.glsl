uint nx(){return uL[0];}uint ny(){return uL[1];}uint nc(){return uL[2];}uint nf(){return uL[3];}
uint nuFaces(){return (nx()+1u)*ny();}
float h(){return uintBitsToFloat(c[10]);}float dt(){return 1.0/(120.0*float(max(uJ,1u)));}
uint work(uint i){return uL[16u+i];}
bool periodicX(){return c[6]==2u||c[6]==3u;}bool periodicY(){return c[6]==2u;}
int wrapIndex(int i,int n){return (i%n+n)%n;}
bool activeFace(uint f){if(f<nuFaces()){uint x=f%(nx()+1u);return periodicX()?x<nx():x>0u&&x<nx();}uint y=(f-nuFaces())/nx();return periodicY()?y<ny():y>0u&&y<ny();}
uint faceIndex(bool vertical,int x,int y){return vertical?nuFaces()+uint(y)*nx()+uint(x):uint(y)*(nx()+1u)+uint(x);}
float fieldRead(uint array,bool vertical,int x,int y){int X=int(nx()),Y=int(ny());if(periodicX())x=wrapIndex(x,X);if(periodicY())y=wrapIndex(y,Y);
 bool normal=vertical?(y==0||y==Y)&&!periodicY():(x==0||x==X)&&!periodicX();if(normal)return 0.;
 float sign=1.,offset=0.;if(!vertical&&!periodicX()){if(x<0){x=-x;sign=-1.;}if(x>X){x=2*X-x;sign=-1.;}}if(vertical&&!periodicY()){if(y<0){y=-y;sign=-1.;}if(y>Y){y=2*Y-y;sign=-1.;}}if(vertical&&!periodicX()){if(x<0){x=-1-x;sign=-1.;}if(x>=X){x=2*X-1-x;sign=-1.;}}
 if(!vertical&&!periodicY()){if(y<0){y=-1-y;sign=-1.;offset=2.*uintBitsToFloat(c[32]);}if(y>=Y){y=2*Y-1-y;sign=-1.;offset=2.*uintBitsToFloat(c[33]);}}
 uint index=faceIndex(vertical,x,y);uint bits=array==0xffffffffu?outState[256u+index]:(array==0xfffffffeu?baseState[256u+index]:w[array+index]);return sign*uintBitsToFloat(bits)+offset;}
float linearRead(uint array,bool vertical,vec2 pos){vec2 p=pos/h()-(vertical?vec2(.5,0.):vec2(0.,.5));ivec2 i=ivec2(floor(p));vec2 t=fract(p);return mix(mix(fieldRead(array,vertical,i.x,i.y),fieldRead(array,vertical,i.x+1,i.y),t.x),mix(fieldRead(array,vertical,i.x,i.y+1),fieldRead(array,vertical,i.x+1,i.y+1),t.x),t.y);}
vec2 velocityAt(uint array,vec2 p){return vec2(linearRead(array,false,p),linearRead(array,true,p));}
vec2 facePosition(uint f){bool v=f>=nuFaces();uint z=v?f-nuFaces():f;return h()*(v?(vec2(float(z%nx())+.5,float(z/nx()))):(vec2(float(z%(nx()+1u)),float(z/(nx()+1u))+.5)));}
uvec4 loadQ(uint array,uint cell){return s64(uvec2(w[array+2u*cell],w[array+2u*cell+1u]));}
void storeQ(uint array,uint cell,uvec4 value){if(!sfits64(value))fail(17u);w[array+2u*cell]=value.x;w[array+2u*cell+1u]=value.y;}
uint pressureCell(int x,int y,uint X,uint Y){x=periodicX()?wrapIndex(x,int(X)):clamp(x,0,int(X)-1);y=periodicY()?wrapIndex(y,int(Y)):clamp(y,0,int(Y)-1);return uint(y)*X+uint(x);}
uvec4 pressureAt(uint array,uint level,int x,int y){uint X=uL[40u+3u*level],Y=uL[41u+3u*level],start=uL[42u+3u*level];return loadQ(array,start+pressureCell(x,y,X,Y));}
void canonicalFaces(uint array){for(uint f=lane;f<nf();f+=64u){bool v=f>=nuFaces();uint z=v?f-nuFaces():f;int x=int(v?z%nx():z%(nx()+1u)),y=int(v?z/nx():z/(nx()+1u));if(!activeFace(f))w[array+f]=floatBitsToUint(fieldRead(array,v,x,y));}}
