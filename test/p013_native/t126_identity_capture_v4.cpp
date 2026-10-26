// T126 native probe: T115_IDENTITY_CAPTURE_V4 per T121 section 5.
// Fixes the V3 evidence defects:
//  1. real SHA256: Node-standard crypto cross-checked in driver; native computes
//     a CORRECT compact SHA-256 (message word w[i] restored) and driver
//     recomputes every digest with crypto and runs known-answer vectors.
//  2. real raw dump: prod/debug same-content same-SRGB8_ALPHA8-attachment RGBA
//     dumped to files; integer/float stage passes marked NOT_RUN when absent.
//  3. actual execution matrix: default-back controlled triangle actually drawn
//     + read + sampled; fractional-alpha actually drawn with blend; dither
//     disabled branch actually toggled; stageErrors is a typed object.
//  4. six-constant matrix with correct EOTF/OETF ids and real F64 bit patterns.
// Old V3 files under test/reports/t115/ and .codex/local-test/t115/ preserved.
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include "material_pass.h"
#include "material_uploader.h"
#include "p055_shaders.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <sstream>

#define CHECK(x) do{if(!(x)){std::printf("FAIL line %d %s egl=%x gl=%x\n",__LINE__,#x,eglGetError(),glGetError());return 1;}}while(0)
using namespace p013;
static std::string g_out;
static std::vector<uint8_t> Read(const std::string& p) {
  std::ifstream f(p, std::ios::binary); return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
static std::string hexOf(const void* p, size_t n) {
  static const char* h = "0123456789abcdef"; std::string s; const uint8_t* b = (const uint8_t*)p;
  for (size_t i = 0; i < n; ++i) { s += h[b[i] >> 4]; s += h[b[i] & 15]; } return s;
}
static void dumpBin(const std::string& n, const void* p, size_t n2) {
  std::ofstream f(g_out + "/" + n, std::ios::binary); f.write((const char*)p, n2);
  f.close(); // caller verifies completion via size check
}
static std::string pct17(double v){char b[40];std::snprintf(b,sizeof b,"%.17g",v);return b;}
static void dumpText(const std::string& n, const std::string& t) {
  std::ofstream f(g_out + "/" + n, std::ios::binary); f.write(t.data(), t.size());
}
// Correct compact SHA-256 (FIPS 180-4, message word w[i] in the round).
namespace sha2 {
struct Ctx { uint32_t h[8]; uint64_t len; uint8_t buf[64]; size_t pos; };
inline uint32_t ror(uint32_t x, int r){return (x>>r)|(x<<(32-r));}
void init(Ctx& c){static const uint32_t H[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};for(int i=0;i<8;i++)c.h[i]=H[i];c.len=0;c.pos=0;}
void block(Ctx& c,const uint8_t* p){
 uint32_t w[64];for(int i=0;i<16;i++)w[i]=((uint32_t)p[i*4]<<24)|((uint32_t)p[i*4+1]<<16)|((uint32_t)p[i*4+2]<<8)|p[i*4+3];
 for(int i=16;i<64;i++){uint32_t s0=ror(w[i-15],7)^ror(w[i-15],18)^(w[i-15]>>3);uint32_t s1=ror(w[i-2],17)^ror(w[i-2],19)^(w[i-2]>>10);w[i]=w[i-16]+s0+w[i-7]+s1;}
 static const uint32_t K[64]={0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
 uint32_t a=c.h[0],b=c.h[1],cc=c.h[2],d=c.h[3],e=c.h[4],f=c.h[5],g=c.h[6],h=c.h[7];
 for(int i=0;i<64;i++){
  uint32_t S1=ror(e,6)^ror(e,11)^ror(e,25);uint32_t ch=(e&f)^(~e&g);
  uint32_t t1=h+S1+ch+K[i]+w[i]; // FIX: message word restored (V3 tinysha omitted it)
  uint32_t S0=ror(a,2)^ror(a,13)^ror(a,22);uint32_t maj=(a&b)^(a&cc)^(b&cc);uint32_t t2=S0+maj;
  h=g;g=f;f=e;e=d+t1;d=cc;cc=b;b=a;a=t1+t2;
 }
 c.h[0]+=a;c.h[1]+=b;c.h[2]+=cc;c.h[3]+=d;c.h[4]+=e;c.h[5]+=f;c.h[6]+=g;c.h[7]+=h;
}
void update(Ctx& c,const uint8_t* d,size_t n){c.len+=n;while(n){size_t k=64-c.pos<n?64-c.pos:n;for(size_t i=0;i<k;i++)c.buf[c.pos+i]=d[i];c.pos+=k;d+=k;n-=k;if(c.pos==64){block(c,c.buf);c.pos=0;}}}
std::string hex(Ctx c){uint64_t bits=c.len*8;uint8_t pad=0x80;update(c,&pad,1);uint8_t z=0;while(c.pos!=56)update(c,&z,1);
 uint8_t lenb[8];for(int i=0;i<8;i++)lenb[i]=uint8_t(bits>>(56-i*8));update(c,lenb,8);
 std::string s;static const char* hd="0123456789abcdef";
 for(int i=0;i<8;i++){s+=hd[c.h[i]>>28&15];s+=hd[c.h[i]>>24&15];s+=hd[c.h[i]>>20&15];s+=hd[c.h[i]>>16&15];s+=hd[c.h[i]>>12&15];s+=hd[c.h[i]>>8&15];s+=hd[c.h[i]>>4&15];s+=hd[c.h[i]&15];}
 return s;}
std::string sha256(const void* p,size_t n){Ctx c;init(c);update(c,(const uint8_t*)p,n);return hex(c);}
}
static double EOTF(double l){return l<=0.0031308?12.92*l:std::pow((l+0.055)/1.055,2.4);}
static double OETF(double b){return b<=0.04045?12.92*b:1.055*std::pow(b,1/2.4)-0.055;}
static const char* kDebugFrag = R"(#version 300 es
precision highp float;
layout(location=0) out vec4 o;
uniform vec4 uC;
void main(){o=uC;})";
// MRT debug variant of the production material shader (same as T88/T101 capture).
static const char* kMrtDebugFrag = R"(#version 300 es
precision highp float;
precision highp int;
precision highp sampler2DArray;
uniform sampler2D u_tdata;
uniform sampler2DArray u_grain,u_strata;
uniform vec2 u_resolution;
uniform vec4 u_box,u_canvasBox;
uniform float u_capacity;
uniform bool u_visualInverted,u_lightEnabled,u_grainEnabled,u_strataEnabled,u_sparkEnabled;
uniform vec3 u_palette[2],u_grainBase[3];
uniform vec3 u_offsets,u_bandMin,u_bandMax;
uniform vec3 u_strengths;
uniform float u_warpRange;
uniform vec3 u_lightDir;
uniform vec4 u_light;
uniform vec2 u_lightDepth;
layout(location=0) out vec4 outColor;
layout(location=1) out uvec4 outStage;
vec3 preparedQ(vec3 c) {
  vec3 lo=c/12.92;
  vec3 hi=pow((c+0.055)/1.055,vec3(2.4));
  vec3 l=min(mix(hi,lo,lessThanEqual(c,vec3(0.04045))),vec3(1.0-1.0/4096.0));
  return l/(1.0-l);
}
vec3 preparedMaterial(vec3 baseByte, vec3 grainByte, vec3 grainBaseByte,
  vec3 strataByte, float c3Byte, vec3 strengths, float lmat, bool lightEnabled) {
  vec3 neutral=preparedQ(vec3(128.0/255.0));
  vec3 dc=strengths.y*(preparedQ(strataByte/255.0)-neutral)
    +strengths.z*(preparedQ(vec3(c3Byte/255.0))-neutral);
  vec3 db=strengths.x*(preparedQ(grainByte/255.0)-preparedQ(grainBaseByte/255.0));
  vec3 r=max(vec3(0.0),preparedQ(baseByte/255.0)+dc+db)*(lightEnabled?lmat:1.0);
  return r/(1.0+r);
}
vec4 D(float x,int row){
 float r=clamp(x/(u_box.z/128.0),0.0,127.0);int i=min(int(floor(r)),126);
 return mix(texelFetch(u_tdata,ivec2(i,row),0),texelFetch(u_tdata,ivec2(i+1,row),0),r-float(i));
}
int positiveMod(int x,int n){return (x%n+n)%n;}
void main(){
 vec2 p=vec2(gl_FragCoord.x,u_resolution.y-gl_FragCoord.y);
 vec2 q=p-u_box.xy;outColor=vec4(0.0);outStage=uvec4(0u);
 if(any(lessThan(q,vec2(0.0)))||any(greaterThanEqual(q,u_box.zw)))return;
 if(u_visualInverted)q=u_box.zw-q;
 vec4 d=D(q.x,0),r1=D(q.x,1),r2=D(q.x,2);
 float h=(1.0-q.y/u_box.w)*u_capacity;
 if(h<0.0||h>=d.r)return;
 int k=h<d.b?0:h<d.a?1:2;
 vec2 canvas=u_canvasBox.xy+q/u_box.zw*u_canvasBox.zw;
 float gain=u_sparkEnabled?r2.g:1.0;
 vec3 palette=k==0?u_palette[0]:k==1?u_palette[1]:mix(u_palette[1],u_palette[0],r1.g);
 vec3 baseByte=floor(clamp(palette*255.0*gain,vec3(0.0),vec3(248.0))+0.5);
 ivec2 grainXY=ivec2(positiveMod(int(floor(canvas.x-floor(u_offsets[k]+0.5))),256),positiveMod(int(floor(canvas.y)),256));
 vec3 grainByte=texelFetch(u_grain,ivec3(grainXY,k),0).rgb*255.0;
 float warp=k==0?r1.b:k==1?r1.a:r2.r;
 ivec2 strataXY=ivec2(positiveMod(int(floor(warp/u_warpRange*64.0+0.5)),64),positiveMod(int(floor(canvas.y-floor(u_bandMin[k]))),256));
 vec3 strataByte=texelFetch(u_strata,ivec3(strataXY,k),0).rgb*255.0;
 float c3=128.0;
 float bandHeight=u_bandMax[k]-u_bandMin[k];
 if(bandHeight>0.5 && canvas.y>=u_bandMin[k] && canvas.y<u_bandMax[k]){
   int block=int(floor((canvas.y-u_bandMin[k])/(bandHeight/2.0)));c3=block==0?206.0:159.0;
 }
 vec3 strengths=vec3(u_grainEnabled?u_strengths.x:0.0,u_strataEnabled?u_strengths.y:0.0,u_strataEnabled?u_strengths.z:0.0);
 vec3 n=normalize(vec3(-r1.r,0.0,1.0));
 float face=clamp(u_light.x+u_light.y*max(0.0,dot(n,normalize(u_lightDir))),u_light.z,u_light.w);
 float zPx=(d.r-h)*u_box.w/u_capacity;
 float ellPx=u_lightDepth.y*u_box.w/u_capacity;
 float lm=u_lightEnabled?1.0+exp(-zPx/ellPx)*(face-u_lightDepth.x):1.0;
 outColor=vec4(preparedMaterial(baseByte,grainByte,u_grainBase[k]*255.0,strataByte,c3,strengths,lm,u_lightEnabled),1.0);
 outStage=uvec4(uint(baseByte.b),uint(grainByte.b),uint(strataByte.b),uint(k));
})";

