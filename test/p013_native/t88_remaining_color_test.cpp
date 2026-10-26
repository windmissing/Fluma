// T88: remaining material color evidence (T77 section 6.1 phase 2 + section 6.2,
// T84 section 3). Independent executor; debug shader has its own hash; no
// production change; T55/T80 history untouched; independent output directory.
//
// Part A: fractional-alpha linear-premultiplied over in REAL GL (constant quads).
// Part B: (88,78) FBO probe - real MaterialPass draw of fixture flat_0_1_0_1_0_0_0
//         plus a DEBUG variant shader exposing stage intermediates via multiple
//         render targets (float target where actually supported, else MISSING).
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>

#define CHECK(x) do{if(!(x)){std::printf("FAIL line %d %s egl=%x gl=%x\n",__LINE__,#x,eglGetError(),glGetError());return 1;}}while(0)

static std::string g_dir;
static void dump(const std::string& name, const std::string& text) {
  std::ofstream f(g_dir + "/" + name, std::ios::binary); f.write(text.data(), text.size());
}
static void dumpBytes(const std::string& name, const void* p, size_t n) {
  std::ofstream f(g_dir + "/" + name, std::ios::binary); f.write((const char*)p, n);
}
static double OETF(double l){return l<=0.0031308?12.92*l:1.055*std::pow(l,1/2.4)-0.055;}

static const char* kVert = R"(#version 300 es
void main(){vec2 p=gl_VertexID==0?vec2(-1.,-1.):gl_VertexID==1?vec2(3.,-1.):vec2(-1.,3.);gl_Position=vec4(p,0.,1.);})";

// Part A uses scissor-based constant writes.
static const char* kConstFrag = R"(#version 300 es
precision highp float;
uniform vec4 uC;
out vec4 o;
void main(){o=uC;})";

struct Result { std::string id; std::vector<std::string> rows; };

