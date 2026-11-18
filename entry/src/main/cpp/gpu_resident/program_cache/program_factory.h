#pragma once
#include "../m2/create_diagnostics.h"
#include "artifact_store.h"
#include <sstream>
namespace grs::programcache {
inline uint64_t Stamp() {
  return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count());
}
inline std::string Quote(const std::string &s) {
  std::ostringstream o;
  create468::String(o, s.c_str());
  return o.str();
}
inline std::string packageSource =
    "UNDECLARED"; // Set once before any owner starts. Provenance only.
inline constexpr char kLinkConfig[] =
    "COMPUTE|SEPARABLE=0|EXPLICIT_GLSL_BINDINGS|NO_PRELINK_ATTRIBUTE_BINDINGS";
struct Pending {
  std::string key, role, shader, layout, environment, route, log, shaderLog,
      cacheLog;
  uint64_t begin = 0, compiled = 0, compileBegin = 0, compileEnd = 0,
           linkBegin = 0, linkEnd = 0, loadBegin = 0, loadEnd = 0,
           reflectBegin = 0, reflectEnd = 0, prepareBegin = 0, prepareEnd = 0,
           warmupBegin = 0, warmupEnd = 0;
  GLenum cacheError = 0, originalError = 0;
  GLint reported = 0, shaderReported = 0;
  EGLint cacheEgl = EGL_SUCCESS;
  std::vector<GLint> formats;
  EGLContext context = EGL_NO_CONTEXT;
  std::thread::id owner;
};
inline thread_local std::map<GLuint, Pending> pending;
inline thread_local std::map<GLuint, Pending> qualified;
inline std::string Environment() {
  std::string text = "T475_DRIVER_CONTEXT_V1";
  for (auto token :
       {GL_VENDOR, GL_RENDERER, GL_VERSION, GL_SHADING_LANGUAGE_VERSION}) {
    auto p = glGetString(token);
    if (!p)
      return {};
    std::string value = reinterpret_cast<const char *>(p);
    text += std::to_string(value.size()) + ":" + value;
  }
  GLint major = 0, minor = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  text += "|" + std::to_string(major) + "." + std::to_string(minor);
  auto display = eglGetCurrentDisplay();
  auto context = eglGetCurrentContext();
  for (auto token : {EGL_CONFIG_ID, EGL_CONTEXT_CLIENT_VERSION}) {
    EGLint value = 0;
    if (!eglQueryContext(display, context, token, &value))
      return {};
    text += "|" + std::to_string(token) + "=" + std::to_string(value);
  }
  GLint n = 0;
  glGetIntegerv(GL_NUM_EXTENSIONS, &n);
  if (n < 0 || n > 4096)
    return {};
  std::vector<std::string> extensions;
  for (GLint i = 0; i < n; i++) {
    const auto *p = glGetStringi(GL_EXTENSIONS, GLuint(i));
    if (!p)
      return {};
    extensions.emplace_back(reinterpret_cast<const char *>(p));
  }
  std::sort(extensions.begin(), extensions.end());
  for (auto &e : extensions)
    text += "|" + e;
  return Hash(text);
}
inline std::string Key(const std::string &source, const std::string &layout,
                       const std::string &environment,
                       const std::string &link = kLinkConfig) {
  std::string bytes = "T475_PROGRAM_KEY_V1";
  for (auto *s : {&source, &layout, &environment, &link}) {
    uint64_t size = s->size();
    for (unsigned i = 0; i < 8; i++)
      bytes += char(size >> (i * 8));
    bytes += *s;
  }
  return Hash(bytes);
}
inline std::string Log(GLuint object, GLint &reported, bool shader = false) {
  if (shader)
    glGetShaderiv(object, GL_INFO_LOG_LENGTH, &reported);
  else
    glGetProgramiv(object, GL_INFO_LOG_LENGTH, &reported);
  if (reported <= 1)
    return {};
  std::string result(size_t(std::min(reported, 2048)), 0);
  GLsizei copied = 0;
  if (shader)
    glGetShaderInfoLog(object, GLsizei(result.size()), &copied, result.data());
  else
    glGetProgramInfoLog(object, GLsizei(result.size()), &copied, result.data());
  result.resize(size_t(std::clamp(copied, 0, GLsizei(result.size() - 1))));
  return result;
}
inline void Receipt(const Pending &p, const char *stage,
                    std::shared_ptr<Artifact> artifact = {}) {
  std::ostringstream out;
  out << "{\"revision\":\"T475_PROGRAM_ARTIFACT_V1\",\"role\":" << Quote(p.role)
      << ",\"key\":" << Quote(p.key) << ",\"shaderSHA\":" << Quote(p.shader)
      << ",\"layoutSHA\":" << Quote(p.layout)
      << ",\"linkConfigSHA\":" << Quote(Hash(kLinkConfig))
      << ",\"environmentSHA\":" << Quote(p.environment)
      << ",\"packageSource\":" << Quote(packageSource)
      << ",\"route\":" << Quote(p.route) << ",\"stage\":" << Quote(stage)
      << ",\"cacheRawGL\":" << p.cacheError
      << ",\"originalRawGL\":" << p.originalError << ",\"log\":" << Quote(p.log)
      << ",\"logReported\":" << p.reported << ",\"logCopied\":" << p.log.size()
      << ",\"logTruncated\":"
      << (p.reported > int(p.log.size() + 1) ? "true" : "false")
      << ",\"shaderLog\":" << Quote(p.shaderLog)
      << ",\"shaderLogReported\":" << p.shaderReported
      << ",\"shaderLogCopied\":" << p.shaderLog.size()
      << ",\"shaderLogTruncated\":"
      << (p.shaderReported > int(p.shaderLog.size() + 1) ? "true" : "false")
      << ",\"beginNs\":" << p.begin << ",\"buildEndNs\":" << p.compiled
      << ",\"endNs\":" << Stamp()
      << ",\"binaryBytes\":" << (artifact ? artifact->bytes.size() : 0)
      << ",\"binaryFormat\":" << (artifact ? artifact->format : 0)
      << ",\"cacheLog\":" << Quote(p.cacheLog)
      << ",\"logBytesHex\":" << Quote(Hex(p.log.data(), p.log.size()))
      << ",\"shaderLogBytesHex\":"
      << Quote(Hex(p.shaderLog.data(), p.shaderLog.size()))
      << ",\"cacheLogBytesHex\":"
      << Quote(Hex(p.cacheLog.data(), p.cacheLog.size()))
      << ",\"cacheRawEGL\":" << p.cacheEgl
      << ",\"compileBeginNs\":" << p.compileBegin
      << ",\"compileEndNs\":" << p.compileEnd
      << ",\"linkBeginNs\":" << p.linkBegin << ",\"linkEndNs\":" << p.linkEnd
      << ",\"loadBeginNs\":" << p.loadBegin << ",\"loadEndNs\":" << p.loadEnd
      << ",\"reflectBeginNs\":" << p.reflectBegin
      << ",\"reflectEndNs\":" << p.reflectEnd
      << ",\"prepareBeginNs\":" << p.prepareBegin
      << ",\"prepareEndNs\":" << p.prepareEnd
      << ",\"warmupBeginNs\":" << p.warmupBegin
      << ",\"warmupEndNs\":" << p.warmupEnd << ",\"supportedFormats\":[";
  for (size_t i = 0; i < p.formats.size(); ++i) {
    if (i)
      out << ',';
    out << p.formats[i];
  }
  out << "]}";
  Store().Receipt(p.key, out.str(), std::move(artifact));
}
struct RetrievableApi : create468::GlApi {
  Pending &record;
  explicit RetrievableApi(Pending &p) : record(p) {}
  void Compile(GLuint s) {
    record.compileBegin = Stamp();
    Receipt(record, "COMPILE_ENTER");
    create468::GlApi::Compile(s);
    record.compileEnd = Stamp();
  }
  void Link(GLuint p) {
    record.linkBegin = Stamp();
    Receipt(record, "LINK_ENTER");
    if (!record.formats.empty()) {
      glProgramParameteri(p, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
      record.cacheError = glGetError();
    }
    create468::GlApi::Link(p);
    record.linkEnd = Stamp();
  }
  void DeleteShader(GLuint shader) {
    record.shaderLog = Log(shader, record.shaderReported, true);
    const auto error = glGetError();
    if (error)
      record.cacheError = error;
    create468::GlApi::DeleteShader(shader);
  }
};
inline bool Build(uint32_t object, const char *source,
                  const char *suppliedShaderSha, const char *layout,
                  GLuint &program, GLenum &originalError,
                  SubmitObserver observer = {}) {
  if (create468::active) {
    create468::active->object = object;
    create468::active->store.Change(
        create468::active->serial, [&](create468::Attempt &attempt) {
          auto &record = attempt.programs[object];
          std::strncpy(record.sha.data(), suppliedShaderSha, 64);
          record.bytes = std::strlen(source);
        });
  }
  Pending p;
  p.begin = Stamp();
  p.context = eglGetCurrentContext();
  p.owner = std::this_thread::get_id();
  p.role = object == 13  ? "G13_COMPUTE"
           : object == 0 ? "SOURCE"
           : object == 1 ? "INTERMEDIATE_QUANTUM"
                         : "FINAL_QUANTUM";
  p.shader = Hash(source);
  p.layout = Hash(layout);
  p.route = "SOURCE_COMPILE";
  // Never consume a prior product error as a harmless cache miss.
  p.key = Key(source, layout, "ENVIRONMENT_NOT_QUERIED");
  const auto prior = glGetError();
  if (prior) {
    if (!originalError)
      originalError = prior;
    p.originalError = prior;
    Receipt(p, "PRIOR_PRODUCT_GL_ERROR");
    return false;
  }
  if (p.context == EGL_NO_CONTEXT) {
    Receipt(p, "NO_CURRENT_CONTEXT");
    return false;
  }
  p.environment = Environment();
  p.cacheEgl = eglGetError();
  GLint count = 0;
  glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &count);
  if (count > 0 && count <= 256) {
    p.formats.resize(count);
    glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, p.formats.data());
  }
  p.cacheError = glGetError();
  if (p.cacheError || p.cacheEgl != EGL_SUCCESS || p.environment.empty())
    p.formats.clear();
  p.key = Key(source, layout, p.environment);
  p.cacheLog = Store().Rejection(p.key);
  if (!p.cacheLog.empty()) {
    p.route = "REJECT_THEN_SOURCE_ONCE";
    Receipt(p, "DISK_REJECTED");
  }
  if (!p.formats.empty() && !p.environment.empty())
    if (auto a = Store().Get(p.key)) {
      if (std::find(p.formats.begin(), p.formats.end(), GLint(a->format)) !=
          p.formats.end()) {
        p.loadBegin = Stamp();
        Receipt(p, "BINARY_LOAD_ENTER", a);
        program = glCreateProgram();
        glProgramBinary(program, a->format, a->bytes.data(),
                        GLsizei(a->bytes.size()));
        GLint good = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &good);
        p.cacheError = glGetError();
        p.log = Log(program, p.reported);
        const auto logError = glGetError();
        if (!p.cacheError)
          p.cacheError = logError;
        p.loadEnd = Stamp();
        p.cacheLog = p.log;
        Receipt(p, "LOAD_CHECKED", a);
        if (good && !p.cacheError) {
          p.route = "BINARY_LOAD";
          p.compiled = Stamp();
          if (pending.size() >= 32)
            pending.erase(pending.begin());
          pending[program] = p;
          Receipt(p, "LOAD_LINK_OK_REFLECTION_PENDING", a);
          return true;
        }
        glDeleteProgram(program);
        program = 0;
      } else {
        p.cacheLog = "UNKNOWN_BINARY_FORMAT";
        Receipt(p, "LOAD_FORMAT_REJECTED", a);
      }
      Store().Reject(p.key);
      p.route = "REJECT_THEN_SOURCE_ONCE";
    }
  RetrievableApi api(p);
  const bool ok = create468::Build(api, object, source, suppliedShaderSha,
                                   program, originalError, observer);
  p.compiled = Stamp();
  const auto postBuild = glGetError();
  if (postBuild && !originalError)
    originalError = postBuild;
  p.originalError = originalError;
  if (program) {
    p.log = Log(program, p.reported);
    const auto logError = glGetError();
    if (logError)
      p.cacheError = logError;
  }
  if (!ok || originalError) {
    Receipt(p, "COMPILE_OR_LINK_FAILED");
    return false;
  }
  if (p.formats.empty())
    p.route = "BINARY_UNAVAILABLE_SOURCE";
  if (pending.size() >= 32)
    pending.erase(pending.begin());
  pending[program] = p;
  Receipt(p, "LINK_OK_REFLECTION_PENDING");
  return true;
}
template <class Uniforms>
inline bool Reflect(Uniforms &uniforms, GLuint program) {
  auto at = pending.find(program);
  if (at != pending.end())
    at->second.reflectBegin = Stamp();
  const bool ok = uniforms.Prepare(program);
  at = pending.find(program);
  if (at != pending.end()) {
    at->second.reflectEnd = Stamp();
    Receipt(at->second, ok ? "REFLECT_OK" : "REFLECT_FAILED");
  }
  return ok;
}
template <class Prepare>
inline GLenum ObservePrepare(GLuint program, Prepare action) {
  auto at = pending.find(program);
  if (at != pending.end())
    at->second.prepareBegin = Stamp();
  const auto error = action();
  at = pending.find(program);
  if (at != pending.end()) {
    at->second.prepareEnd = Stamp();
    if (error)
      at->second.originalError = error;
    Receipt(at->second, error ? "PREPARE_FAILED" : "PREPARE_OK");
  }
  return error;
}
inline void Warmup(GLuint program, uint64_t begin, uint64_t end, GLenum error) {
  auto at = qualified.find(program);
  if (at == qualified.end())
    return;
  auto p = at->second;
  p.warmupBegin = begin;
  p.warmupEnd = end;
  p.originalError = error;
  Receipt(p, error ? "WARMUP_FAILED" : "WARMUP_OK", Store().Get(p.key));
}
inline void Forget(GLuint program) {
  qualified.erase(program);
  auto at = pending.find(program);
  if (at == pending.end())
    return;
  Receipt(at->second, "RESOURCE_REFLECT_PREPARE_ABORTED");
  pending.erase(at);
}
// Called only AFTER the existing uniform reflection and original PrepareProgram
// succeeded.
inline GLenum Prepared(GLuint program) {
  auto at = pending.find(program);
  if (at == pending.end())
    return GL_NO_ERROR;
  Pending p = std::move(at->second);
  pending.erase(at);
  if (p.context != eglGetCurrentContext() ||
      p.owner != std::this_thread::get_id()) {
    Receipt(p, "STALE_CONTEXT_REJECTED");
    return GL_NO_ERROR;
  }
  const auto prior = glGetError();
  if (prior) {
    p.originalError = prior;
    Receipt(p, "REFLECT_PREPARE_GL_FAILED");
    return prior;
  }
  if (qualified.size() >= 32)
    qualified.erase(qualified.begin());
  qualified[program] = p;
  if (p.route == "BINARY_LOAD") {
    Receipt(p, "REFLECT_PREPARE_OK", Store().Get(p.key));
    return GL_NO_ERROR;
  }
  if (p.formats.empty()) {
    Store().DisableExport(p.key);
    Receipt(p, "BINARY_UNAVAILABLE");
    return GL_NO_ERROR;
  }
  if (Store().ExportUnavailable(p.key)) {
    Receipt(p, "EXPORT_UNAVAILABLE_THIS_STARTUP");
    return GL_NO_ERROR;
  }
  try {
    // At this point the product has checked its own GL error. Export faults are
    // cache-only.
    Receipt(p, "EXPORT_LENGTH_ENTER");
    GLint n = 0;
    glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &n);
    p.cacheError = glGetError();
    if (p.cacheError || n <= 0 || size_t(n) > kEntryLimit) {
      Store().DisableExport(p.key);
      Receipt(p, "EXPORT_LENGTH_REJECTED");
      return GL_NO_ERROR;
    }
    std::vector<uint8_t> bytes(size_t(n), uint8_t(0));
    GLsizei copied = 0;
    GLenum format = 0;
    Receipt(p, "EXPORT_ENTER");
    glGetProgramBinary(program, n, &copied, &format, bytes.data());
    p.cacheError = glGetError();
    if (p.cacheError || copied <= 0 || copied > n ||
        std::find(p.formats.begin(), p.formats.end(), GLint(format)) ==
            p.formats.end()) {
      Store().DisableExport(p.key);
      Receipt(p, "EXPORT_FAILED");
      return GL_NO_ERROR;
    }
    bytes.resize(size_t(copied));
    auto artifact = Store().Put(p.key, format, std::move(bytes));
    Receipt(p, artifact ? "EXPORT_WRITE_SUBMITTED" : "CACHE_NOT_STORED",
            artifact);
    return GL_NO_ERROR;
  } catch (...) {
    Store().DisableExport(p.key);
    Receipt(p, "CACHE_EXPORT_EXCEPTION");
    return GL_NO_ERROR;
  }
}
} // namespace grs::programcache
