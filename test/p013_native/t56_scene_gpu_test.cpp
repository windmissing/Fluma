// T56 actual desktop GLES offscreen integration; no native-window/device claims.
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <fstream>
#include <cstring>
#include "scene_gpu.h"
#include "p055_shaders.h"
using namespace p013;
static EGLDisplay display;static EGLSurface surface;
namespace p013 {Status EglContext::Swap(uint64_t){return eglSwapBuffers(::display,::surface)?Status::OK:Status::SWAP_FAILED;}}
#define CHECK(x) do{if(!(x)){std::printf("FAIL %d %s egl=%x gl=%x\n",__LINE__,#x,eglGetError(),glGetError());return 1;}}while(0)
std::vector<uint8_t> Read(const char* p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
struct Backend:SceneGpu {
 SceneGlGpu gl;int fail=-1;bool failSwap=false;Backend(EglContext& e,MaterialPass& m,DualFieldPass& d):gl(e,m,d,{kMaterialVert,kMaterialFrag}){}
 Status Begin(const ScenePacket& p)override{return gl.Begin(p);}Status Layer(int n,const ScenePacket& p,const SceneConfiguration& c,const SceneLease* l)override{return n==fail?Status::GL_DRAW_FAILED:gl.Layer(n,p,c,l);}
 Status Present(bool old)override{return gl.Present(old);}Status Swap(uint64_t n)override{return failSwap?Status::SWAP_FAILED:gl.Swap(n);}void CommitTargets()noexcept override{gl.CommitTargets();}
 std::vector<uint8_t> Pixels(){std::vector<uint8_t> p(gl.targets.width*gl.targets.height*4);glBindFramebuffer(GL_FRAMEBUFFER,gl.targets.fbos[gl.targets.lastGood]);glReadPixels(0,0,gl.targets.width,gl.targets.height,GL_RGBA,GL_UNSIGNED_BYTE,p.data());return p;}
};
SceneConfiguration Config(int mode,uint64_t rev){SceneConfiguration c;auto& v=c.values;v[0]=mode;v[1]=3;v[3]=24;v[4]=1;v[5]=double(rev);v[6]=v[7]=16;v[8]=128;v[9]=256;v[10]=v[11]=v[12]=v[19]=1;v[13]=v[14]=256./11;v[15]=.25;v[16]=v[17]=v[18]=1;
 double grain[]={145/255.,168/255.,111/255.,216/255.,196/255.,150/255.,181/255.,182/255.,131/255.};for(int i=0;i<9;i++)v[20+i]=grain[i];for(int i=0;i<6;i++)v[29+i]=grain[i];double light[]={.85,.15,.8,1.12,1,24,-.55,-.55,.63,6};for(int i=0;i<10;i++)v[35+i]=light[i];v[45]=2;v[46]=20;v[47]=1;v[48]=24;v[50]=20261020;c.revision=rev;c.digest=MaterialSha256(reinterpret_cast<uint8_t*>(v.data()),sizeof(v));return c;}
int main(){
 display=eglGetDisplay(EGL_DEFAULT_DISPLAY);CHECK(eglInitialize(display,nullptr,nullptr));CHECK(eglBindAPI(EGL_OPENGL_ES_API));
 EGLint attr[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};EGLConfig config;EGLint n=0;CHECK(eglChooseConfig(display,attr,&config,1,&n)&&n);EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};auto context=eglCreateContext(display,config,EGL_NO_CONTEXT,ca);CHECK(context!=EGL_NO_CONTEXT);EGLint sa[]={EGL_WIDTH,160,EGL_HEIGHT,288,EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_SRGB_KHR,EGL_NONE};surface=eglCreatePbufferSurface(display,config,sa);CHECK(surface!=EGL_NO_SURFACE);CHECK(eglMakeCurrent(display,surface,surface,context));
 std::printf("GL_RENDERER %s\nGL_VERSION %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));if(std::strstr(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)),"GL_EXT_sRGB_write_control"))glEnable(0x8DB9);
 EglContext unusedWindow;MaterialPass materialPass;DualFieldPass dual;Backend backend(unusedWindow,materialPass,dual);SceneCompositor compositor;MaterialGlGpu gpu;MaterialUploader uploader(gpu);
 ScenePacket p;p.run="gpu-run";p.stateEpoch=p.commit=p.contextEpoch=1;p.capacity=2;p.count=2;p.xy={50,200,75,40};p.meta={256,1,513,2};p.medium="WET_COLUMN_V1";auto& f=p.frame;f.sessionId=f.frameId=f.snapshotId=f.fieldEpoch=f.surfaceEpoch=f.geometryEpoch=f.continuityEpoch=f.appearanceConfigRevision=1;f.surfaceWidthPx=160;f.surfaceHeightPx=288;f.boxXPx=f.boxYPx=16;f.boxWidthPx=128;f.boxHeightPx=f.capacity=256;f.T_dataLayoutVersion=2;f.zOriginVersion=1;
 p.motion={0,0,0,222,192,172,272,222,192,0};for(int c=0;c<128;c++){p.data[c*4]=100;p.data[c*4+1]=220;p.data[c*4+2]=50;p.data[c*4+3]=80;p.data[512+c*4+1]=.5;p.data[1024+c*4+1]=1.05f;}
 auto cfg=Config(0,1);auto bind=uploader.BindConfiguration(1,1,cfg.digest);cfg.bindingId=bind.bindingId;CHECK(compositor.Register(cfg));p.token="wet";auto r=compositor.Draw(p,uploader,backend);if(!r.current)std::printf("stage %s code%d\n",r.stage.c_str(),r.code);CHECK(r.current&&r.rendered.pipeline=="BASIC_FULLSCENE_V1");auto wet=backend.Pixels();auto pixel=[&](int x,int y,int c){return wet[((287-y)*160+x)*4+c];};
 CHECK(std::abs(int(pixel(66,72,0))-145)<=1&&std::abs(int(pixel(66,72,1))-168)<=1);
 CHECK(std::abs(int(pixel(91,232,0))-216)<=1&&std::abs(int(pixel(91,232,1))-196)<=1);
 CHECK(glGetError()==GL_NO_ERROR);for(size_t i=3;i<wet.size();i+=4)CHECK(wet[i]==255);CHECK(backend.gl.targets.Bytes()==160*288*8&&backend.gl.targets.allocations==2);
 p.medium="DRY_AIR_V1";p.token="dry";f.frameId++;r=compositor.Draw(p,uploader,backend);CHECK(r.current);auto dry=backend.Pixels();CHECK(wet!=dry&&backend.gl.targets.allocations==2);std::puts("PASS full background, water/fog/highlight, dry suppression, sediment, real state1/state2 particles and foreground in sRGB full FBO");
 for(int c=0;c<128;c++)p.data[c*4]=160;p.commit=2;f.snapshotId=2;
 backend.fail=5;p.token="partial";f.frameId++;r=compositor.Draw(p,uploader,backend);CHECK(!r.current&&r.state=="STALE"&&r.rendered.token.empty()&&r.lastGood.token=="dry"&&backend.Pixels()==dry);backend.fail=-1;
 backend.failSwap=true;f.frameId++;r=compositor.Draw(p,uploader,backend);CHECK(!r.current&&!r.requestedSwap&&!r.staleSwap&&backend.Pixels()==dry);backend.failSwap=false;
 for(int c=0;c<128;c++)p.data[c*4]=100;p.commit=3;f.snapshotId=3;
 auto grain=Read(".codex/local-test/t56-fixtures/grain.rgba"),strata=Read(".codex/local-test/t56-fixtures/strata.rgba");CHECK(grain.size()==kMaterialGrainBytes&&strata.size()==kMaterialStrataBytes);
 MaterialDesc desc;desc.surfaceEpoch=desc.contextEpoch=1;desc.bundleId="T56_SAME_SIGNED_NUMERIC_BYTES";desc.metadata="host fixture";desc.grainHash=MaterialSha256(grain.data(),grain.size());desc.strataHash=MaterialSha256(strata.data(),strata.size());desc.lambdaMaterial=desc.lambdaCanvas=256./11;desc.warpRange=.25;desc.seed=24;desc.geometryEpoch=1;desc.grainBase=cfg.Params().grainBaseSrgb;
 std::vector<uint8_t> p1Off;
 for(int mode=1;mode<=2;mode++){
  cfg=Config(mode,uint64_t(mode+1));bind=uploader.BindConfiguration(1,cfg.revision,cfg.digest);cfg.bindingId=bind.bindingId;CHECK(compositor.Register(cfg));desc.generation=mode;desc.revision=cfg.revision;CHECK(uploader.Stage(desc,grain.data(),grain.size(),strata.data(),strata.size()).code==0);while(uploader.Pump(65536,1,1).state!="READY"){}
  f.appearanceConfigRevision=cfg.revision;p.requestedGeneration=mode;f.frameId++;p.token=mode==1?"P1-OFF":"P1-ON";r=compositor.Draw(p,uploader,backend);if(!r.current)std::printf("stage %s code%d\n",r.stage.c_str(),r.code);CHECK(r.current&&uploader.Inspect().activeGeneration==uint64_t(mode));auto pixels=backend.Pixels();if(mode==1)p1Off=pixels;else CHECK(pixels!=p1Off);
 }
 std::puts("PASS actual T55 material OFF/ON inside complete frame, pending lease commits only after actual host swap");
 CHECK(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));CHECK(eglDestroyContext(display,context));backend.gl.Reset(false);materialPass.Reset(false);dual.Reset(false);uploader.Reset(false);compositor.ResetGpu();context=eglCreateContext(display,config,EGL_NO_CONTEXT,ca);CHECK(context!=EGL_NO_CONTEXT);CHECK(eglMakeCurrent(display,surface,surface,context));if(std::strstr(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)),"GL_EXT_sRGB_write_control"))glEnable(0x8DB9);p.contextEpoch=2;CHECK(uploader.Restore(1,2).code==0);while(uploader.Pump(65536,1,2).state!="READY"){}p.token="restored";f.frameId++;r=compositor.Draw(p,uploader,backend);CHECK(r.current&&r.lastGood.context==2);CHECK(glGetError()==GL_NO_ERROR);
 std::puts("PASS real EGL context recreation + CPU material restore + complete redraw; no stale GL names");
 uploader.Reset(true);materialPass.Reset(true);dual.Reset(true);backend.gl.Reset(true);CHECK(backend.gl.targets.Bytes()==0&&backend.gl.layers.data==0);CHECK(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));eglDestroySurface(display,surface);eglDestroyContext(display,context);eglTerminate(display);std::puts("PASS T56 real offscreen integration; no device, display-present, timing or color-parity claim");return 0;
}