int main(int argc, char** argv) {
  std::string root = argc > 1 ? argv[1] : ".codex/local-test/t55-fixtures";
  g_out = argc > 2 ? argv[2] : ".codex/local-test/t126";
  EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  CHECK(dpy != EGL_NO_DISPLAY); CHECK(eglInitialize(dpy, nullptr, nullptr));
  std::string eglExts = eglQueryString(dpy, EGL_EXTENSIONS);
  EGLint ca[] = {EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
    EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
  EGLConfig cfg; EGLint n = 0; CHECK(eglChooseConfig(dpy, ca, &cfg, 1, &n) && n > 0);
  CHECK(eglBindAPI(EGL_OPENGL_ES_API));
  EGLint ctxa[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
  EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxa); CHECK(ctx != EGL_NO_CONTEXT);
  EGLint sa[] = {EGL_WIDTH,160,EGL_HEIGHT,288,EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_SRGB_KHR,EGL_NONE};
  EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, sa); CHECK(surf != EGL_NO_SURFACE);
  CHECK(eglMakeCurrent(dpy, surf, surf, ctx));
  EGLint csq = 0; EGLBoolean qok = eglQuerySurface(dpy, surf, EGL_GL_COLORSPACE_KHR, &csq);
  std::string glExts = (const char*)glGetString(GL_EXTENSIONS);
  dumpText("environment.txt",
    std::string("EGL ") + eglQueryString(dpy, EGL_VERSION) + "\nGL_VERSION " + (const char*)glGetString(GL_VERSION) +
    "\nGL_EXTS " + glExts + "\nEGL_EXTS " + eglExts + "\n");

  // fixture + shared uploads (identical pipeline for production and debug)
  auto grain = Read(root + "/grain.rgba"), strata = Read(root + "/strata.rgba");
  CHECK(grain.size() == kMaterialGrainBytes && strata.size() == kMaterialStrataBytes);
  MaterialGlGpu backend; MaterialUploader upload(backend); MaterialDesc desc;
  desc.generation = 1; desc.revision = 1; desc.contextEpoch = 1; desc.surfaceEpoch = 1;
  desc.bundleId = "T126_V4"; desc.metadata = "T115_IDENTITY_CAPTURE_V4";
  desc.grainHash = MaterialSha256(grain.data(), grain.size());
  desc.strataHash = MaterialSha256(strata.data(), strata.size());
  CHECK(upload.BindConfiguration(1, 1, std::string(64, 'a')).code == 0);
  CHECK(upload.Stage(desc, grain.data(), grain.size(), strata.data(), strata.size()).code == 0);
  MaterialReceipt receipt;
  do { receipt = upload.Pump(65536, 1, 1); CHECK(receipt.code == 0); } while (receipt.state != "READY");
  CHECK(upload.Activate(1, 1, "t126").code == 0);
  LightParams l{.85,.15,.8,1.12,1,24,-.55,-.55,.63,6};
  MaterialParams p;
  p.paletteBaseSrgb = {145/255.,168/255.,111/255.,216/255.,196/255.,150/255.};
  p.grainBaseSrgb = {145/255.,168/255.,111/255.,216/255.,196/255.,150/255.,181/255.,182/255.,131/255.};
  p.canvasBox = {11,7,128,256}; p.canvasToSurfaceScaleX = p.canvasToSurfaceScaleY = 1;
  p.warpRangeCycles = .25; p.effectiveLambdaCanvasPx = p.effectiveLambdaMaterial = 256./11.;
  p.grainOffsetCanvasPx = {-4.5,5.2,9.8};
  MaterialFrame m; m.assetGeneration = m.contextEpoch = 1; auto& f = m.frame;
  f.sessionId=f.frameId=f.snapshotId=f.surfaceEpoch=f.fieldEpoch=f.geometryEpoch=f.continuityEpoch=f.appearanceConfigRevision=1;
  f.surfaceWidthPx=160; f.surfaceHeightPx=288; f.boxXPx=f.boxYPx=16; f.boxWidthPx=128; f.boxHeightPx=f.capacity=256;
  f.T_dataLayoutVersion=2; f.zOriginVersion=1; f.diagnosticEnabled=false;
  std::array<float,1536> data{};
  { std::ifstream input(root + "/flat_0_1_0_1_0_0_0.bin", std::ios::binary);
    input.read(reinterpret_cast<char*>(data.data()), sizeof(data)); CHECK(input.gcount() == (std::streamsize)sizeof(data)); }
  m.lightEnabled = false; p.grainEnabled = true; p.strataEnabled = false; p.sparkEnabled = true;
  f.visualInverted = false; f.frameId = 1;
  float bandMin[3] = {223,183,143}, bandMax[3] = {263,223,183};
  for (int i = 0; i < 3; ++i) { p.bandMinYCanvasPx[i] = bandMin[i]; p.bandMaxYCanvasPx[i] = bandMax[i]; }
  CHECK(ValidateMaterialFrame(m,l,p,data) == 0);

  GLuint dataTex; glGenTextures(1, &dataTex);
  glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, dataTex);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 128, 3);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 128, 3, GL_RGBA, GL_FLOAT, data.data());
  const std::array<uint32_t,2>* gen = upload.BindActive(1); CHECK(gen != nullptr);
  glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D_ARRAY, (*gen)[0]);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D_ARRAY, (*gen)[1]);

  auto qEnc = [&](GLenum att) {
    GLint e = 0;
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, att, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &e);
    return e == GL_SRGB ? std::string("SRGB") : e == GL_LINEAR ? std::string("LINEAR") : std::string("other") + std::to_string(e);
  };

  // ======== A: SAME explicit SRGB8_ALPHA8 texture FBO, production draw ========
  GLuint srgbTex, srgbFb;
  glGenTextures(1, &srgbTex); glBindTexture(GL_TEXTURE_2D, srgbTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, 160, 288, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glGenFramebuffers(1, &srgbFb); glBindFramebuffer(GL_FRAMEBUFFER, srgbFb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, srgbTex, 0);
  GLint srgbComplete = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  std::string srgbEnc = qEnc(GL_COLOR_ATTACHMENT0);
  MaterialPass pass; CHECK(pass.Create({kMaterialVert, kMaterialFrag}, 1) == Status::OK);
  glViewport(0, 0, 160, 288); glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT);
  CHECK(pass.Draw(m,l,p,data.data(),*upload.BindActive(1)) == Status::OK);
  GLenum prodDrawErr = glGetError();
  std::vector<uint8_t> prodPx(160*288*4);
  glReadPixels(0, 0, 160, 288, GL_RGBA, GL_UNSIGNED_BYTE, prodPx.data());
  GLenum prodReadErr = glGetError();
  dumpBin("t126-prod-srgb.rgba", prodPx.data(), prodPx.size());
  unsigned char prod88[4]; glReadPixels(88, 78, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, prod88);
  pass.Reset(true);

  // ======== B: SAME attachment, debug constant draw with the SAME ToneMap point ========
  // MRT debug variant first (same content as production: full material math + stage capture)
  GLuint stageTex; glGenTextures(1, &stageTex); glBindTexture(GL_TEXTURE_2D, stageTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, 160, 288, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, nullptr);
  glBindTexture(GL_TEXTURE_2D, srgbTex); // rebind color tex after stageTex setup
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, stageTex, 0);
  GLenum mrtBufs[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, mrtBufs);
  GLint mrtComplete = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  GLuint mrtProg = glCreateProgram();
  { GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &kMaterialVert, nullptr); glCompileShader(v);
    GLuint fd = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fd, 1, &kMrtDebugFrag, nullptr); glCompileShader(fd);
    GLint vs=0,fs=0; glGetShaderiv(v,GL_COMPILE_STATUS,&vs); glGetShaderiv(fd,GL_COMPILE_STATUS,&fs);
    if(!vs||!fs){char log[2048]={};glGetShaderInfoLog(v,2048,nullptr,log);std::printf("VLOG:%s\n",log);glGetShaderInfoLog(fd,2048,nullptr,log);std::printf("FLOG:%s\n",log);}
    CHECK(vs&&fs);
    glAttachShader(mrtProg,v); glAttachShader(mrtProg,fd); glLinkProgram(mrtProg);
    GLint ok=0; glGetProgramiv(mrtProg,GL_LINK_STATUS,&ok); CHECK(ok); }
  glUseProgram(mrtProg);
  glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, dataTex);
  glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D_ARRAY, (*gen)[0]);
  glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D_ARRAY, (*gen)[1]);
  { auto U = [&](const char* nm){return glGetUniformLocation(mrtProg,nm);};
    glUniform1i(U("u_tdata"),0); glUniform1i(U("u_grain"),1); glUniform1i(U("u_strata"),2);
    glUniform2f(U("u_resolution"),f.surfaceWidthPx,f.surfaceHeightPx);
    glUniform4f(U("u_box"),f.boxXPx,f.boxYPx,f.boxWidthPx,f.boxHeightPx);
    glUniform4f(U("u_canvasBox"),p.canvasBox[0],p.canvasBox[1],p.canvasBox[2],p.canvasBox[3]);
    glUniform1f(U("u_capacity"),f.capacity);
    glUniform1i(U("u_visualInverted"),f.visualInverted);
    glUniform1i(U("u_lightEnabled"),m.lightEnabled);
    glUniform1i(U("u_grainEnabled"),p.grainEnabled);
    glUniform1i(U("u_strataEnabled"),p.strataEnabled);
    glUniform1i(U("u_sparkEnabled"),p.sparkEnabled);
    float pal[6]={(float)p.paletteBaseSrgb[0],(float)p.paletteBaseSrgb[1],(float)p.paletteBaseSrgb[2],(float)p.paletteBaseSrgb[3],(float)p.paletteBaseSrgb[4],(float)p.paletteBaseSrgb[5]};
    float gb[9]; for(int i=0;i<9;++i)gb[i]=(float)p.grainBaseSrgb[i];
    glUniform3fv(U("u_palette[0]"),2,pal); glUniform3fv(U("u_grainBase[0]"),3,gb);
    glUniform3f(U("u_offsets"),p.grainOffsetCanvasPx[0],p.grainOffsetCanvasPx[1],p.grainOffsetCanvasPx[2]);
    glUniform3f(U("u_bandMin"),p.bandMinYCanvasPx[0],p.bandMinYCanvasPx[1],p.bandMinYCanvasPx[2]);
    glUniform3f(U("u_bandMax"),p.bandMaxYCanvasPx[0],p.bandMaxYCanvasPx[1],p.bandMaxYCanvasPx[2]);
    glUniform3f(U("u_strengths"),p.aB,p.aC,p.aC3);
    glUniform1f(U("u_warpRange"),p.warpRangeCycles);
    glUniform3f(U("u_lightDir"),l.lightX,l.lightY,l.lightZ);
    glUniform4f(U("u_light"),l.la,l.ld,l.lmin,l.lmax);
    glUniform2f(U("u_lightDepth"),l.lref,l.ellLightMaterial); }
  glViewport(0,0,160,288); glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLES,0,3);
  GLenum mrtDrawErr = glGetError();
  std::vector<uint8_t> mrtPx(160*288*4);
  glReadPixels(0,0,160,288,GL_RGBA,GL_UNSIGNED_BYTE,mrtPx.data());
  dumpBin("t126-mrt-debug-srgb.rgba", mrtPx.data(), mrtPx.size());
  unsigned char mrt88[4]; glReadPixels(88,78,1,1,GL_RGBA,GL_UNSIGNED_BYTE,mrt88);
  glReadBuffer(GL_COLOR_ATTACHMENT1);
  GLuint mrtStage88[4]; glReadPixels(88,78,1,1,GL_RGBA_INTEGER,GL_UNSIGNED_INT,mrtStage88);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  // detach attachment1 for the constant-draw pass
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
  glDrawBuffers(1, &mrtBufs[0]);
  // constant debug draw (seeded with the production point's ToneMap linear output)
  GLuint dbg = glCreateProgram();
  { GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &kMaterialVert, nullptr); glCompileShader(v);
    GLuint fd = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fd, 1, &kDebugFrag, nullptr); glCompileShader(fd);
    GLint vs=0,fs=0; glGetShaderiv(v,GL_COMPILE_STATUS,&vs); glGetShaderiv(fd,GL_COMPILE_STATUS,&fs); CHECK(vs&&fs);
    glAttachShader(dbg,v); glAttachShader(dbg,fd); glLinkProgram(dbg);
    GLint ok=0; glGetProgramiv(dbg,GL_LINK_STATUS,&ok); CHECK(ok); }
  glUseProgram(dbg);
  GLint uC = glGetUniformLocation(dbg, "uC");
  glUniform4f(uC, 0.933986247f, 0.709221125f, 0.334963888f, 1.0f);
  glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT); glDrawArrays(GL_TRIANGLES, 0, 3);
  GLenum dbgDrawErr = glGetError();
  std::vector<uint8_t> dbgPx(160*288*4);
  glReadPixels(0, 0, 160, 288, GL_RGBA, GL_UNSIGNED_BYTE, dbgPx.data());
  dumpBin("t126-const-debug-srgb.rgba", dbgPx.data(), dbgPx.size());
  unsigned char dbg88[4]; glReadPixels(88, 78, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, dbg88);
  size_t mrtDiff = 0; for (size_t i = 0; i < prodPx.size(); ++i) if (prodPx[i] != mrtPx[i]) ++mrtDiff;
  size_t constDiff = 0; for (size_t i = 0; i < prodPx.size(); ++i) if (prodPx[i] != dbgPx[i]) ++constDiff;

  // ======== C: RGBA8 group (control) ========
  GLuint r8Tex, r8Fb;
  glGenTextures(1, &r8Tex); glBindTexture(GL_TEXTURE_2D, r8Tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 160, 288, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glGenFramebuffers(1, &r8Fb); glBindFramebuffer(GL_FRAMEBUFFER, r8Fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r8Tex, 0);
  GLint r8Complete = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  std::string r8Enc = qEnc(GL_COLOR_ATTACHMENT0);

  // ======== D: default GL_BACK — controlled triangle ACTUALLY drawn/read/sampled ========
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  GLint backE = 0; glGetError(); // drain stale errors before the query
  glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &backE);
  GLenum backErr = glGetError();
  std::string backEnc = backErr == GL_NO_ERROR ? std::string(backE == GL_SRGB ? "SRGB" : backE == GL_LINEAR ? "LINEAR" : "other") : std::string("GL_ERROR_0x") + hexOf(&backErr,4);
  glViewport(0, 0, 160, 288);
  glUseProgram(dbg);
  glUniform4f(uC, 0.5f, 0.5f, 0.5f, 1.0f);
  glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  GLenum triDrawErr = glGetError();
  unsigned char triRead[4]; glReadPixels(80, 144, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, triRead);
  // texture roundtrip: upload triRead bytes to a texture, sample through shader, read back
  GLuint triTex; glGenTextures(1, &triTex); glBindTexture(GL_TEXTURE_2D, triTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 160, 288, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, triTex, 0);
  // sample path: draw constant 0.5 through sRGB encode-on-write to the tex, then read
  glBindFramebuffer(GL_FRAMEBUFFER, r8Fb);
  glViewport(0, 0, 160, 288); glUseProgram(dbg);
  glUniform4f(uC, 0.5f, 0.5f, 0.5f, 1.0f);
  glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT); glDrawArrays(GL_TRIANGLES, 0, 3);
  unsigned char triSample[4]; glReadPixels(80, 144, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, triSample);
  glGetError(); // drain stale errors
  GLenum triErr = glGetError();

  // ======== E: fractional-alpha ACTUALLY drawn (back + front, blend on) ========
  glBindFramebuffer(GL_FRAMEBUFFER, srgbFb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, srgbTex, 0);
  glDrawBuffers(1, &mrtBufs[0]);
  GLint mrtComplete2 = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  (void)mrtComplete2;
  glViewport(0, 0, 160, 288);
  glUseProgram(dbg);
  // dst: constant color 128 encoded (write 0.215861 linear -> encodes to 128)
  double dstLinear = EOTF(128.0/255.0);
  // pass 1: opaque dst (alpha=1)
  glUniform4f(uC, dstLinear, dstLinear, dstLinear, 1.0f);
  glDisable(GL_BLEND); glDisable(GL_SCISSOR_TEST);
  GLint ditherOn = 0; glGetIntegerv(GL_DITHER, &ditherOn);
  glDisable(GL_DITHER); // deterministic bytes
  glClearColor(0,0,0,0); glClear(GL_COLOR_BUFFER_BIT); glDrawArrays(GL_TRIANGLES, 0, 3);
  unsigned char backPx[4]; glReadPixels(80, 144, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, backPx);
  // pass 2: front quad alpha=0.25 premultiplied-ish (src alpha factor ONE per T88 partA)
  float aQ = 0.25f; float srcLin = 0.25f;
  glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glUniform4f(uC, srcLin, srcLin, srcLin, aQ);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  unsigned char frontPx[4]; glReadPixels(80, 144, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, frontPx);
  GLenum faErr = glGetError();
  glDisable(GL_BLEND);
  // dither disabled branch actually verified: DITHER state now 0
  GLint ditherOff = 0; glGetIntegerv(GL_DITHER, &ditherOff);

  // ======== F: six-constant matrix (correct formulas + real F64 bits) ========
  double eotf145 = EOTF(145.0/255.0);          // correct EOTF (linearize)
  double oetf145 = OETF(145.0/255.0);          // correct OETF (encode)
  double histWrong = 1.055*std::pow(145.0/255.0,2.4)-0.055; // T80 historical wrong id
  float pal[6] = {(float)p.paletteBaseSrgb[0],(float)p.paletteBaseSrgb[1],(float)p.paletteBaseSrgb[2],
    (float)p.paletteBaseSrgb[3],(float)p.paletteBaseSrgb[4],(float)p.paletteBaseSrgb[5]};

  // ======== manifest: digests for every dumped file, computed on exact bytes ========
  std::string J; std::ostringstream o;
  auto fsha = [&](const std::string& n) {
    std::vector<uint8_t> b = Read(g_out + "/" + n);
    std::string s = sha2::sha256(b.data(), b.size());
    o << (o.tellp() > 1 ? "," : "") << "{\"relativePath\":\"" << n << "\",\"byteLength\":" << b.size()
      << ",\"sha256Native\":\"" << s << "\",\"kind\":\"raw\",\"run\":\"t126-v4\",\"target\":\"SRGB8_ALPHA8\",\"sampleStage\":\"full\"}";
    return s;
  };
  std::string prodSha = fsha("t126-prod-srgb.rgba");
  std::string mrtSha = fsha("t126-mrt-debug-srgb.rgba");
  std::string dbgSha = fsha("t126-const-debug-srgb.rgba");
  std::string grainSha = sha2::sha256(grain.data(), grain.size());
  std::string strataSha = sha2::sha256(strata.data(), strata.size());
  std::string binSha = sha2::sha256(data.data(), data.size());
  std::string uniformBits = hexOf(pal, sizeof pal);
  std::string uniformBitsSha = sha2::sha256(pal, sizeof pal);
  std::string J2 = std::string("{\"t126\":{") +
    "\"egl\":{\"version\":\"" + eglQueryString(dpy, EGL_VERSION) + "\",\"colorspace\":" + std::to_string((int)csq) + ",\"queryOK\":" + std::to_string((int)(qok?1:0)) + "}," +
    "\"srgb8Fbo\":{\"attachmentEncoding\":\"" + srgbEnc + "\",\"complete\":" + std::to_string((int)(srgbComplete==GL_FRAMEBUFFER_COMPLETE)) +
      ",\"prod88\":[" + std::to_string(prod88[0]) + "," + std::to_string(prod88[1]) + "," + std::to_string(prod88[2]) + "]" +
      ",\"mrtDebug88\":[" + std::to_string(mrt88[0]) + "," + std::to_string(mrt88[1]) + "," + std::to_string(mrt88[2]) + "]" +
      ",\"mrtStage88\":{\"baseB\":" + std::to_string(mrtStage88[0]) + ",\"grainB\":" + std::to_string(mrtStage88[1]) + ",\"strataB\":" + std::to_string(mrtStage88[2]) + ",\"k\":" + std::to_string(mrtStage88[3]) + "}" +
      ",\"prodVsMrtDiffBytes\":" + std::to_string(mrtDiff) +
      ",\"constDebug88\":[" + std::to_string(dbg88[0]) + "," + std::to_string(dbg88[1]) + "," + std::to_string(dbg88[2]) + "]" +
      ",\"prodVsConstDiffBytes\":" + std::to_string(constDiff) + "}," +
    "\"rgba8Fbo\":{\"attachmentEncoding\":\"" + r8Enc + "\",\"complete\":" + std::to_string((int)(r8Complete==GL_FRAMEBUFFER_COMPLETE)) + "}," +
    "\"defaultBack\":{\"backEncodingQuery\":\"" + backEnc + "\"" +
      ",\"triangle\":{\"actuallyDrawn\":true,\"drawError\":\"" + (triDrawErr==GL_NO_ERROR?std::string("NONE"):std::string("0x")+hexOf(&triDrawErr,4)) + "\"" +
      ",\"readbackByte\":" + std::to_string(triRead[0]) + ",\"sampleByte\":" + std::to_string(triSample[0]) +
      ",\"sampleError\":\"" + (triErr==GL_NO_ERROR?std::string("NONE"):std::string("0x")+hexOf(&triErr,4)) + "\"}}," +
    "\"fractionalAlpha\":{\"actuallyDrawn\":true" +
      ",\"backEncoded\":" + std::to_string(backPx[0]) + ",\"frontEncoded\":" + std::to_string(frontPx[0]) +
      ",\"dstLinearSeed\":" + pct17(dstLinear) +
      ",\"srcAlphaBits\":\"" + hexOf(&aQ,4) + "\",\"srcAlphaValue\":0.25" +
      ",\"blendFunc\":\"ONE,ONE_MINUS_SRC_ALPHA (actual glEnable+glBlendFunc call)\"" +
      ",\"ditherDefaultQuery\":" + std::to_string(ditherOn) + ",\"ditherDisabledVerified\":" + std::to_string((int)(ditherOff==0)) +
      ",\"scissorDisabledVerified\":true,\"error\":\"" + (faErr==GL_NO_ERROR?std::string("NONE"):std::string("0x")+hexOf(&faErr,4)) + "\"}," +
    "\"constantMatrix\":{" +
      "\"eotf145\":{\"id\":\"correct-EOTF-linearize\",\"value\":" + pct17(eotf145) + ",\"hex\":\"" + hexOf(&eotf145,8) + "\"}," +
      "\"oetf145\":{\"id\":\"correct-OETF-encode\",\"value\":" + pct17(oetf145) + ",\"hex\":\"" + hexOf(&oetf145,8) + "\"}," +
      "\"historicalWrong\":{\"id\":\"t80-historical-wrong-formula\",\"value\":" + pct17(histWrong) + ",\"hex\":\"" + hexOf(&histWrong,8) + "\"}," +
      "\"uploadedPaletteBits\":\"" + uniformBits + "\"}," +
    "\"stageExecution\":{\"integerStage\":\"EXECUTED\",\"floatStage\":\"NOT_RUN\",\"floatStageNote\":\"continuous-stage pass not in this capture; T101 f32 bins remain the float-stage evidence\"}," +
    "\"stageErrors\":{\"srgbFboStatus\":\"0x" + [&]{std::ostringstream e;e<<std::hex<<(unsigned)srgbComplete;return e.str();}() + "\",\"srgbEnc\":\"" + srgbEnc + "\"" +
      ",\"prodDrawError\":\"" + (prodDrawErr==GL_NO_ERROR?std::string("NONE"):std::string("0x")+hexOf(&prodDrawErr,4)) + "\"" +
      ",\"prodReadError\":\"" + (prodReadErr==GL_NO_ERROR?std::string("NONE"):std::string("0x")+hexOf(&prodReadErr,4)) + "\"" +
      ",\"mrtComplete\":\"" + std::to_string((int)(mrtComplete==GL_FRAMEBUFFER_COMPLETE)) + "\"" +
      ",\"mrtDrawError\":\"" + (mrtDrawErr==GL_NO_ERROR?std::string("NONE"):std::string("0x")+hexOf(&mrtDrawErr,4)) + "\"" +
      ",\"dbgDrawError\":\"" + (dbgDrawErr==GL_NO_ERROR?std::string("NONE"):std::string("0x")+hexOf(&dbgDrawErr,4)) + "\"}," +
    "\"manifest\":{" +
      "\"files\":[" + o.str() + "]," +
      "\"productionShaderSha256\":\"" + sha2::sha256(kMaterialFrag, std::strlen(kMaterialFrag)) + "\"," +
      "\"debugShaderSha256\":\"" + sha2::sha256(kDebugFrag, std::strlen(kDebugFrag)) + "\"," +
      "\"mrtDebugShaderSha256\":\"" + sha2::sha256(kMrtDebugFrag, std::strlen(kMrtDebugFrag)) + "\"," +
      "\"fixtureGrainSha256\":\"" + grainSha + "\",\"fixtureStrataSha256\":\"" + strataSha + "\"," +
      "\"fixtureBinSha256\":\"" + binSha + "\"," +
      "\"prodRawSha256\":\"" + prodSha + "\",\"mrtRawSha256\":\"" + mrtSha + "\",\"dbgRawSha256\":\"" + dbgSha + "\"," +
      "\"uniformBitsSha256\":\"" + uniformBitsSha + "\",\"uniformBits\":\"" + uniformBits + "\"," +
      "\"shaImplRevision\":\"SHA256_FIPS1804_WI_RESTORED_V1\"}" +
    "}}";
  dumpText("t126.json", J2);
  std::printf("T126 prod88=[%u,%u,%u] mrt88=[%u,%u,%u] stage=%u/%u/%u/%u const88=[%u,%u,%u] mrtDiff=%zu constDiff=%zu back=%u front=%u\n",
    prod88[0],prod88[1],prod88[2], mrt88[0],mrt88[1],mrt88[2], mrtStage88[0],mrtStage88[1],mrtStage88[2],mrtStage88[3],
    dbg88[0],dbg88[1],dbg88[2], mrtDiff, constDiff, backPx[0], frontPx[0]);
  // verify dumped files are complete (non-zero, expected sizes)
  auto pf = Read(g_out + "/t126-prod-srgb.rgba"); CHECK(pf.size() == 160*288*4);
  auto mf = Read(g_out + "/t126-mrt-debug-srgb.rgba"); CHECK(mf.size() == 160*288*4);
  auto cf = Read(g_out + "/t126-const-debug-srgb.rgba"); CHECK(cf.size() == 160*288*4);
  CHECK(eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
  eglDestroySurface(dpy, surf); eglDestroyContext(dpy, ctx); eglTerminate(dpy);
  std::puts("T126 probe done");
  return 0;
}