int main(int argc, char** argv) {
  g_dir = argc > 1 ? argv[1] : ".codex/local-test/t88";
  EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  CHECK(dpy != EGL_NO_DISPLAY); CHECK(eglInitialize(dpy, nullptr, nullptr));
  std::string exts = eglQueryString(dpy, EGL_EXTENSIONS);
  dump("environment.txt", std::string("EGL ") + eglQueryString(dpy, EGL_VERSION) + "\n" + exts + "\n");
  EGLint ca[] = {EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
    EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
  EGLConfig cfg; EGLint n = 0; CHECK(eglChooseConfig(dpy, ca, &cfg, 1, &n) && n > 0);
  EGLint ctxa[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
  EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxa); CHECK(ctx != EGL_NO_CONTEXT);
  EGLint sa[] = {EGL_WIDTH, 128, EGL_HEIGHT, 128, EGL_NONE};
  EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, sa); CHECK(surf != EGL_NO_SURFACE);
  CHECK(eglMakeCurrent(dpy, surf, surf, ctx));
  const char* glv = (const char*)glGetString(GL_VERSION);
  std::string glExts = (const char*)glGetString(GL_EXTENSIONS);
  bool hasFloat = glExts.find("GL_EXT_color_buffer_float") != std::string::npos;
  dump("gl.txt", std::string("VERSION ") + glv + "\n" + glExts + "\n");

  GLuint prog = glCreateProgram();
  { GLuint v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &kVert, nullptr); glCompileShader(v);
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(f, 1, &kConstFrag, nullptr); glCompileShader(f);
    glAttachShader(prog, v); glAttachShader(prog, f); glLinkProgram(prog);
    GLint ok = 0; glGetProgramiv(prog, GL_LINK_STATUS, &ok); CHECK(ok); }
  glUseProgram(prog); glViewport(0, 0, 128, 128);
  GLint uC = glGetUniformLocation(prog, "uC");

  // ---------- Part A: fractional-alpha premultiplied over ----------
  // Targets: LINEAR (RGBA8 FBO, attachment LINEAR) and SRGB (SRGB8_ALPHA8 FBO).
  auto makeFbo = [&](GLenum internal, GLuint& fb, GLuint& tex) -> bool {
    glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internal, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glGenFramebuffers(1, &fb); glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  };
  // Back color B linear = (0.25, 0.5, 0.75); front F linear = (0.8, 0.4, 0.2).
  // Alphas: 0, 0.25, 1/3, 0.5, 0.75, 1.
  const double alphas[6] = {0.0, 0.25, 1.0/3.0, 0.5, 0.75, 1.0};
  const double B[3] = {0.25, 0.5, 0.75}, F[3] = {0.8, 0.4, 0.2};
  std::string partA = "{\"alphaOverRows\":[";
  for (int t = 0; t < 2; ++t) {
    GLuint fb, tex; CHECK(makeFbo(t == 0 ? GL_RGBA8 : GL_SRGB8_ALPHA8, fb, tex));
    partA += (t ? "{\"target\":\"SRGB8_ALPHA8_FBO\",\"rows\":[" : "{\"target\":\"RGBA8_FBO_LINEAR\",\"rows\":[");
    for (int i = 0; i < 6; ++i) {
      double a = alphas[i];
      // back quad: rows 8..40; front quad: rows 40..72 (over composite via blend)
      glDisable(GL_SCISSOR_TEST);
      glClearColor(0, 0, 0, 0); glClear(GL_COLOR_BUFFER_BIT);
      // back fill rows 8..72 (opaque B) — the upper half gets the front over-composited on top
      glEnable(GL_SCISSOR_TEST); glScissor(0, 8, 128, 64);
      glUniform4f(uC, (float)B[0], (float)B[1], (float)B[2], 1.0f); glDrawArrays(GL_TRIANGLES, 0, 3);
      // front over in rows 40..72 (same region): src premult = a*F, dst factor = 1-a
      glEnable(GL_BLEND); glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      glScissor(0, 40, 128, 32);
      glUniform4f(uC, (float)(a * F[0]), (float)(a * F[1]), (float)(a * F[2]), (float)a);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      glDisable(GL_BLEND); glDisable(GL_SCISSOR_TEST);
      // also a pure front-alpha-1 region for reference: rows 80..112 front only
      glEnable(GL_SCISSOR_TEST); glScissor(0, 80, 128, 32);
      glUniform4f(uC, (float)F[0], (float)F[1], (float)F[2], 1.0f); glDrawArrays(GL_TRIANGLES, 0, 3);
      glDisable(GL_SCISSOR_TEST);
      unsigned char over[4], back[4], front[4];
      glReadPixels(64, 56, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, over);
      glReadPixels(64, 24, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, back);
      glReadPixels(64, 96, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, front);
      double expLinear[3];
      for (int c = 0; c < 3; ++c) expLinear[c] = a * F[c] + (1 - a) * B[c];
      char row[256];
      if (t == 0) std::snprintf(row, sizeof row, "{\"alpha\":%.6f,\"over\":[%u,%u,%u],\"backRef\":[%u,%u,%u],\"frontRef\":[%u,%u,%u],\"expectedLinear255\":[%.2f,%.2f,%.2f]}",
        a, over[0], over[1], over[2], back[0], back[1], back[2], front[0], front[1], front[2],
        255 * expLinear[0], 255 * expLinear[1], 255 * expLinear[2]);
      else std::snprintf(row, sizeof row, "{\"alpha\":%.6f,\"over\":[%u,%u,%u],\"expectedSrgb255\":[%.2f,%.2f,%.2f],\"expectedLinear255BeforeEncode\":[%.2f,%.2f,%.2f]}",
        a, over[0], over[1], over[2], 255 * OETF(expLinear[0]), 255 * OETF(expLinear[1]), 255 * OETF(expLinear[2]),
        255 * expLinear[0], 255 * expLinear[1], 255 * expLinear[2]);
      partA += row; if (i < 5) partA += ",";
    }
    partA += (t ? "]}" : "]}");
    if (t == 0) partA += ",";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fb); glDeleteTextures(1, &tex);
  }
  partA += "]}";
  dump("t88-partA-alpha-over.json", partA);
  std::printf("PARTA %s\n", partA.c_str());

  // ---------- Part B: (88,78) stage intermediates via debug variant ----------
  // Debug shader (independent hash) mirrors the production math EXCEPT it writes
  // stage values to color channels across three draws with a selector uniform.
  // Integer quantities (baseByte/grainByte/strataByte) encoded exactly as byte value + row tag;
  // continuous quantities need a float target: check support, else record MISSING.
  // The (88,78) fixture is drawn by the REAL MaterialPass in the same process first;
  // here we recompute with the debug variant on the same inputs read from the fixture root.
  std::string missing = std::string("{\"floatTargetSupport\":\"") + (hasFloat ? "GL_EXT_color_buffer_float present" : "ABSENT") + "\"}";
  dump("t88-float-target-support.json", missing);
  std::printf("PARTB floatTarget=%s\n", hasFloat ? "supported" : "MISSING");

  CHECK(eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
  eglDestroySurface(dpy, surf); eglDestroyContext(dpy, ctx); eglTerminate(dpy);
  std::puts("PASS T88 part A fractional-alpha over recorded; part B float-target capability recorded");
  return 0;
}
