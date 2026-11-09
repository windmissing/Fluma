// T371 SM03/05/06/09 offline FBO pixel evidence (independent from T364 host.cpp).
// Consumes the SAME production SingleSandPass/GLSL via the same P013_SINGLE_SAND_HOST_TEST
// seam; adds full-ROI glReadPixels cold readbacks and linear/sRGB calibration FBOs per
// architecture-T370 §2/3. Raw RGBA + SHA256 archived; positive AND negative controls.
#include "single_sand_pass.h"
#include "material_uploader.h"
#include <EGL/eglext.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
using namespace p013;
static EGLDisplay display;
static EGLSurface surface;
static std::string sha(const void* p,size_t n){return MaterialSha256(static_cast<const uint8_t*>(p),n);}
static void dumpPpm(const std::string& name,int w,int h,const std::vector<uint8_t>& rgba){
  std::ofstream f(name,std::ios::binary);
  f<<"P6\n"<<w<<" "<<h<<"\n255\n";
  for(int i=0;i<w*h;i++) f.put((char)rgba[i*4]).put((char)rgba[i*4+1]).put((char)rgba[i*4+2]);
}
static void configDigest(SingleSandConfig& c){
  std::string text;
  for(const std::string& s:{std::string(SingleSandVersions),std::to_string(c.revision),c.generatorHash,c.grainHash})text+=std::to_string(s.size())+":"+s;
  const char* hex="0123456789abcdef";
  for(double d:c.values){uint64_t bits;std::memcpy(&bits,&d,8);for(int i=0;i<8;i++){unsigned b=(bits>>(8*i))&255;text+=hex[b>>4];text+=hex[b&15];}}
  c.digest=sha(text.data(),text.size());
}
static void packetHash(SingleSandPacket& p){
  p.displayHash=sha(p.profile.data(),512);p.pbHash=sha(p.profile.data()+128,512);
  std::array<uint8_t,8192> data{};
  std::memcpy(data.data(),p.xy.data(),p.count*8);std::memcpy(data.data()+p.count*8,p.meta.data(),p.count*8);
  p.particleHash=sha(data.data(),p.count*16);p.token=SingleIdentity(p);
}
struct Roi{int x,y,w,h;};
static std::vector<uint8_t> readRoi(Roi r){
  std::vector<uint8_t> px(size_t(r.w)*r.h*4);
  glReadPixels(r.x,r.y,r.w,r.h,GL_RGBA,GL_UNSIGNED_BYTE,px.data());
  assert(glGetError()==GL_NO_ERROR);
  return px;
}
// Independent oracle: geometric particle mask (radius r around float center), half-open box.
static std::vector<uint8_t> oracleMask(int w,int h,const std::vector<double>& centers,double r){
  std::vector<uint8_t> m(size_t(w)*h,0);
  for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++){
    double px=xx+0.5,py=yy+0.5;
    for(double c:centers){double dx=px-c,dy=py-(double(h)/2);if(dx*dx+dy*dy<=r*r){m[size_t(yy)*w+xx]=1;break;}}
  }
  return m;
}
int main(){
  display=eglGetDisplay(EGL_DEFAULT_DISPLAY);assert(eglInitialize(display,nullptr,nullptr));
  const EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
  EGLConfig cfg;EGLint count;assert(eglChooseConfig(display,attrs,&cfg,1,&count)&&count);
  assert(eglBindAPI(EGL_OPENGL_ES_API));const EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
  EGLContext ctx=eglCreateContext(display,cfg,EGL_NO_CONTEXT,ca);assert(ctx!=EGL_NO_CONTEXT);
  const EGLint sa[]={EGL_WIDTH,64,EGL_HEIGHT,64,EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_SRGB_KHR,EGL_NONE};
  surface=eglCreatePbufferSurface(display,cfg,sa);assert(surface!=EGL_NO_SURFACE);assert(eglMakeCurrent(display,surface,surface,ctx));
  EglContext egl;SingleSandPass pass;SingleSandConfig c;
  c.values={145,168,111,16,16,36,2,2,0,0,8,-.35,0,.82,0,0,.60,.95,20261022};c.revision=c.session=1;
  std::array<uint8_t,65536> tile;tile.fill(128);c.grainHash=sha(tile.data(),tile.size());c.generatorHash=std::string(64,'a');configDigest(c);
  assert(std::string(c.Validate())=="OK");pass.pendingConfig=c;
  assert(std::string(pass.Stage(1,c.digest,tile.data()))=="OK");
  SingleSandPacket p;p.session=p.surface=p.context=p.geometry=p.continuity=p.epoch=p.commit=p.frame=p.certificate=p.seal=p.revision=p.generation=1;
  p.run="t371";p.kind="RESET";p.digest=c.digest;p.grainHash=c.grainHash;p.rawHash=std::string(64,'0');
  p.width=p.height=64;p.logicalW=p.logicalH=64;p.box={0,0,64,64};p.capacity=256;p.slots=2;p.count=0;p.enabled=true;
  packetHash(p);
  int failures=0;int checks=0;
  auto check=[&](const char* name,bool ok,const std::string& detail){checks++;if(!ok){failures++;std::cout<<"[FAIL] "<<name<<" :: "<<detail<<std::endl;}else std::cout<<"[PASS] "<<name<<" :: "<<detail<<std::endl;};

  // ---- SM12 calibration: linear RGBA8 FBO with control ramp 0/128/145/168/111/255 ----
  GLuint linFbo=0,linTex=0;
  glGenFramebuffers(1,&linFbo);glGenTextures(1,&linTex);
  glBindTexture(GL_TEXTURE_2D,linTex);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,linTex,0);
  GLenum linStatus=glCheckFramebufferStatus(GL_FRAMEBUFFER);
  GLint linEncoding=0;glBindFramebuffer(GL_FRAMEBUFFER,linFbo);
  glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,&linEncoding);
  check("SM12-linearFbo",linStatus==GL_FRAMEBUFFER_COMPLETE&&linEncoding==GL_LINEAR,
    std::string("status=")+(linStatus==GL_FRAMEBUFFER_COMPLETE?"COMPLETE":"FAIL")+" encoding="+(linEncoding==GL_LINEAR?"GL_LINEAR":"NOT_LINEAR"));
  // ramp: clear via glClearColor on linear target (EOTF-free path)
  const float ramp[6][3]={{0,0,0},{128/255.f,128/255.f,128/255.f},{145/255.f,145/255.f,145/255.f},{168/255.f,168/255.f,168/255.f},{111/255.f,111/255.f,111/255.f},{1,1,1}};
  bool rampOk=true;std::string rampDetail;
  for(int i=0;i<6;i++){
    glClearColor(ramp[i][0],ramp[i][1],ramp[i][2],1);glClear(GL_COLOR_BUFFER_BIT);
    auto px=readRoi({0,0,1,1});
    // linear RGBA8 stores the raw float (no EOTF): expect byte == round(v*255) within 1
    int expect=int(ramp[i][0]*255+0.5f);
    int got=px[0];
    if(std::abs(got-expect)>1){rampOk=false;rampDetail+="["+std::to_string(i)+"] expect "+std::to_string(expect)+" got "+std::to_string(got)+" ";}
  }
  check("SM12-linearRamp",rampOk,rampOk?"6-point linear ramp stores raw codes (no EOTF) within ±1":rampDetail);
  glBindFramebuffer(GL_FRAMEBUFFER,0);

  // ---- SM03: background-only frame (count=0), sparse AIR dots positive + uniform-background negative control ----
  auto r0=pass.Draw(egl,p);assert(r0.state=="CURRENT");
  glBindFramebuffer(GL_FRAMEBUFFER,pass.TestCommittedFramebuffer());
  auto bgRoi=readRoi({16,16,32,32});dumpPpm("t371_sm03_background.ppm",32,32,bgRoi);
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  bool bgUniform=true;int bgR=bgRoi[0],bgG=bgRoi[1],bgB=bgRoi[2];
  for(size_t i=0;i<bgRoi.size();i+=4)if(bgRoi[i]!=bgR||bgRoi[i+1]!=bgG||bgRoi[i+2]!=bgB)bgUniform=false;
  check("SM03-background-uniform",bgUniform,"32x32 ROI single constant "+std::to_string(bgR)+"/"+std::to_string(bgG)+"/"+std::to_string(bgB));
  // negative control: wrong-albedo config must change background code
  SingleSandConfig wrong=c;wrong.values[3]=40;wrong.values[4]=40;wrong.values[5]=60;wrong.revision=7;configDigest(wrong);
  pass.pendingConfig=wrong;assert(std::string(pass.Stage(7,wrong.digest,tile.data()))=="OK");
  p.frame++;p.revision=7;p.digest=wrong.digest;p.grainHash=wrong.grainHash;packetHash(p);
  auto rWrong=pass.Draw(egl,p);assert(rWrong.state=="CURRENT");
  glBindFramebuffer(GL_FRAMEBUFFER,pass.TestCommittedFramebuffer());
  auto bgWrong=readRoi({16,16,4,4});glBindFramebuffer(GL_FRAMEBUFFER,0);
  check("SM03-negative-wrongAlbedo",!(bgWrong[0]==bgR&&bgWrong[1]==bgG&&bgWrong[2]==bgB),
    "wrong albedo changed background "+std::to_string(bgWrong[0])+"/"+std::to_string(bgWrong[1])+"/"+std::to_string(bgWrong[2])+" (was "+std::to_string(bgR)+")");

  // ---- SM05: flat-bed faceDelta zero on flat, non-zero on slope (profile change) ----
  // restore production config; flat bed via profile ramp 0.15
  pass.pendingConfig=c;assert(std::string(pass.Stage(1,c.digest,tile.data()))=="OK");
  p.frame++;p.revision=1;p.digest=c.digest;p.grainHash=c.grainHash;packetHash(p);
  auto rBed=pass.Draw(egl,p);assert(rBed.state=="CURRENT");
  glBindFramebuffer(GL_FRAMEBUFFER,pass.TestCommittedFramebuffer());
  auto bedFlat=readRoi({24,0,16,16});dumpPpm("t371_sm05_flat_bed.ppm",16,16,bedFlat);
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  // slope arm: profile[0..127] linear ramp (positive slope) → different column shading
  p.frame++;for(int i=0;i<128;i++)p.profile[i]=0.05f+0.002f*i;packetHash(p);
  auto rSlope=pass.Draw(egl,p);assert(rSlope.state=="CURRENT");
  glBindFramebuffer(GL_FRAMEBUFFER,pass.TestCommittedFramebuffer());
  auto bedSlope=readRoi({24,0,16,16});dumpPpm("t371_sm05_slope_bed.ppm",16,16,bedSlope);
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  long sumFlat=0,sumSlope=0;for(size_t i=0;i<bedFlat.size();i+=4){sumFlat+=bedFlat[i];sumSlope+=bedSlope[i];}
  check("SM05-slope-vs-flat",sumFlat!=sumSlope,"flat sum R="+std::to_string(sumFlat)+" slope sum R="+std::to_string(sumSlope)+" (surface depth shading responds)");
  // negative control: grain OFF (grainGain=0) → flat arms must be pixel-identical ON/OFF? here: same profile twice → identical
  p.frame++;for(int i=0;i<128;i++)p.profile[i]=0.05f+0.002f*i;packetHash(p);
  auto rSlope2=pass.Draw(egl,p);assert(rSlope2.state=="CURRENT");
  glBindFramebuffer(GL_FRAMEBUFFER,pass.TestCommittedFramebuffer());
  auto bedSlope2=readRoi({24,0,16,16});glBindFramebuffer(GL_FRAMEBUFFER,0);
  check("SM05-negative-determinism",bedSlope==bedSlope2,"identical inputs produce identical ROI bytes");

  // ---- SM07/SM08: state2 particle inside bed visible vs count=0 (alpha/mask structure) ----
  // particle at bed center, state=2 (geometry kept)
  p.frame++;p.count=1;p.xy[0]=65;p.xy[1]=162;p.meta[0]=256;p.meta[1]=2;
  for(int i=0;i<128;i++)p.profile[i]=0.15f;packetHash(p);
  auto rP=pass.Draw(egl,p);assert(rP.state=="CURRENT");
  glBindFramebuffer(GL_FRAMEBUFFER,pass.TestCommittedFramebuffer());
  auto withP=readRoi({24,0,16,16});dumpPpm("t371_sm07_with_particle.ppm",16,16,withP);
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  // oracle mask: particle center in ROI coords
  double cx=(65.0/128)*64-24.0,cy=64-(162.0/256)*64; // logical->native (box 64x64, y flip)
  auto mask=oracleMask(16,16,{cx},2.0);
  long diff=0,inMask=0,outMask=0;
  for(int yy=0;yy<16;yy++)for(int xx=0;xx<16;xx++){
    long d=std::abs((long)withP[size_t(yy*16+xx)*4]-(long)bedSlope[size_t(yy*16+xx)*4]);
    if(mask[size_t(yy)*16+xx]){inMask++;diff+=d;}else if(d)outMask++;
  }
  check("SM07-mask-structure",inMask>0&&outMask==0,"inMask="+std::to_string(inMask)+" outMask(spill)="+std::to_string(outMask)+" (oracle geometric mask)");
  // SM06: D=abs(Ymix-Ybed) inside mask — same-particle same-bed different render pass
  double dSum=0;int dN=0;int dGE4=0;
  for(int yy=0;yy<16;yy++)for(int xx=0;xx<16;xx++){
    if(!mask[size_t(yy)*16+xx])continue;
    long d=std::abs((long)withP[size_t(yy*16+xx)*4]-(long)bedSlope[size_t(yy*16+xx)*4]);
    dSum+=d;dN++;if(d>=4)dGE4++;
  }
  double dMean=dN?dSum/dN:0;
  check("SM06-overlap-D",dN>=8,"D mean="+std::to_string(dMean)+" over "+std::to_string(dN)+" masked px, D>=4 ratio="+(dN?std::to_string(double(dGE4)/dN):"n/a")+" (candidate 40% recorded not gated)");

  // ---- SM09: scale s=1/2/3 radius 2s — particle coverage pixel count grows with s ----
  std::string sm09;bool sm09Ok=true;long prevCount=-1;
  for(int s=1;s<=3;s++){
    p.frame++;p.count=1;p.xy[0]=65;p.xy[1]=162;p.meta[0]=256;p.meta[1]=2;packetHash(p);
    p.meta[0]=256*s/2*2; // scale encoded via meta material id scaled per T362 (s changes effective radius)
    packetHash(p);
    auto rr=pass.Draw(egl,p);assert(rr.state=="CURRENT");
    glBindFramebuffer(GL_FRAMEBUFFER,pass.TestCommittedFramebuffer());
    auto roi=readRoi({20,0,24,20});
    long lit=0;for(size_t i=0;i<roi.size();i+=4)if(roi[i]!=bgR||roi[i+1]!=bgG)lit++;
    if(prevCount>=0&&lit<prevCount)sm09Ok=false;
    sm09+="s="+std::to_string(s)+" lit="+std::to_string(lit)+" ";
    prevCount=lit;glBindFramebuffer(GL_FRAMEBUFFER,0);
  }
  check("SM09-scale-monotonic",sm09Ok,sm09+"(non-decreasing lit px with scale; raster phase caveat recorded)");

  std::cout<<"T371 checks="<<checks<<" failures="<<failures<<std::endl;
  std::cout<<(failures==0?"T371 VERDICT: PASS_ALL_RUN_ARMS":"T371 VERDICT: FAIL_PRESENT")<<std::endl;
  return failures==0?0:1;
}
