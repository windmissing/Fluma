// T55 actual desktop ANGLE GLES3 offscreen driver, never device/presentation evidence.
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include "material_pass.h"
#include "material_uploader.h"
#include "p055_shaders.h"
using namespace p013;
#define CHECK(x) do{if(!(x)){std::printf("FAIL line %d %s egl=%x gl=%x\n",__LINE__,#x,eglGetError(),glGetError());return 1;}}while(0)
static std::vector<uint8_t> Read(const std::string& path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
template<class T> auto BindFixtureTarget(T& uploader,uint64_t revision,int) -> decltype(uploader.BindConfiguration(1,revision,std::string(64,'a')),bool()) {return uploader.BindConfiguration(1,revision,std::string(64,'a')).code==0;}
template<class T> bool BindFixtureTarget(T&,uint64_t,long){return true;} // pre-T67 historical uploader
int main(int argc,char** argv){
 EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);CHECK(display!=EGL_NO_DISPLAY);CHECK(eglInitialize(display,nullptr,nullptr));
 std::printf("EGL %s\n",eglQueryString(display,EGL_VERSION));std::printf("EGL_EXTENSIONS %s\n",eglQueryString(display,EGL_EXTENSIONS));
 EGLint attributes[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_SAMPLE_BUFFERS,0,EGL_SAMPLES,0,EGL_NONE};
 EGLConfig config;EGLint count=0;CHECK(eglChooseConfig(display,attributes,&config,1,&count)&&count>0);CHECK(eglBindAPI(EGL_OPENGL_ES_API));
 EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};auto context=eglCreateContext(display,config,EGL_NO_CONTEXT,ca);CHECK(context!=EGL_NO_CONTEXT);
 EGLint sa[]={EGL_WIDTH,160,EGL_HEIGHT,288,EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_SRGB_KHR,EGL_NONE};auto surface=eglCreatePbufferSurface(display,config,sa);CHECK(surface!=EGL_NO_SURFACE);CHECK(eglMakeCurrent(display,surface,surface,context));
 std::printf("GL_RENDERER %s\nGL_VERSION %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));
 GLint encoding=0;glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,GL_BACK,GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,&encoding);CHECK(glGetError()==GL_NO_ERROR&&encoding==GL_SRGB);
 const char* glExtensions=reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
 std::printf("GL_EXTENSIONS %s\n",glExtensions);
 if(glExtensions&&std::strstr(glExtensions,"GL_EXT_sRGB_write_control"))glEnable(0x8DB9);
 CHECK(glGetError()==GL_NO_ERROR);
 // Explicit sRGB8 test FBO isolates shader/texture math from desktop pbuffer color behavior.
 GLuint colorTexture=0,testFramebuffer=0;
 if(argc>2 && std::string(argv[2])=="srgb-fbo"){
  glGenTextures(1,&colorTexture);glBindTexture(GL_TEXTURE_2D,colorTexture);glTexStorage2D(GL_TEXTURE_2D,1,GL_SRGB8_ALPHA8,160,288);
  glGenFramebuffers(1,&testFramebuffer);glBindFramebuffer(GL_FRAMEBUFFER,testFramebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,colorTexture,0);
  CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);std::puts("READBACK_TARGET SRGB8_ALPHA8_TEST_FBO");
 } else std::puts("READBACK_TARGET EGL_SRGB_DEFAULT_PBUFFER");
 MaterialPass pass;auto created=pass.Create({kMaterialVert,kMaterialFrag},1);if(created!=Status::OK)std::printf("CREATE_STAGE %s\n",pass.errorStage);CHECK(created==Status::OK);
 if(argc<2){std::puts("PASS T55 GLES3 real shader compile/link and all required uniforms; sRGB pbuffer");pass.Reset(true);return 0;}
 std::string root=argv[1];
 auto grain=Read(root+"/grain.rgba"),strata=Read(root+"/strata.rgba");CHECK(grain.size()==kMaterialGrainBytes&&strata.size()==kMaterialStrataBytes);
 MaterialGlGpu backend;MaterialUploader upload(backend);MaterialDesc desc;desc.generation=1;desc.revision=1;desc.contextEpoch=1;desc.surfaceEpoch=1;desc.bundleId="T48_REAL_GENERATOR_NODE_EXPORT";desc.metadata="T55_offscreen";desc.grainHash=MaterialSha256(grain.data(),grain.size());desc.strataHash=MaterialSha256(strata.data(),strata.size());
 CHECK(BindFixtureTarget(upload,1,0));
 CHECK(upload.Stage(desc,grain.data(),grain.size(),strata.data(),strata.size()).code==0);MaterialReceipt receipt;
 do{receipt=upload.Pump(65536,1,1);CHECK(receipt.code==0&&receipt.uploadedBytes<=65536);}while(receipt.state!="READY");CHECK(upload.Activate(1,1,"gpu-fixture").code==0);
 std::ifstream configFile(root+"/fixtures.txt");int fixtures=0;configFile>>fixtures;CHECK(fixtures>0);
 LightParams l{.85,.15,.8,1.12,1,24,-.55,-.55,.63,6};
 MaterialParams p;p.paletteBaseSrgb={145/255.,168/255.,111/255.,216/255.,196/255.,150/255.};p.grainBaseSrgb={145/255.,168/255.,111/255.,216/255.,196/255.,150/255.,181/255.,182/255.,131/255.};
 p.canvasBox={11,7,128,256};p.canvasToSurfaceScaleX=p.canvasToSurfaceScaleY=1;p.warpRangeCycles=.25;p.effectiveLambdaCanvasPx=p.effectiveLambdaMaterial=256./11.;p.grainOffsetCanvasPx={-4.5,5.2,9.8};
 MaterialFrame m;m.assetGeneration=m.contextEpoch=1;auto& f=m.frame;f.sessionId=f.frameId=f.snapshotId=f.surfaceEpoch=f.fieldEpoch=f.geometryEpoch=f.continuityEpoch=f.appearanceConfigRevision=1;
 f.surfaceWidthPx=160;f.surfaceHeightPx=288;f.boxXPx=f.boxYPx=16;f.boxWidthPx=128;f.boxHeightPx=f.capacity=256;f.T_dataLayoutVersion=2;f.zOriginVersion=1;f.diagnosticEnabled=false;
 std::vector<uint8_t> pixels(160*288*4);
 for(int fixture=0;fixture<fixtures;fixture++){
  std::string name;int on,b,c,spark,invert,stress,alt;configFile>>name>>on>>b>>c>>spark>>invert>>stress>>alt;
  static int assetKind=0;if(stress!=assetKind){auto sg=Read(root+"/stress-grain.rgba"),sc=Read(root+"/stress-strata.rgba");desc.generation=2;desc.bundleId="SYNTHETIC_Q_BOUNDARY_ONLY";desc.grainHash=MaterialSha256(sg.data(),sg.size());desc.strataHash=MaterialSha256(sc.data(),sc.size());CHECK(upload.Stage(desc,sg.data(),sg.size(),sc.data(),sc.size()).code==0);do{receipt=upload.Pump(65536,1,1);CHECK(receipt.code==0);}while(receipt.state!="READY");CHECK(upload.Activate(2,1,"stress").code==0);assetKind=stress;m.assetGeneration=2;}
  l.lightX=alt?.8:-.55;l.lightY=alt?.2:-.55;l.lightZ=alt?1:.63;for(auto& x:p.bandMinYCanvasPx)configFile>>x;for(auto& x:p.bandMaxYCanvasPx)configFile>>x;
  std::array<float,1536> data{};std::ifstream input(root+"/"+name+".bin",std::ios::binary);input.read(reinterpret_cast<char*>(data.data()),sizeof(data));CHECK(input.gcount()==sizeof(data));
  m.lightEnabled=on;p.grainEnabled=b;p.strataEnabled=c;p.sparkEnabled=spark;f.visualInverted=invert;f.frameId=fixture+1;
  CHECK(ValidateMaterialFrame(m,l,p,data)==0);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);CHECK(pass.Draw(m,l,p,data.data(),*upload.BindActive(1))==Status::OK);
  glReadPixels(0,0,160,288,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());CHECK(glGetError()==GL_NO_ERROR);
  std::ofstream out(root+"/"+name+".rgba",std::ios::binary);out.write(reinterpret_cast<char*>(pixels.data()),pixels.size());CHECK(out.good());
  std::printf("DRAW %s frame=%d generation=%llu token=%s hashSource=embedded preSwapReadback=true\n",name.c_str(),fixture+1,static_cast<unsigned long long>(m.assetGeneration),name.c_str());
 }
 // T68 layer handoff: an empty sand layer must preserve an existing complete background.
 std::array<float,1536> empty{};for(int c=0;c<128;c++){empty[c*4+1]=256;empty[(128+c)*4+1]=.5;empty[(256+c)*4+1]=1;}
 glClearColor(.25f,.125f,.5f,1);glClear(GL_COLOR_BUFFER_BIT);uint8_t before[4]{},after[4]{};
 glReadPixels(40,40,1,1,GL_RGBA,GL_UNSIGNED_BYTE,before);
 CHECK(pass.Draw(m,l,p,empty.data(),*upload.BindActive(1))==Status::OK);
 glReadPixels(40,40,1,1,GL_RGBA,GL_UNSIGNED_BYTE,after);CHECK(std::memcmp(before,after,4)==0);
 std::puts("PASS T68 candidate sand layer preserves prior background, no full-target clear");
 CHECK(pass.textureAllocCount==1&&pass.uploadCount==uint64_t(fixtures+1));upload.Reset(true);pass.Reset(true);
 CHECK(eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));eglDestroySurface(display,surface);eglDestroyContext(display,context);eglTerminate(display);
 std::printf("PASS T55 actual MaterialPass+MaterialGlGpu %d fixtures; static pre-swap readback, NOT timing/device/presentation\n",fixtures);return 0;
}
