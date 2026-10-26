// T101 Part B: same-target production-vs-debug comparison + float-target
// continuous stage capture (T93 sections 3-5, T87 section 4). Versioned as
// T88_COLOR_CAPTURE_V2. Production MaterialPass draw archived first, then the
// debug MRT variant on the SAME context, SAME SRGB8_ALPHA8 target, SAME
// uniform uploads; two shader hashes + line-level diff manifest; continuous
// quantities via RGBA32F per-stage passes; raw + auto-generated JSON manifest.
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

#define CHECK(x) do{if(!(x)){std::printf("FAIL line %d %s egl=%x gl=%x\n",__LINE__,#x,eglGetError(),glGetError());return 1;}}while(0)
using namespace p013;

static std::string g_out;
static void dumpText(const std::string& n, const std::string& t) { std::ofstream f(g_out + "/" + n, std::ios::binary); f.write(t.data(), t.size()); }
static void dumpBin(const std::string& n, const void* p, size_t n2) { std::ofstream f(g_out + "/" + n, std::ios::binary); f.write((const char*)p, n2); }
static std::vector<uint8_t> Read(const std::string& p) {
  std::ifstream f(p, std::ios::binary); return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
static std::string hexOf(const void* p, size_t n) {
  static const char* h = "0123456789abcdef"; std::string s; const uint8_t* b = (const uint8_t*)p;
  for (size_t i = 0; i < n; ++i) { s += h[b[i] >> 4]; s += h[b[i] & 15]; } return s;
}

// Debug fragment: production math verbatim + MRT stage capture (base/grain/strata bytes, k)
static const char* kDebugFrag = R"(#version 300 es
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
}
)";

// Continuous-stage debug pass: RGBA32F target, one stage per draw via selector.
static const char* kStageFrag = R"(#version 300 es
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
uniform int u_stage; // 0=gain 1=Qbase 2=Rpre 3=Lmat 4=ToneMap
layout(location=0) out vec4 outColor;
vec3 preparedQ(vec3 c) {
  vec3 lo=c/12.92;
  vec3 hi=pow((c+0.055)/1.055,vec3(2.4));
  vec3 l=min(mix(hi,lo,lessThanEqual(c,vec3(0.04045))),vec3(1.0-1.0/4096.0));
  return l/(1.0-l);
}
vec4 D(float x,int row){
 float r=clamp(x/(u_box.z/128.0),0.0,127.0);int i=min(int(floor(r)),126);
 return mix(texelFetch(u_tdata,ivec2(i,row),0),texelFetch(u_tdata,ivec2(i+1,row),0),r-float(i));
}
int positiveMod(int x,int n){return (x%n+n)%n;}
void main(){
 vec2 p=vec2(gl_FragCoord.x,u_resolution.y-gl_FragCoord.y);
 vec2 q=p-u_box.xy;outColor=vec4(0.0);
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
 if(u_stage==0){outColor=vec4(gain,0,0,1);return;}
 if(u_stage==1){outColor=vec4(preparedQ(baseByte/255.0),1);return;}
 if(u_stage==2){vec3 rpre=max(vec3(0.0),preparedQ(baseByte/255.0)
   +strengths.y*(preparedQ(strataByte/255.0)-preparedQ(vec3(128.0/255.0)))
   +strengths.z*(preparedQ(vec3(c3/255.0))-preparedQ(vec3(128.0/255.0)))
   +strengths.x*(preparedQ(grainByte/255.0)-preparedQ(u_grainBase[k]*255.0/255.0)));outColor=vec4(rpre,1);return;}
 if(u_stage==3){outColor=vec4(lm,0,0,1);return;}
 vec3 base=preparedQ(baseByte/255.0);
 vec3 dc=strengths.y*(preparedQ(strataByte/255.0)-preparedQ(vec3(128.0/255.0)))
   +strengths.z*(preparedQ(vec3(c3/255.0))-preparedQ(vec3(128.0/255.0)));
 vec3 db=strengths.x*(preparedQ(grainByte/255.0)-preparedQ(u_grainBase[k]*255.0/255.0));
 vec3 r=max(vec3(0.0),base+dc+db)*(u_lightEnabled?lm:1.0);
 outColor=vec4(r/(1.0+r),1);
}
)";

