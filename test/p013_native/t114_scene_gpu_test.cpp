#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <fstream>
#include "scene_gpu.h"
#include "p055_shaders.h"
using namespace p013;
static EGLDisplay display;static EGLSurface surface;
namespace p013 {Status EglContext::Swap(uint64_t){return eglSwapBuffers(::display,::surface)?Status::OK:Status::SWAP_FAILED;}}
#define CHECK(x) do{if(!(x)){std::printf("FAIL %d %s egl=%x gl=%x\n",__LINE__,#x,eglGetError(),glGetError());return 1;}}while(0)
static std::vector<uint8_t> Read(const char* path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
struct Backend final:SceneGpu {
 EglContext e;MaterialPass m;DualFieldPass d;SceneGlGpu gl{e,m,d,{kMaterialVert,kMaterialFrag}};
 void SetBaseStripTexture(uint32_t t)override{gl.SetBaseStripTexture(t);}
 Status Begin(const ScenePacket& p)override{return gl.Begin(p);}
 Status Layer(int k,const ScenePacket& p,const SceneConfiguration& c,const SceneLease* l)override{return gl.Layer(k,p,c,l);}
 Status Present(bool old)override{return gl.Present(old);}
 Status Swap(uint64_t id)override{return gl.Swap(id);}
 void CommitTargets()noexcept override{gl.CommitTargets();}
 std::vector<uint8_t> Pixels(){std::vector<uint8_t> out(160*288*4);
  glBindFramebuffer(GL_FRAMEBUFFER,gl.targets.fbos[gl.targets.lastGood]);glReadPixels(0,0,160,288,GL_RGBA,GL_UNSIGNED_BYTE,out.data());return out;}
 void Reset(){gl.Reset(true);m.Reset(true);d.Reset(true);}
};
int main(){
 display=eglGetDisplay(EGL_DEFAULT_DISPLAY);CHECK(eglInitialize(display,nullptr,nullptr));CHECK(eglBindAPI(EGL_OPENGL_ES_API));
 EGLint a[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
 EGLConfig cfg;EGLint n=0;CHECK(eglChooseConfig(display,a,&cfg,1,&n)&&n);
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};auto ctx=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);CHECK(ctx!=EGL_NO_CONTEXT);
 EGLint sa[]={EGL_WIDTH,160,EGL_HEIGHT,288,EGL_NONE};surface=eglCreatePbufferSurface(display,cfg,sa);CHECK(surface!=EGL_NO_SURFACE);
 CHECK(eglMakeCurrent(display,surface,surface,ctx));
 Backend backend;MaterialGlGpu ag;MaterialUploader assets(ag);BaseStripGlGpu bg;BaseStripUploader strips(bg);SceneCompositor scene;
 SceneConfiguration c;auto& v=c.values;v[0]=1;v[1]=3;v[3]=24;v[4]=v[5]=1;v[6]=v[7]=16;v[8]=128;v[9]=256;
 v[10]=v[11]=v[12]=v[19]=1;v[13]=v[14]=256./11;v[15]=.25;v[16]=v[17]=v[18]=1;v[50]=20261026;
 const std::array<double,6> palette{145,168,111,216,196,150};for(int i=0;i<6;i++){v[29+i]=palette[i]/255.;v[51+i]=palette[i];}
 double grainBase[]={145/255.,168/255.,111/255.,216/255.,196/255.,150/255.,181/255.,182/255.,131/255.};for(int i=0;i<9;i++)v[20+i]=grainBase[i];
 double light[]={.85,.15,.8,1.12,1,24,-.55,-.55,.63,6};for(int i=0;i<10;i++)v[35+i]=light[i];
 c.revision=1;c.digest=MaterialSha256(reinterpret_cast<const uint8_t*>(v.data()),sizeof(v));auto bound=assets.BindConfiguration(1,1,c.digest);c.bindingId=bound.bindingId;CHECK(scene.Register(c));
 auto grain=Read(".codex/local-test/t56-fixtures/grain.rgba"),strata=Read(".codex/local-test/t56-fixtures/strata.rgba");CHECK(grain.size()==kMaterialGrainBytes&&strata.size()==kMaterialStrataBytes);
 MaterialDesc md;md.surfaceEpoch=md.contextEpoch=md.revision=md.generation=1;md.bundleId="T114";md.metadata="GPU";
 md.grainHash=MaterialSha256(grain.data(),grain.size());md.strataHash=MaterialSha256(strata.data(),strata.size());
 md.lambdaMaterial=md.lambdaCanvas=256./11;md.warpRange=.25;md.seed=24;md.geometryEpoch=1;md.grainBase=c.Params().grainBaseSrgb;
 CHECK(assets.Stage(md,grain.data(),grain.size(),strata.data(),strata.size()).code==0);
 while(assets.Pump(65536,1,1).state!="READY"){}
 ScenePacket p;p.baseStripV2=true;p.run="gpu";p.medium="WET_COLUMN_V1";p.stateEpoch=p.commit=p.contextEpoch=1;p.capacity=1;p.xy.resize(2);p.meta.resize(2);
 auto& f=p.frame;f.sessionId=f.frameId=f.snapshotId=f.fieldEpoch=f.surfaceEpoch=f.geometryEpoch=f.continuityEpoch=f.appearanceConfigRevision=1;
 f.surfaceWidthPx=160;f.surfaceHeightPx=288;f.boxXPx=f.boxYPx=16;f.boxWidthPx=128;f.boxHeightPx=256;f.capacity=256;f.T_dataLayoutVersion=2;f.zOriginVersion=1;
 p.requestedGeneration=1;p.motion={0,0,0,222,192,172,272,222,192,0};
 for(int j=0;j<128;j++){p.data[j*4]=100;p.data[j*4+1]=220;p.data[j*4+2]=50;p.data[j*4+3]=80;
  p.data[512+j*4+1]=.5f;p.data[1024+j*4+1]=1.05f;}
 BaseStripDesc sd;sd.session=1;sd.stateEpoch=1;sd.commit=1;sd.bindingId=c.bindingId;sd.geometry=1;sd.context=1;sd.surface=1;
 sd.width=160;sd.height=288;sd.boxX=16;sd.boxWidth=128;sd.spark=true;sd.run="gpu";sd.digest=c.digest;
 sd.profileHash=MaterialSha256(reinterpret_cast<const uint8_t*>(p.data.data()),1536*4);
 sd.paletteHash=MaterialSha256(reinterpret_cast<const uint8_t*>(palette.data()),sizeof(palette));
 std::vector<uint8_t> bytes(160*12);
 auto stage=[&](int serial,uint8_t value){sd.serial=serial;sd.token=serial==1?"one":"two";p.token=sd.token;f.frameId=serial;
  for(int k=0;k<3;k++)for(int j=0;j<160;j++){size_t i=(k*160+j)*4;bool valid=j>=16&&j<144;
   bytes[i]=bytes[i+1]=bytes[i+2]=valid?value:0;bytes[i+3]=valid?1:0;}
  sd.payloadHash=MaterialSha256(bytes.data(),bytes.size());p.baseStrip=sd;
  CHECK(strips.Stage(sd,bytes.data(),bytes.size()).code==0);CHECK(strips.Pump(65536,1,1).pendingBytes==0);return 0;};
 CHECK(stage(1,0)==0);auto r=scene.Draw(p,assets,backend,&strips);if(!r.current)std::printf("V2 stage=%s code=%d\n",r.stage.c_str(),r.code);CHECK(r.current&&r.actualStripSerial==1);auto dark=backend.Pixels();
 CHECK(stage(2,248)==0);r=scene.Draw(p,assets,backend,&strips);if(!r.current)std::printf("V2 stage=%s code=%d\n",r.stage.c_str(),r.code);CHECK(r.current&&r.actualStripSerial==2);auto bright=backend.Pixels();
 CHECK(dark!=bright&&glGetError()==GL_NO_ERROR);std::puts("T114 V2 complete shader scene changes with RGBA8UI strip PASS");
 strips.ClearSession(true);assets.ClearSession();backend.Reset();CHECK(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));
 eglDestroySurface(display,surface);eglDestroyContext(display,ctx);eglTerminate(display);return 0;
}