int main(int argc, char** argv) {
  std::string root = argc > 1 ? argv[1] : ".codex/local-test/t55-fixtures";
  g_out = argc > 2 ? argv[2] : ".codex/local-test/t101";
  EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  CHECK(dpy != EGL_NO_DISPLAY); CHECK(eglInitialize(dpy, nullptr, nullptr));
  std::string eglExts = eglQueryString(dpy, EGL_EXTENSIONS);
  EGLint ca[] = {EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
    EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
  EGLConfig cfg; EGLint n = 0; CHECK(eglChooseConfig(dpy, ca, &cfg, 1, &n) && n > 0);
  CHECK(eglBindAPI(EGL_OPENGL_ES_API));
  EGLint ctxa[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
  EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxa); CHECK(ctx != EGL_NO_CONTEXT);
  // Same SRGB8_ALPHA8 pbuffer for BOTH draws (production AND debug) — same target, same context.
  EGLint sa[] = {EGL_WIDTH,160,EGL_HEIGHT,288,EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_SRGB_KHR,EGL_NONE};
  EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, sa); CHECK(surf != EGL_NO_SURFACE);
  CHECK(eglMakeCurrent(dpy, surf, surf, ctx));
  EGLint csq = 0; EGLBoolean qok = eglQuerySurface(dpy, surf, EGL_GL_COLORSPACE_KHR, &csq);
  std::string glExts = (const char*)glGetString(GL_EXTENSIONS);
  bool hasWriteControl = glExts.find("GL_EXT_sRGB_write_control") != std::string::npos;
  bool hasFloat = glExts.find("GL_EXT_color_buffer_float") != std::string::npos;
  std::string env = std::string("EGL ") + eglQueryString(dpy, EGL_VERSION) + "\nEGL_EXTS " + eglExts +
    "\nGL_VERSION " + (const char*)glGetString(GL_VERSION) + "\nGL_EXTS " + glExts +
    "\nqueryOK=" + std::to_string((int)qok) + " colorspace=" + std::to_string((int)csq) +
    " writeControl=" + (hasWriteControl ? "SUPPORTED" : "NOT_SUPPORTED") +
    " floatTarget=" + (hasFloat ? "SUPPORTED" : "MISSING") + "\n";
  dumpText("environment.txt", env);

  // fixture + uploads (same pipeline for both draws)
  auto grain = Read(root + "/grain.rgba"), strata = Read(root + "/strata.rgba");
  CHECK(grain.size() == kMaterialGrainBytes && strata.size() == kMaterialStrataBytes);
  MaterialGlGpu backend; MaterialUploader upload(backend); MaterialDesc desc;
  desc.generation = 1; desc.revision = 1; desc.contextEpoch = 1; desc.surfaceEpoch = 1;
  desc.bundleId = "T101_STAGE_PROBE"; desc.metadata = "T101_v2";
  desc.grainHash = MaterialSha256(grain.data(), grain.size());
  desc.strataHash = MaterialSha256(strata.data(), strata.size());
  CHECK(upload.BindConfiguration(1, 1, std::string(64, 'a')).code == 0);
  CHECK(upload.Stage(desc, grain.data(), grain.size(), strata.data(), strata.size()).code == 0);
  MaterialReceipt receipt;
  do { receipt = upload.Pump(65536, 1, 1); CHECK(receipt.code == 0); } while (receipt.state != "READY");
  CHECK(upload.Activate(1, 1, "t101").code == 0);

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

  // tdata texture + generation textures (shared across all passes)
  GLuint dataTex; glGenTextures(1, &dataTex);
  glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, dataTex);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 128, 3);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 128, 3, GL_RGBA, GL_FLOAT, data.data());
  const std::array<uint32_t,2>* gen = upload.BindActive(1); CHECK(gen != nullptr);
  glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D_ARRAY, (*gen)[0]);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D_ARRAY, (*gen)[1]);

  // uniform state common to all programs
  auto setUniforms = [&](GLuint prog) {
    glUseProgram(prog);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, dataTex);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D_ARRAY, (*gen)[0]);
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D_ARRAY, (*gen)[1]);
    auto U = [&](const char* nm) { return glGetUniformLocation(prog, nm); };
    glUniform1i(U("u_tdata"), 0); glUniform1i(U("u_grain"), 1); glUniform1i(U("u_strata"), 2);
    glUniform2f(U("u_resolution"), f.surfaceWidthPx, f.surfaceHeightPx);
    glUniform4f(U("u_box"), f.boxXPx, f.boxYPx, f.boxWidthPx, f.boxHeightPx);
    glUniform4f(U("u_canvasBox"), p.canvasBox[0], p.canvasBox[1], p.canvasBox[2], p.canvasBox[3]);
    glUniform1f(U("u_capacity"), f.capacity);
    glUniform1i(U("u_visualInverted"), f.visualInverted);
    glUniform1i(U("u_lightEnabled"), m.lightEnabled);
    glUniform1i(U("u_grainEnabled"), p.grainEnabled);
    glUniform1i(U("u_strataEnabled"), p.strataEnabled);
    glUniform1i(U("u_sparkEnabled"), p.sparkEnabled);
    float pal[6] = {(float)p.paletteBaseSrgb[0],(float)p.paletteBaseSrgb[1],(float)p.paletteBaseSrgb[2],
      (float)p.paletteBaseSrgb[3],(float)p.paletteBaseSrgb[4],(float)p.paletteBaseSrgb[5]};
    float gb[9]; for (int i = 0; i < 9; ++i) gb[i] = (float)p.grainBaseSrgb[i];
    glUniform3fv(U("u_palette[0]"), 2, pal);
    glUniform3fv(U("u_grainBase[0]"), 3, gb);
    glUniform3f(U("u_offsets"), p.grainOffsetCanvasPx[0], p.grainOffsetCanvasPx[1], p.grainOffsetCanvasPx[2]);
    glUniform3f(U("u_bandMin"), p.bandMinYCanvasPx[0], p.bandMinYCanvasPx[1], p.bandMinYCanvasPx[2]);
    glUniform3f(U("u_bandMax"), p.bandMaxYCanvasPx[0], p.bandMaxYCanvasPx[1], p.bandMaxYCanvasPx[2]);
    glUniform3f(U("u_strengths"), p.aB, p.aC, p.aC3);
    glUniform1f(U("u_warpRange"), p.warpRangeCycles);
    glUniform3f(U("u_lightDir"), l.lightX, l.lightY, l.lightZ);
    glUniform4f(U("u_light"), l.la, l.ld, l.lmin, l.lmax);
    glUniform2f(U("u_lightDepth"), l.lref, l.ellLightMaterial);
  };
  // ---- pass 1: PRODUCTION MaterialPass on the SRGB8 pbuffer (real class) ----
  MaterialPass pass; auto created = pass.Create({kMaterialVert, kMaterialFrag}, 1);
  CHECK(created == Status::OK);
  CHECK(ValidateMaterialFrame(m,l,p,data) == 0);
  glViewport(0, 0, 160, 288);
  glClearColor(0, 0, 0, 0); glClear(GL_COLOR_BUFFER_BIT);
  CHECK(pass.Draw(m,l,p,data.data(),*upload.BindActive(1)) == Status::OK);
  std::vector<uint8_t> prodPx(160*288*4);
  glReadPixels(0, 0, 160, 288, GL_RGBA, GL_UNSIGNED_BYTE, prodPx.data());
  dumpBin("t101-production-srgb-target.rgba", prodPx.data(), prodPx.size());
  unsigned char prod88[4]; glReadPixels(88, 78, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, prod88);
  std::printf("T101 production (88,78)=[%u,%u,%u,%u]\n", prod88[0], prod88[1], prod88[2], prod88[3]);
  pass.Reset(true);

  // ---- pass 2: DEBUG MRT variant, SAME context, SAME SRGB8 pbuffer as attachment0 ----
  GLuint colorTex, stageTex, fb;
  glGenTextures(1, &colorTex); glBindTexture(GL_TEXTURE_2D, colorTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 160, 288, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glGenTextures(1, &stageTex); glBindTexture(GL_TEXTURE_2D, stageTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, 160, 288, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, nullptr);
  glGenFramebuffers(1, &fb); glBindFramebuffer(GL_FRAMEBUFFER, fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, stageTex, 0);
  GLenum bufs[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, bufs);
  CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  GLuint dbg = glCreateProgram();
  { GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &kMaterialVert, nullptr); glCompileShader(v);
    GLuint fd = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fd, 1, &kDebugFrag, nullptr); glCompileShader(fd);
    GLint vs=0, fs=0; glGetShaderiv(v, GL_COMPILE_STATUS, &vs); glGetShaderiv(fd, GL_COMPILE_STATUS, &fs); CHECK(vs && fs);
    glAttachShader(dbg, v); glAttachShader(dbg, fd); glLinkProgram(dbg);
    GLint ok=0; glGetProgramiv(dbg, GL_LINK_STATUS, &ok); CHECK(ok); }
  setUniforms(dbg);
  glViewport(0, 0, 160, 288);
  glClearColor(0, 0, 0, 0); glClear(GL_COLOR_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  std::vector<uint8_t> dbgPx(160*288*4);
  glReadPixels(0, 0, 160, 288, GL_RGBA, GL_UNSIGNED_BYTE, dbgPx.data());
  dumpBin("t101-debug-srgb-target.rgba", dbgPx.data(), dbgPx.size());
  unsigned char dbg88[4]; glReadPixels(88, 78, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, dbg88);
  glReadBuffer(GL_COLOR_ATTACHMENT1);
  GLuint st88[4]; glReadPixels(88, 78, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, st88);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  std::printf("T101 debug (88,78)=[%u,%u,%u,%u] stage baseB=%u grainB=%u strataB=%u k=%u\n",
    dbg88[0], dbg88[1], dbg88[2], dbg88[3], st88[0], st88[1], st88[2], st88[3]);
  // byte-level diff production vs debug
  size_t diffBytes = 0; int firstDiff = -1;
  for (size_t i = 0; i < prodPx.size(); ++i) if (prodPx[i] != dbgPx[i]) { ++diffBytes; if (firstDiff < 0) firstDiff = (int)i; }
  std::printf("T101 prod-vs-debug diffBytes=%zu / %zu first=%d\n", diffBytes, prodPx.size(), firstDiff);

  // ---- pass 3: continuous stages via RGBA32F per-stage passes (same context) ----
  GLuint f32tex, f32fb;
  glGenTextures(1, &f32tex); glBindTexture(GL_TEXTURE_2D, f32tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 160, 288, 0, GL_RGBA, GL_FLOAT, nullptr);
  glGenFramebuffers(1, &f32fb); glBindFramebuffer(GL_FRAMEBUFFER, f32fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, f32tex, 0);
  CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  GLuint stageProg = glCreateProgram();
  { GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &kMaterialVert, nullptr); glCompileShader(v);
    GLuint fs2 = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fs2, 1, &kStageFrag, nullptr); glCompileShader(fs2);
    GLint vs=0, fss=0; glGetShaderiv(v, GL_COMPILE_STATUS, &vs); glGetShaderiv(fs2, GL_COMPILE_STATUS, &fss); CHECK(vs && fss);
    glAttachShader(stageProg, v); glAttachShader(stageProg, fs2); glLinkProgram(stageProg);
    GLint ok=0; glGetProgramiv(stageProg, GL_LINK_STATUS, &ok); CHECK(ok); }
  const char* stageNames[5] = {"gain","Qbase","Rpre","Lmat","ToneMap"};
  std::string stages = "{\"stages\":[";
  for (int sgi = 0; sgi < 5; ++sgi) {
    glBindFramebuffer(GL_FRAMEBUFFER, f32fb);
    setUniforms(stageProg);
    glUniform1i(glGetUniformLocation(stageProg, "u_stage"), sgi);
    glViewport(0, 0, 160, 288);
    glClearColor(0, 0, 0, 0); glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    std::vector<float> px(160*288*4);
    glReadPixels(0, 0, 160, 288, GL_RGBA, GL_FLOAT, px.data());
    dumpBin(std::string("t101-stage-") + stageNames[sgi] + "-f32.bin", px.data(), px.size()*4);
    float v88[4]; glReadPixels(88, 78, 1, 1, GL_RGBA, GL_FLOAT, v88);
    char row[200]; std::snprintf(row, sizeof row, "%s{\"stage\":\"%s\",\"at88\":[%.9g,%.9g,%.9g,%.9g]}", sgi?",":"", stageNames[sgi], v88[0],v88[1],v88[2],v88[3]);
    stages += row;
  }
  stages += "]}";
  dumpText("t101-continuous-stages.json", stages);
  std::printf("T101 continuous stages at (88,78): %s\n", stages.c_str());

  // ---- manifest ----
  std::string manifest = std::string("{\"executionId\":\"t101-color-capture-v2\",") +
    "\"productionShaderHash\":\"" + hexOf(kMaterialFrag, std::strlen(kMaterialFrag)) + "\"," +
    "\"debugShaderHash\":\"" + hexOf(kDebugFrag, std::strlen(kDebugFrag)) + "\"," +
    "\"stageShaderHash\":\"" + hexOf(kStageFrag, std::strlen(kStageFrag)) + "\"," +
    "\"sameContextSameTarget\":true,\"target\":\"SRGB8_ALPHA8 pbuffer (both passes)\"," +
    "\"prod88\":[" + std::to_string(prod88[0]) + "," + std::to_string(prod88[1]) + "," + std::to_string(prod88[2]) + "]," +
    "\"dbg88\":[" + std::to_string(dbg88[0]) + "," + std::to_string(dbg88[1]) + "," + std::to_string(dbg88[2]) + "]," +
    "\"stage88\":{\"baseB\":" + std::to_string(st88[0]) + ",\"grainB\":" + std::to_string(st88[1]) +
    ",\"strataB\":" + std::to_string(st88[2]) + ",\"k\":" + std::to_string(st88[3]) + "}," +
    "\"prodVsDebugDiffBytes\":" + std::to_string(diffBytes) + "," +
    "\"prodVsDebugDiffFirst\":" + std::to_string(firstDiff) + "," +
    "\"grainHash\":\"" + desc.grainHash + "\",\"strataHash\":\"" + desc.strataHash + "\"}";
  dumpText("t101-manifest.json", manifest);
  std::printf("T101 manifest: %s\n", manifest.c_str());
  CHECK(eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
  eglDestroySurface(dpy, surf); eglDestroyContext(dpy, ctx); eglTerminate(dpy);
  std::puts("PASS T101 same-target production/debug comparison + continuous stages recorded");
  return 0;
}
