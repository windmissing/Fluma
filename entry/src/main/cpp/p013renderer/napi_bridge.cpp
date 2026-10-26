// P-013 P0 v1. The mutex protects the borrowed window through unbind.
#include "napi_bridge.h"

#include <napi/native_api.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <thread>

#include "egl_context.h"
#include "composition_probe.h"
#include "dualfield_pass.h"
#include "material_uploader.h"
#include "material_pass.h"
#include <vector>
#include "p013_shader_sources.h"
#include "shader_pass.h"
#include "tdata_uploader.h"
namespace p013 {
namespace {
struct FrameStorage {
  std::array<float, kFloats> staging{};
  std::array<Telemetry, kTelemetryCapacity> ring{};
};
struct Session {
  std::mutex mutex;
  EglContext egl;
  ShaderPass shader;
  CompositionProbe probe;
  DualFieldPass dual;
  std::vector<float> dualXY;
  std::vector<uint32_t> dualMeta;
  std::string dualRun;
  double dualStateEpoch=0, dualCommit=0;
  bool dualBasic=false;
  MaterialPass materialPass;
  std::array<float,1536> materialData{};
  MaterialGlGpu materialGpu;
  MaterialUploader material{materialGpu};
  TDataUploader uploader;
  std::unique_ptr<FrameStorage> storage;
  size_t head = 0, count = 0;
  uint64_t dropped = 0, serial = 0, id = 0, attempt = 0, lastSwap = 0;
  std::thread::id owner{};
  bool ownerBound = false, released = false;
  Status lastError = Status::OK;
  std::string componentId;
  OH_NativeXComponent* component = nullptr;
} g;
void Put(napi_env e, napi_value o, const char* k, double v) {
  if (!o) return;
  napi_value x = nullptr;
  napi_create_double(e, v, &x);
  napi_set_named_property(e, o, k, x);
}
void Put(napi_env e, napi_value o, const char* k, bool v) {
  if (!o) return;
  napi_value x = nullptr;
  napi_get_boolean(e, v, &x);
  napi_set_named_property(e, o, k, x);
}
void Put(napi_env e, napi_value o, const char* k, const char* v) {
  if (!o) return;
  napi_value x = nullptr;
  napi_create_string_utf8(e, v, NAPI_AUTO_LENGTH, &x);
  napi_set_named_property(e, o, k, x);
}
napi_value Object(napi_env e) {
  napi_value o = nullptr;
  if (napi_create_object(e, &o) != napi_ok || !o) {
    napi_throw_error(e, "P013_NAPI_ALLOCATION",
                     "Cannot allocate renderer result");
    return nullptr;
  }
  return o;
}
bool Number(napi_env e, napi_value o, const char* k, double& out) {
  napi_value v = nullptr;
  napi_valuetype t;
  return napi_get_named_property(e, o, k, &v) == napi_ok &&
         napi_typeof(e, v, &t) == napi_ok && t == napi_number &&
         napi_get_value_double(e, v, &out) == napi_ok;
}
bool Boolean(napi_env e, napi_value o, const char* k, bool& out) {
  napi_value v = nullptr;
  napi_valuetype t;
  return napi_get_named_property(e, o, k, &v) == napi_ok &&
         napi_typeof(e, v, &t) == napi_ok && t == napi_boolean &&
         napi_get_value_bool(e, v, &out) == napi_ok;
}
bool String(napi_env e, napi_value v, std::string& out) {
  napi_valuetype t;
  if (napi_typeof(e, v, &t) != napi_ok || t != napi_string) return false;
  char b[256]{};
  size_t n = 0;
  if (napi_get_value_string_utf8(e, v, b, sizeof(b), &n) != napi_ok ||
      n >= sizeof(b) - 1)
    return false;
  out.assign(b, n);
  return true;
}
Status Owner() {
  return g.ownerBound && g.owner != std::this_thread::get_id()
             ? Status::WRONG_THREAD
             : Status::OK;
}
napi_value Control(napi_env e, Status code) {
  napi_value o = Object(e);
  Put(e, o, "code", static_cast<double>(code));
  Put(e, o, "sessionId", static_cast<double>(g.id));
  Put(e, o, "state", StateName(g.egl.state));
  Put(e, o, "surfaceEpoch", static_cast<double>(g.egl.surfaceEpoch));
  Put(e, o, "contextEpoch", static_cast<double>(g.egl.contextEpoch));
  return o;
}
void ResetGpu() {
  g.materialPass.Reset(false);
  g.material.Reset(false);
  g.dual.Reset(false);
  g.probe.Reset();
  g.shader.Reset(false);
  g.uploader.Reset(false);
}
void Destroy(ResetReason reason) {
  // Normal owner teardown deletes names while current. Callback/error teardown
  // abandons registered names only when their owning context is destroyed.
  if (g.egl.state == State::READY && g.egl.active && !g.egl.current &&
      (g.shader.Program() || g.uploader.Texture() || g.probe.Program() || g.dual.bed || g.materialPass.Program())) {
    SurfaceInfo info{};
    if (g.egl.EnsureCurrent({g.egl.surfaceEpoch, g.egl.width, g.egl.height},
                            info) == Status::OK) {
      g.materialPass.Reset(true);
      g.material.Reset(true);
      g.dual.Reset(true);
      g.probe.Reset(true);
      g.shader.Reset(true);
      g.uploader.Reset(true);
      g.egl.Unbind();
    }
  }
  g.egl.DestroyUnbound(reason);
  if (!g.egl.current) ResetGpu();
}
void Created(OH_NativeXComponent* c, void* w) {
  std::lock_guard<std::mutex> lock(g.mutex);
  if (c != g.component || g.released) return;
  const auto epoch = g.egl.surfaceEpoch;
  g.egl.OnWindowCreated(c, w);
  if (epoch != g.egl.surfaceEpoch) ResetGpu();
}
void Changed(OH_NativeXComponent* c, void* w) {
  std::lock_guard<std::mutex> lock(g.mutex);
  if (c == g.component && !g.released) g.egl.OnWindowChanged(c, w);
}
void Destroyed(OH_NativeXComponent* c, void* w) {
  std::lock_guard<std::mutex> lock(g.mutex);
  if (c != g.component) return;
  const auto epoch = g.egl.surfaceEpoch;
  g.egl.OnWindowDestroyed(c, w);
  if (epoch != g.egl.surfaceEpoch) ResetGpu();
}
OH_NativeXComponent_Callback callbacks{Created, Changed, Destroyed, nullptr};
void Hook(napi_env env, napi_value exports) {
  bool has = false;
  napi_has_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &has);
  if (!has) return;
  napi_value v = nullptr;
  OH_NativeXComponent* c = nullptr;
  if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &v) !=
          napi_ok ||
      napi_unwrap(env, v, reinterpret_cast<void**>(&c)) != napi_ok || !c)
    return;
  char id[OH_XCOMPONENT_ID_LEN_MAX + 1]{};
  uint64_t n = sizeof(id);
  if (OH_NativeXComponent_GetXComponentId(c, id, &n) != 0) return;
  {
    std::lock_guard<std::mutex> lock(g.mutex);
    if (g.ownerBound && g.componentId != id) return;
    g.component = c;
    g.componentId = id;
    g.released = false;
  }
  OH_NativeXComponent_RegisterCallback(c, &callbacks);
}
void Cleanup(void*) {
  std::lock_guard<std::mutex> lock(g.mutex);
  if (Owner() == Status::OK) {
    Destroy(ResetReason::RELEASE);
    g.egl.Release();
    g.material.ClearSession();
    g.ownerBound = false;
    g.released = true;
    g.count = 0;
    g.storage.reset();
  }
}
napi_value InitSession(napi_env e, napi_callback_info info) {
  size_t n = 1;
  napi_value a[1]{};
  napi_get_cb_info(e, info, &n, a, nullptr, nullptr);
  std::string id;
  bool valid = n == 1 && String(e, a[0], id) && !id.empty();
  std::lock_guard<std::mutex> lock(g.mutex);
  if (!valid) return Control(e, Status::INVALID_ARGUMENT);
  if (Owner() != Status::OK) return Control(e, Status::WRONG_THREAD);
  if (g.ownerBound && id != g.componentId)
    return Control(e, Status::SESSION_BUSY);
  if (id != g.componentId || g.egl.state == State::WAIT_WINDOW)
    return Control(e, Status::NOT_READY);
  if (!g.ownerBound) {
    if (g.serial >= kSafeInteger) return Control(e, Status::SESSION_BUSY);
    g.storage.reset(new (std::nothrow) FrameStorage());
    if (!g.storage) return Control(e, Status::GL_RESOURCE_FAILED);
    g.count = g.head = 0;
    g.id = ++g.serial;
    g.attempt = g.lastSwap = 0;
    g.owner = std::this_thread::get_id();
    g.ownerBound = true;
    g.released = false;
    g.egl.SetActive(true);
  }
  return Control(e, Status::OK);
}
napi_value GetState(napi_env e, napi_callback_info) {
  std::lock_guard<std::mutex> lock(g.mutex);
  Status code = Owner();
  if (code == Status::OK) g.egl.QuerySize();
  napi_value o = Control(e, code);
  Put(e, o, "widthPx", static_cast<double>(g.egl.width));
  Put(e, o, "heightPx", static_cast<double>(g.egl.height));
  Put(e, o, "sizeSource",
      g.egl.width > 0 ? "EGL_QUERY_SURFACE" : "UNAVAILABLE");
  Put(e, o, "active", g.egl.active);
  Put(e, o, "ownerBound", g.ownerBound);
  Put(e, o, "lastErrorCode", static_cast<double>(g.lastError));
  Put(e, o, "nativeOwnedBytes",
      static_cast<double>(g.storage ? sizeof(FrameStorage) : 0));
  Put(e, o, "textureLiveCount", g.uploader.Texture() ? 1.0 : 0.0);
  Put(e, o, "programLiveCount", (g.shader.Program() ? 1.0 : 0.0) + (g.probe.Program() ? 1.0 : 0.0) + (g.dual.bed ? 1.0 : 0.0) + (g.dual.particles ? 1.0 : 0.0));
  Put(e, o, "vaoLiveCount", (g.shader.Vao() ? 1.0 : 0.0) + (g.probe.Vao() ? 1.0 : 0.0) + (g.dual.vao ? 1.0 : 0.0));
  Put(e,o,"particleBufferLiveCount",(g.dual.xyBuffer ? 1.0 : 0.0)+(g.dual.metaBuffer ? 1.0 : 0.0));
  Put(e,o,"particleStagingBytes",static_cast<double>((g.dualXY.capacity()+g.dualMeta.capacity())*4));
  Put(e, o, "materialProgramLiveCount",g.materialPass.Program()?1.0:0.0);
  Put(e, o, "materialTdataTextureLiveCount",g.materialPass.DataTexture()?1.0:0.0);
  const auto materialState=g.material.Inspect();
  Put(e,o,"materialTextureLiveCount",double(materialState.textureLiveCount));Put(e,o,"materialAssetCpuBytes",double(materialState.cpuBytes));
  Put(e, o, "telemetryLiveCount", static_cast<double>(g.count));
  Put(e, o, "textureAbandonCount",
      static_cast<double>(g.uploader.textureAbandonCount));
  Put(e, o, "programAbandonCount",
      static_cast<double>(g.shader.programAbandonCount + g.probe.programAbandonCount));
  Put(e, o, "vaoAbandonCount", static_cast<double>(g.shader.vaoAbandonCount + g.probe.vaoAbandonCount));
  return o;
}
napi_value SetActive(napi_env e, napi_callback_info info) {
  size_t n = 1;
  napi_value a[1]{};
  napi_get_cb_info(e, info, &n, a, nullptr, nullptr);
  bool active = false;
  napi_valuetype t;
  bool valid = n == 1 && napi_typeof(e, a[0], &t) == napi_ok &&
               t == napi_boolean &&
               napi_get_value_bool(e, a[0], &active) == napi_ok;
  std::lock_guard<std::mutex> lock(g.mutex);
  Status code = Owner();
  if (code == Status::OK) {
    if (!valid)
      code = Status::INVALID_ARGUMENT;
    else if (g.released)
      code = Status::RELEASED;
    else {
      if (!active) Destroy(ResetReason::BACKGROUND);
      code = g.egl.SetActive(active);
      if (!active && !g.egl.current) ResetGpu();
    }
  }
  return Control(e, code);
}
napi_value Release(napi_env e, napi_callback_info) {
  std::lock_guard<std::mutex> lock(g.mutex);
  Status code = Owner();
  if (code != Status::OK) return Control(e, code);
  Destroy(ResetReason::RELEASE);
  g.egl.Release();
  if (g.egl.current) return Control(e, Status::EGL_INIT_FAILED);
  g.ownerBound = false;
  g.material.ClearSession();
  g.released = true;
  g.count = 0;
  g.head = 0;
  g.storage.reset();
  g.dualBasic=false;g.dualRun.clear();g.dualStateEpoch=g.dualCommit=0;
  std::vector<float>().swap(g.dualXY);std::vector<uint32_t>().swap(g.dualMeta);
  return Control(e, Status::OK);
}
void Save(const Telemetry& t) {
  if (!g.storage) {
    g.dropped++;
    return;
  }
  if (g.count == kTelemetryCapacity) {
    g.head = (g.head + 1) % kTelemetryCapacity;
    g.count--;
    g.dropped++;
  }
  g.storage->ring[(g.head + g.count) % kTelemetryCapacity] = t;
  g.count++;
}
napi_value Result(napi_env e, const Telemetry& t) {
  napi_value o = Control(e, t.code);
  Put(e, o, "sessionId", t.frame.sessionId);
  Put(e, o, "surfaceEpoch", t.frame.surfaceEpoch);
  Put(e, o, "contextEpoch", t.contextEpoch);
  Put(e, o, "state", StateName(t.state));
  Put(e, o, "frameId", t.frame.frameId);
  Put(e, o, "snapshotId", t.frame.snapshotId);
  Put(e, o, "uploadedFrameId", static_cast<double>(t.uploadedFrameId));
  Put(e, o, "submittedFrameId", static_cast<double>(t.submittedFrameId));
  Put(e, o, "lastSwapFrameId", static_cast<double>(t.lastSwapFrameId));
  Put(e, o, "swapSucceeded", t.swapSucceeded);
  Put(e, o, "eglError", static_cast<double>(t.eglError));
  Put(e, o, "glError", static_cast<double>(t.glError));
  Put(e, o, "recoveryRequired", t.recoveryRequired);
  Put(e, o, "nativeTypedByteLength", static_cast<double>(t.nativeTypedByteLength));
  Put(e, o, "nativeTypedByteOffset", static_cast<double>(t.nativeTypedByteOffset));
  Put(e, o, "nativeBufferByteLength", static_cast<double>(t.nativeBufferByteLength));
  Put(e, o, "errorStage", t.errorStage);
  return o;
}
bool ParseFrame(napi_env e, napi_value o, FrameInput& f, const char*& stage) {
  napi_valuetype type;
  if (napi_typeof(e, o, &type) != napi_ok || type != napi_object) {
    stage = "FRAME_OBJECT";
    return false;
  }
  bool ok = true;
#define READ(x) do { if (!Number(e, o, #x, f.x)) { if (ok) stage = "FRAME_FIELD_" #x; ok = false; } } while (false)
  READ(sessionId);
  READ(frameId);
  READ(snapshotId);
  READ(surfaceEpoch);
  READ(fieldEpoch);
  READ(geometryEpoch);
  READ(continuityEpoch);
  READ(appearanceConfigRevision);
  READ(lightConfigRevision);
  READ(frameTimeNs);
  READ(surfaceWidthPx);
  READ(surfaceHeightPx);
  READ(boxXPx);
  READ(boxYPx);
  READ(boxWidthPx);
  READ(boxHeightPx);
  READ(capacity);
  READ(T_dataLayoutVersion);
  READ(zOriginVersion);
#undef READ
  if (!Boolean(e, o, "visualInverted", f.visualInverted)) {
    if (ok) stage = "FRAME_FIELD_visualInverted";
    ok = false;
  }
  if (!Boolean(e, o, "diagnosticEnabled", f.diagnosticEnabled)) {
    if (ok) stage = "FRAME_FIELD_diagnosticEnabled";
    ok = false;
  }
  return ok;
}
bool ParseLight(napi_env e, napi_value o, LightParams& l, const char*& stage) {
  napi_valuetype type;
  if (napi_typeof(e, o, &type) != napi_ok || type != napi_object) {
    stage = "LIGHT_OBJECT";
    return false;
  }
  bool ok = true;
#define READ(x) do { if (!Number(e, o, #x, l.x)) { if (ok) stage = "LIGHT_FIELD_" #x; ok = false; } } while (false)
  READ(la);
  READ(ld);
  READ(lmin);
  READ(lmax);
  READ(lref);
  READ(ellLightMaterial);
  READ(lightX);
  READ(lightY);
  READ(lightZ);
  READ(sigmaCandidate);
#undef READ
  return ok;
}
bool TypedView(napi_env e,napi_value v,napi_typedarray_type expected,size_t expectedCount,void*& data,const char*& stage){
 bool typed=false,detached=false;size_t count=0,offset=0,bytes=0;void* base=nullptr;napi_value buffer=nullptr;napi_typedarray_type type{};
 if(napi_is_typedarray(e,v,&typed)!=napi_ok||!typed){stage="DUAL_NOT_TYPEDARRAY";return false;}
 if(napi_get_typedarray_info(e,v,&type,&count,&data,&buffer,&offset)!=napi_ok){stage="DUAL_TYPED_INFO";return false;}
 if(type!=expected){stage="DUAL_TYPED_TYPE";return false;}
 if(expectedCount>SIZE_MAX/4 || count!=expectedCount*4){stage="DUAL_TYPED_LENGTH";return false;}
 if(!data||napi_is_detached_arraybuffer(e,buffer,&detached)!=napi_ok||detached){stage="DUAL_BUFFER_DETACHED";return false;}
 if(napi_get_arraybuffer_info(e,buffer,&base,&bytes)!=napi_ok||!base||offset>bytes||expectedCount*4>bytes-offset||offset%4!=0||data!=static_cast<unsigned char*>(base)+offset){stage="DUAL_BUFFER_RANGE";return false;}
 return true;
}
bool ExactString(napi_env e,napi_value o,const char* name,const char* expected){napi_value v=nullptr;std::string text;return napi_get_named_property(e,o,name,&v)==napi_ok&&String(e,v,text)&&text==expected;}
// T54 G2: separate bridge surface, never changes P0 drawFrame semantics.
napi_value MaterialResult(napi_env e,const MaterialReceipt& r){
 auto o=Object(e);Put(e,o,"code",double(r.code));Put(e,o,"assetGeneration",double(r.assetGeneration));
 Put(e,o,"state",r.state.c_str());Put(e,o,"receivedBytes",double(r.receivedBytes));
 Put(e,o,"uploadedBytes",double(r.uploadedBytes));Put(e,o,"totalBytes",double(r.totalBytes));
 Put(e,o,"pendingGeneration",double(r.pendingGeneration));Put(e,o,"activeGeneration",double(r.activeGeneration));Put(e,o,"appearanceConfigRevision",double(r.appearanceConfigRevision));
 Put(e,o,"contextEpoch",double(r.contextEpoch));Put(e,o,"errorStage",r.errorStage.c_str());
 Put(e,o,"materialUploadBytes",double(r.uploadedBytes));Put(e,o,"materialPendingBytes",double(r.pendingBytes));
 Put(e,o,"materialPumpCount",double(r.pumpCount));Put(e,o,"materialTextureLiveCount",double(r.textureLiveCount));
 Put(e,o,"assetCpuBytes",double(r.cpuBytes));Put(e,o,"discardedGeneration",double(r.discardedGeneration));
 Put(e,o,"requestedAppearanceConfigRevision",double(r.requestedRevision));
 Put(e,o,"boundAppearanceConfigRevision",double(r.boundRevision));Put(e,o,"bindingId",double(r.bindingId));
 Put(e,o,"configurationDigest",r.configurationDigest.c_str());Put(e,o,"frameToken",r.frameToken.c_str());return o;
}
bool MaterialText(napi_env e,napi_value o,const char* key,std::string& value){
 napi_value v=nullptr;return napi_get_named_property(e,o,key,&v)==napi_ok&&String(e,v,value)&&!value.empty();
}
int ParseMaterial(napi_env e,napi_value v,MaterialDesc& d,double& session,const char*& stage){
 stage="version";double layout=0,z=0,api=0;
 if(!ExactString(e,v,"materialRecipeVersion","P1_ALBEDO_DELTA_V1")||
 !ExactString(e,v,"slopeKernelVersion","E2_GAUSS_COL6_R18_P1_V2")||
 !ExactString(e,v,"colorPipelineVersion","LIN_SRGB_REINHARD_V1")||
 !ExactString(e,v,"textureSamplingVersion","P1_TEXEL_CANVAS_PX_V1")||
 !ExactString(e,v,"strataSamplingVersion","CONTINUOUS_COLUMN_WARP_V1")||
 !Number(e,v,"T_dataLayoutVersion",layout)||layout!=2||!Number(e,v,"zOriginVersion",z)||z!=1||
 !Number(e,v,"apiVersion",api)||api!=2)return 400;
 stage="metadata";
 double gen=0,rev=0,surface=0,context=0;
 if(!Number(e,v,"sessionId",session)||!SafeInteger(session)||session<1||
 !Number(e,v,"surfaceEpoch",surface)||!SafeInteger(surface)||surface<0||
 !Number(e,v,"contextEpoch",context)||!SafeInteger(context)||context<1||
 !Number(e,v,"assetGeneration",gen)||!SafeInteger(gen)||gen<1||
 !Number(e,v,"appearanceConfigRevision",rev)||!SafeInteger(rev)||rev<0||
 !Number(e,v,"geometryEpoch",d.geometryEpoch)||!SafeInteger(d.geometryEpoch)||d.geometryEpoch<0||
 !Number(e,v,"renderSeed",d.seed)||!SafeInteger(d.seed)||
 !MaterialText(e,v,"materialBundleId",d.bundleId)||!MaterialText(e,v,"paletteHash",d.paletteHash)||
 !MaterialText(e,v,"grainHash",d.grainHash)||!MaterialText(e,v,"strataHash",d.strataHash)||
 !MaterialText(e,v,"assetGeneratorVersion",d.generatorVersion))return 400;
 d.generation=gen;d.revision=rev;d.surfaceEpoch=surface;d.contextEpoch=context;
 const char* sizes[]={"grainWidth","grainHeight","strataWidth","strataHeight","layers"};
 const double expected[]={256,256,64,256,3};
 for(int i=0;i<5;i++){double value=0;if(!Number(e,v,sizes[i],value)||value!=expected[i])return 400;}
 if(!ExactString(e,v,"inputEncoding","SRGB8_STRAIGHT_OPAQUE"))return 400;
 if(!Number(e,v,"effectiveLambdaMaterial",d.lambdaMaterial)||!std::isfinite(d.lambdaMaterial)||d.lambdaMaterial<=0||
 !Number(e,v,"effectiveLambdaCanvasPx",d.lambdaCanvas)||!std::isfinite(d.lambdaCanvas)||d.lambdaCanvas<=0||
 !Number(e,v,"warpRangeCycles",d.warpRange)||!std::isfinite(d.warpRange)||d.warpRange<=0)return 400;
 napi_value base=nullptr;uint32_t rows=0;
 if(napi_get_named_property(e,v,"grainBaseSrgb",&base)!=napi_ok||napi_get_array_length(e,base,&rows)!=napi_ok||rows!=3)return 400;
 for(uint32_t k=0;k<3;k++){napi_value row=nullptr;uint32_t cols=0;if(napi_get_element(e,base,k,&row)!=napi_ok||napi_get_array_length(e,row,&cols)!=napi_ok||cols!=3)return 400;
 for(uint32_t c=0;c<3;c++){napi_value n=nullptr;double x=0;if(napi_get_element(e,row,c,&n)!=napi_ok||napi_get_value_double(e,n,&x)!=napi_ok||!std::isfinite(x)||x<0||x>1)return 400;d.grainBase[k*3+c]=x;}}
 napi_value parameters=nullptr;napi_valuetype parametersType;
 if(napi_get_named_property(e,v,"assetGenerationParameters",&parameters)!=napi_ok||napi_typeof(e,parameters,&parametersType)!=napi_ok)return 400;
 if(parametersType==napi_undefined)d.metadata="U";
 else {
  std::array<char,16385> text{};size_t length=0;
  if(parametersType!=napi_string||napi_get_value_string_utf8(e,parameters,text.data(),text.size(),&length)!=napi_ok||!length||length>=text.size()-1)return 400;
  d.metadata="S"+std::string(text.data(),length);
 }
 return 0;
}
Status MaterialOwner(){auto code=Owner();if(code==Status::OK&&(!g.ownerBound||g.released))code=Status::NOT_READY;return code;}
bool MaterialBytes(napi_env e,napi_value v,size_t expected,void*& data,const char*& stage){
 bool typed=false,detached=false;size_t count=0,offset=0,bytes=0;void* base=nullptr;napi_value buffer=nullptr;napi_typedarray_type type{};
 stage="byteOffset";
 if(napi_is_typedarray(e,v,&typed)!=napi_ok||!typed||napi_get_typedarray_info(e,v,&type,&count,&data,&buffer,&offset)!=napi_ok||type!=napi_uint8_array)return false;
 if(count!=expected){stage="byteLength";return false;}
 if(!data||napi_is_detached_arraybuffer(e,buffer,&detached)!=napi_ok||detached||
 napi_get_arraybuffer_info(e,buffer,&base,&bytes)!=napi_ok||!base||offset>bytes||expected>bytes-offset||data!=static_cast<uint8_t*>(base)+offset)return false;
 return true;
}
napi_value BindMaterialConfiguration(napi_env e,napi_callback_info info){
 size_t n=2;napi_value a[2]{};napi_get_cb_info(e,info,&n,a,nullptr,nullptr);double session=0,revision=0;std::string digest;
 bool valid=n==1&&Number(e,a[0],"sessionId",session)&&SafeInteger(session)&&session>0&&Number(e,a[0],"appearanceConfigRevision",revision)&&SafeInteger(revision)&&MaterialText(e,a[0],"configurationDigest",digest);
 std::lock_guard<std::mutex> lock(g.mutex);MaterialConfigReceipt r;
 auto owner=MaterialOwner();
 if(owner!=Status::OK){r.code=int(owner);r.errorStage="owner";}
 else if(!valid||session!=g.id){r.code=400;r.errorStage="configBinding";}
 else r=g.material.BindConfiguration(uint64_t(session),uint64_t(revision),digest);
 napi_value o=Object(e);Put(e,o,"code",double(r.code));Put(e,o,"sessionId",double(r.sessionId));Put(e,o,"bindingId",double(r.bindingId));Put(e,o,"appearanceConfigRevision",double(r.revision));Put(e,o,"configurationDigest",r.digest.c_str());Put(e,o,"errorStage",r.errorStage.c_str());return o;
}
napi_value StageMaterialBundle(napi_env e,napi_callback_info info){
 size_t n=3;napi_value a[3]{};napi_get_cb_info(e,info,&n,a,nullptr,nullptr);
 MaterialDesc d;double session=0;const char* stage="metadata";int code=n==3?ParseMaterial(e,a[0],d,session,stage):400;
 void* grain=nullptr;void* strata=nullptr;

 std::lock_guard<std::mutex> lock(g.mutex);auto owner=MaterialOwner();if(owner!=Status::OK){auto r=g.material.Inspect();r.code=int(owner);r.errorStage="owner";return MaterialResult(e,r);}
 if(!code&&(session!=g.id||d.surfaceEpoch!=g.egl.surfaceEpoch||d.contextEpoch!=g.egl.contextEpoch)){code=400;stage="contextEpoch";}
 if(!code){auto binding=g.material.CheckConfiguration(d);if(binding.code)return MaterialResult(e,binding);}
 if(!code){stage="byteOffset";if(!MaterialBytes(e,a[1],kMaterialGrainBytes,grain,stage)||!MaterialBytes(e,a[2],kMaterialStrataBytes,strata,stage))code=400;}
 if(code){auto r=g.material.Inspect();r.code=code;r.state="FAILED";r.errorStage=stage;r.assetGeneration=d.generation;return MaterialResult(e,r);}
 SurfaceInfo si{};auto current=g.egl.PrepareCurrent(g.egl.surfaceEpoch,si);
 if(current!=Status::OK){auto r=g.material.Inspect();r.code=int(current);r.errorStage=g.egl.errorStage;return MaterialResult(e,r);}
 if(d.contextEpoch!=g.egl.contextEpoch || d.surfaceEpoch!=g.egl.surfaceEpoch){g.egl.Unbind();auto r=g.material.Inspect();r.code=400;r.state="FAILED";r.errorStage="contextEpoch";return MaterialResult(e,r);}
 auto r=g.material.Stage(d,static_cast<uint8_t*>(grain),kMaterialGrainBytes,static_cast<uint8_t*>(strata),kMaterialStrataBytes);
 auto unbound=g.egl.Unbind();if(unbound!=Status::OK){r.code=int(unbound);r.errorStage="unbind";}return MaterialResult(e,r);
}
napi_value PumpMaterialUploads(napi_env e,napi_callback_info info){
 size_t n=2;napi_value a[2]{};napi_get_cb_info(e,info,&n,a,nullptr,nullptr);double budget=0;napi_valuetype type;
 if(n!=1||napi_typeof(e,a[0],&type)!=napi_ok||type!=napi_number||napi_get_value_double(e,a[0],&budget)!=napi_ok||!SafeInteger(budget)||budget<1||budget>65536){
  std::lock_guard<std::mutex> lock(g.mutex);return MaterialResult(e,g.material.InvalidBudget());}
 std::lock_guard<std::mutex> lock(g.mutex);auto owner=MaterialOwner();auto r=g.material.Inspect();
 if(owner!=Status::OK){r.code=int(owner);r.errorStage="owner";return MaterialResult(e,r);}
 SurfaceInfo si{};auto code=g.egl.PrepareCurrent(g.egl.surfaceEpoch,si);
 if(code==Status::OK){
  if(r.state=="ACTIVE" && !g.material.BindActive(g.egl.contextEpoch))r=g.material.Restore(g.egl.surfaceEpoch,g.egl.contextEpoch);
  r=g.material.Pump(size_t(budget),g.egl.surfaceEpoch,g.egl.contextEpoch);auto unbound=g.egl.Unbind();if(unbound!=Status::OK){r.code=int(unbound);r.errorStage="unbind";}}
 else{r.code=int(code);r.errorStage=g.egl.errorStage;}return MaterialResult(e,r);
}
napi_value CanActivateMaterial(napi_env e,napi_callback_info info){
 size_t n=2;napi_value a[2]{};napi_get_cb_info(e,info,&n,a,nullptr,nullptr);double gen=0,epoch=0;
 bool valid=n==2&&napi_get_value_double(e,a[0],&gen)==napi_ok&&napi_get_value_double(e,a[1],&epoch)==napi_ok&&SafeInteger(gen)&&SafeInteger(epoch)&&gen>0&&epoch>=0;
 std::lock_guard<std::mutex> lock(g.mutex);napi_value result=nullptr;
 napi_get_boolean(e,valid&&MaterialOwner()==Status::OK&&epoch==g.egl.contextEpoch&&g.material.TargetMatches(g.egl.surfaceEpoch,g.egl.contextEpoch)&&g.material.CanActivate(gen,epoch),&result);return result;
}
napi_value ActivateMaterial(napi_env e,napi_callback_info info){
 size_t n=2;napi_value a[2]{};napi_get_cb_info(e,info,&n,a,nullptr,nullptr);double gen=0;std::string token;
 bool valid=n==2&&napi_get_value_double(e,a[0],&gen)==napi_ok&&SafeInteger(gen)&&gen>0&&String(e,a[1],token)&&!token.empty();
 std::lock_guard<std::mutex> lock(g.mutex);auto r=g.material.Inspect();auto code=MaterialOwner();
 if(code!=Status::OK||!valid){r.code=code==Status::OK?400:int(code);r.errorStage="activation";return MaterialResult(e,r);}
 SurfaceInfo si{};code=g.egl.PrepareCurrent(g.egl.surfaceEpoch,si);
 if(code==Status::OK){if(!g.material.TargetMatches(g.egl.surfaceEpoch,g.egl.contextEpoch)){
    r=g.material.Discard(gen);r.code=401;r.errorStage="epoch";
  } else r=g.material.Activate(gen,g.egl.contextEpoch,token);auto u=g.egl.Unbind();if(u!=Status::OK){r.code=int(u);r.errorStage="unbind";}}
 else{r.code=int(code);r.errorStage=g.egl.errorStage;}return MaterialResult(e,r);
}
napi_value DiscardMaterialStage(napi_env e,napi_callback_info info){
 size_t n=1;napi_value a[1]{};napi_get_cb_info(e,info,&n,a,nullptr,nullptr);double gen=0;bool valid=n==1&&napi_get_value_double(e,a[0],&gen)==napi_ok&&SafeInteger(gen)&&gen>0;
 std::lock_guard<std::mutex> lock(g.mutex);auto r=g.material.Inspect();auto code=MaterialOwner();
 if(code!=Status::OK||!valid){r.code=code==Status::OK?400:int(code);r.errorStage="discard";return MaterialResult(e,r);}
 SurfaceInfo si{};code=g.egl.PrepareCurrent(g.egl.surfaceEpoch,si);if(code==Status::OK){r=g.material.Discard(gen);auto u=g.egl.Unbind();if(u!=Status::OK)r.code=int(u);}else r.code=int(code);return MaterialResult(e,r);
}
// T55 G3 complete material frame. Strictly isolated from P0 draw semantics.
template<size_t N> bool MaterialArray(napi_env e,napi_value o,const char* key,std::array<double,N>& out){
 napi_value a=nullptr;uint32_t length=0;if(napi_get_named_property(e,o,key,&a)!=napi_ok||napi_get_array_length(e,a,&length)!=napi_ok||length!=N)return false;
 for(uint32_t i=0;i<N;i++){napi_value v=nullptr;if(napi_get_element(e,a,i,&v)!=napi_ok||napi_get_value_double(e,v,&out[i])!=napi_ok||!std::isfinite(out[i]))return false;}return true;
}
bool ParseMaterialParams(napi_env e,napi_value o,MaterialParams& p){
 return MaterialArray(e,o,"paletteBaseSrgb",p.paletteBaseSrgb)&&MaterialArray(e,o,"grainBaseSrgb",p.grainBaseSrgb)&&
 MaterialArray(e,o,"grainOffsetCanvasPx",p.grainOffsetCanvasPx)&&MaterialArray(e,o,"bandMinYCanvasPx",p.bandMinYCanvasPx)&&MaterialArray(e,o,"bandMaxYCanvasPx",p.bandMaxYCanvasPx)&&MaterialArray(e,o,"canvasBox",p.canvasBox)&&
 Number(e,o,"canvasToSurfaceScaleX",p.canvasToSurfaceScaleX)&&Number(e,o,"canvasToSurfaceScaleY",p.canvasToSurfaceScaleY)&&
 Number(e,o,"aB",p.aB)&&Number(e,o,"aC",p.aC)&&Number(e,o,"aC3",p.aC3)&&Number(e,o,"warpRangeCycles",p.warpRangeCycles)&&Number(e,o,"effectiveLambdaMaterial",p.effectiveLambdaMaterial)&&Number(e,o,"effectiveLambdaCanvasPx",p.effectiveLambdaCanvasPx)&&
 Boolean(e,o,"grainEnabled",p.grainEnabled)&&Boolean(e,o,"strataEnabled",p.strataEnabled)&&Boolean(e,o,"sparkEnabled",p.sparkEnabled);
}
napi_value DrawMaterialFrame(napi_env e,napi_callback_info info){
 size_t n=4;napi_value a[4]{};napi_get_cb_info(e,info,&n,a,nullptr,nullptr);
 MaterialFrame m;MaterialParams material;LightParams light{};const char* stage="MATERIAL_ARGUMENT";void* ptr=nullptr;
 double api=0,generation=0,context=0;std::string token;
 bool valid=n==4&&ParseFrame(e,a[0],m.frame,stage);
 if(valid){stage="MATERIAL_VERSION";valid=Number(e,a[0],"apiVersion",api)&&api==2&&
 ExactString(e,a[0],"materialRecipeVersion","P1_ALBEDO_DELTA_V1")&&ExactString(e,a[0],"slopeKernelVersion","E2_GAUSS_COL6_R18_P1_V2")&&
 ExactString(e,a[0],"colorPipelineVersion","LIN_SRGB_REINHARD_V1")&&ExactString(e,a[0],"textureSamplingVersion","P1_TEXEL_CANVAS_PX_V1")&&ExactString(e,a[0],"strataSamplingVersion","CONTINUOUS_COLUMN_WARP_V1");}
 if(valid){stage="MATERIAL_FRAME_IDENTITY";valid=Number(e,a[0],"assetGeneration",generation)&&SafeInteger(generation)&&generation>0&&Number(e,a[0],"contextEpoch",context)&&SafeInteger(context)&&context>0&&Boolean(e,a[0],"lightEnabled",m.lightEnabled)&&MaterialText(e,a[0],"frameToken",token);}
 if(valid){stage="MATERIAL_TDATA";valid=TypedView(e,a[1],napi_float32_array,1536,ptr,stage)&&ParseLight(e,a[2],light,stage)&&ParseMaterialParams(e,a[3],material);}
 if(valid){m.assetGeneration=generation;m.contextEpoch=context;}
 std::lock_guard<std::mutex> lock(g.mutex);Telemetry t{};t.frame=m.frame;
 int code=int(MaterialOwner());
 if(!code&&m.frame.sessionId!=g.id)code=int(Status::RELEASED);
 if(!code&&!valid)code=400;
 if(!code){std::memcpy(g.materialData.data(),ptr,1536*sizeof(float));stage="MATERIAL_VALUE";code=ValidateMaterialFrame(m,light,material,g.materialData);}
 if(!code&&(m.frame.frameId<=g.attempt)){code=int(Status::STALE_FRAME);stage="MATERIAL_STALE_FRAME";}
 if(!code&&(m.contextEpoch!=g.egl.contextEpoch||m.frame.surfaceEpoch!=g.egl.surfaceEpoch)){code=int(Status::STALE_SURFACE);stage="MATERIAL_EPOCH";}
 if(!code&&!g.egl.composition){code=403;stage="MATERIAL_COLORSPACE";}
 const bool activateReady=!code&&g.material.TargetMatches(g.egl.surfaceEpoch,g.egl.contextEpoch)&&g.material.CanActivate(m.assetGeneration,m.contextEpoch);
 const auto* desc=activateReady?g.material.PendingDesc():g.material.ActiveDesc();
 if(!code&&(!desc||desc->generation!=m.assetGeneration||(!activateReady&&!g.material.BindActive(m.contextEpoch)))){code=401;stage="ASSET_NOT_READY";}
 if(!code&&(desc->revision!=m.frame.appearanceConfigRevision||desc->contextEpoch!=m.contextEpoch||desc->geometryEpoch!=m.frame.geometryEpoch)){code=402;stage="ASSET_REVISION_MISMATCH";}
 if(!code){
  uint8_t palette[6];bool match=true;for(int i=0;i<6;i++){double byte=material.paletteBaseSrgb[i]*255;palette[i]=uint8_t(std::floor(byte+.5));if(double(palette[i])/255!=material.paletteBaseSrgb[i])match=false;}
  match=match&&MaterialSha256(palette,6)==desc->paletteHash&&material.grainBaseSrgb==desc->grainBase&&material.warpRangeCycles==desc->warpRange&&material.effectiveLambdaMaterial==desc->lambdaMaterial&&material.effectiveLambdaCanvasPx==desc->lambdaCanvas;
  if(!match){code=402;stage="MATERIAL_CONFIG_GENERATION";}
 }
 if(!code){
  g.attempt=m.frame.frameId;SurfaceInfo si{};
  auto current=g.egl.EnsureCurrent({uint64_t(m.frame.surfaceEpoch),int(m.frame.surfaceWidthPx),int(m.frame.surfaceHeightPx)},si);code=int(current);
  if(!code){
   if(g.materialPass.epoch!=g.egl.contextEpoch){current=g.materialPass.Create({kP013MaterialVertex,kP013MaterialFragment},g.egl.contextEpoch);code=int(current);}
   if(!code && activateReady){auto activated=g.material.Activate(m.assetGeneration,m.contextEpoch,token);code=activated.code;}
   if(!code){glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);current=g.materialPass.Draw(m,light,material,g.materialData.data(),*g.material.BindActive(m.contextEpoch));code=int(current);t.glError=g.materialPass.error;}
   if(!code){t.uploadedFrameId=t.submittedFrameId=m.frame.frameId;current=g.egl.Swap(m.frame.frameId);code=int(current);t.swapSucceeded=!code;}
  }
  if(code)stage=g.materialPass.errorStage;
  auto unbound=g.egl.Unbind();if(unbound!=Status::OK){code=int(unbound);stage="MATERIAL_UNBIND";}
 }
 if(!code&&t.swapSucceeded)g.lastSwap=m.frame.frameId;
 t.code=static_cast<Status>(code);t.state=g.egl.state;t.contextEpoch=g.egl.contextEpoch;t.lastSwapFrameId=g.lastSwap;
 t.eglError=g.egl.error;t.errorStage=code?stage:"NONE";g.lastError=t.code;Save(t);
 auto result=Result(e,t);Put(e,result,"apiVersion",2.0);Put(e,result,"requestedGeneration",generation);Put(e,result,"activeGeneration",double(g.material.Inspect().activeGeneration));
 Put(e,result,"assetGeneration",code?0:generation);Put(e,result,"appearanceConfigRevision",m.frame.appearanceConfigRevision);Put(e,result,"frameToken",token.c_str());
 Put(e,result,"materialRecipeVersion","P1_ALBEDO_DELTA_V1");Put(e,result,"colorPipelineVersion","LIN_SRGB_REINHARD_V1");Put(e,result,"slopeKernelVersion","E2_GAUSS_COL6_R18_P1_V2");Put(e,result,"T_dataLayoutVersion",2.0);Put(e,result,"zOriginVersion",1.0);
 Put(e,result,"shaderHash",kP013MaterialShaderHash);Put(e,result,"materialTdataBytes",!code?6144.0:0.0);Put(e,result,"stale",false);
 Put(e,result,"renderContractRevision","P013_MATERIAL_ISOLATED_V1");Put(e,result,"sceneTransactionImplemented",false);
 Put(e,result,"faultReason",code?stage:"NONE");
 Put(e,result,"materialUploadCpuMs",g.materialPass.uploadMs);Put(e,result,"materialDrawCpuMs",g.materialPass.drawMs);
 return result;
}
// T62: explicit geometry preparation, no draw/swap/physics publication.
napi_value PrepareDualFieldSurface(napi_env e, napi_callback_info info) {
 size_t n=2; napi_value a[2]{}; napi_get_cb_info(e,info,&n,a,nullptr,nullptr);
 double session=0, epoch=0;
 bool valid=n==2 && napi_get_value_double(e,a[0],&session)==napi_ok &&
   napi_get_value_double(e,a[1],&epoch)==napi_ok && SafeInteger(session) && SafeInteger(epoch);
 std::lock_guard<std::mutex> lock(g.mutex);
 Status code=Owner();
 if(code==Status::OK && (!valid || epoch<0)) code=Status::INVALID_ARGUMENT;
 if(code==Status::OK && (!g.ownerBound || session!=g.id || g.released)) code=Status::RELEASED;
 if(code==Status::OK && epoch!=g.egl.surfaceEpoch) code=Status::STALE_SURFACE;
 if(code==Status::OK && (!g.dualBasic || !g.egl.composition)) {
   Destroy(ResetReason::RELEASE); code=g.egl.ConfigureComposition(true); g.dualBasic=code==Status::OK;
 }
 if(code==Status::OK) { SurfaceInfo si{}; code=g.egl.PrepareCurrent(g.egl.surfaceEpoch,si);
   const auto unbound=g.egl.Unbind(); if(unbound!=Status::OK)code=unbound; }
 napi_value o=Control(e,code);
 Put(e,o,"widthPx",static_cast<double>(g.egl.width)); Put(e,o,"heightPx",static_cast<double>(g.egl.height));
 Put(e,o,"sizeSource",code==Status::OK?"EGL_QUERY_SURFACE":"UNAVAILABLE");
 Put(e,o,"geometrySpace","SURFACE_PX_V1");
 Put(e,o,"errorStage",code==Status::OK?"NONE":g.egl.errorStage);
 return o;
}
napi_value DrawDualFieldFrame(napi_env e,napi_callback_info info){
 size_t n=5;napi_value a[5]{};napi_get_cb_info(e,info,&n,a,nullptr,nullptr);
 FrameInput f{};const char* stage="DUAL_ARGUMENT_COUNT";Status code=Status::OK;
 double count=0,capacity=0,stateEpoch=0,commit=0,contextEpoch=0;bool enabled=false;std::string run;DualVisual visual{};
 void* tdata=nullptr;void* xy=nullptr;void* meta=nullptr;
 bool valid=n==5&&ParseFrame(e,a[0],f,stage);
 if(valid){stage="DUAL_PACKET_VERSION";valid=ExactString(e,a[0],"dualFieldPacketRevision","P013_DUALFIELD_PACKET_V1")&&ExactString(e,a[0],"particleLayoutVersion","PARTICLE_XY_META_SOA_V1")&&ExactString(e,a[0],"renderPipelineId","DUALFIELD_BASIC_V1")&&ExactString(e,a[0],"colorPipelineVersion","BASIC_SRGB_DISPLAY_V1");}
 if(valid){stage="DUAL_PUBLICATION";napi_value v=nullptr;valid=Number(e,a[0],"particleCount",count)&&Number(e,a[0],"particleCapacity",capacity)&&Number(e,a[0],"stateEpoch",stateEpoch)&&Number(e,a[0],"contextEpoch",contextEpoch)&&SafeInteger(contextEpoch)&&contextEpoch>=0&&Number(e,a[0],"physicsCommitId",commit)&&Boolean(e,a[0],"particleLayerEnabled",enabled)&&napi_get_named_property(e,a[0],"runId",&v)==napi_ok&&String(e,v,run)&&!run.empty()&&SafeInteger(count)&&SafeInteger(capacity)&&SafeInteger(stateEpoch)&&SafeInteger(commit)&&count>=0&&count<=capacity&&capacity>0&&capacity<=static_cast<double>(SIZE_MAX/16)&&stateEpoch>0&&commit>0;}
 if(valid)valid=TypedView(e,a[1],napi_float32_array,kFloats,tdata,stage)&&TypedView(e,a[2],napi_float32_array,static_cast<size_t>(capacity)*2,xy,stage)&&TypedView(e,a[3],napi_uint32_array,static_cast<size_t>(capacity)*2,meta,stage);
 if(valid){stage="DUAL_VISUAL";valid=ExactString(e,a[4],"visualRevision","P012_EXISTING_PHASE_CANDIDATES_V1")&&Number(e,a[4],"radiusPx",visual.radiusPx)&&std::isfinite(visual.radiusPx)&&visual.radiusPx>0;
 const char* keys[]={"heavyR","heavyG","heavyB","lightR","lightG","lightB","particleHeavyR","particleHeavyG","particleHeavyB","particleLightR","particleLightG","particleLightB"};
 for(size_t i=0;valid&&i<12;i++)valid=Number(e,a[4],keys[i],visual.colors[i])&&std::isfinite(visual.colors[i])&&visual.colors[i]>=0&&visual.colors[i]<=1;}
 if(valid){stage="DUAL_PARTICLE_VALUE";auto* positions=static_cast<float*>(xy);auto* tags=static_cast<uint32_t*>(meta);
 for(size_t i=0;valid&&i<static_cast<size_t>(count);i++){uint32_t tag=tags[2*i];uint32_t state=tag>>8;
 valid=std::isfinite(positions[2*i])&&std::isfinite(positions[2*i+1])&&positions[2*i]>=0&&positions[2*i]<128&&positions[2*i+1]>=0&&positions[2*i+1]<=f.capacity&&(tag&0xffff0000u)==0&&(tag&255u)<=1&&(state==1||state==2)&&tags[2*i+1]>0;
 for(size_t j=0;valid&&j<i;j++)valid=tags[2*j+1]!=tags[2*i+1];}}
 std::lock_guard<std::mutex> lock(g.mutex);Telemetry t{};t.frame=f;code=Owner();
 if(code==Status::OK&&!g.ownerBound)code=g.released?Status::RELEASED:Status::NOT_READY;
 if(code==Status::OK&&f.sessionId!=g.id)code=Status::RELEASED;
 if(code==Status::OK&&!valid)code=Status::INVALID_ARGUMENT;
 if(code==Status::OK&&g.dual.epoch!=0&&contextEpoch!=g.egl.contextEpoch){code=Status::STALE_SURFACE;stage="DUAL_CONTEXT_EPOCH";}
 if(code==Status::OK&&(!SafeInteger(f.frameId)||f.frameId<1))code=Status::INVALID_ARGUMENT;
 if(code==Status::OK&&f.frameId<=g.attempt)code=Status::STALE_FRAME;
 if(code==Status::OK&&g.dualRun==run&&(stateEpoch<g.dualStateEpoch||(stateEpoch==g.dualStateEpoch&&commit<g.dualCommit))){code=Status::STALE_FRAME;stage="DUAL_STALE_PUBLICATION";}
 if(code==Status::OK){g.attempt=f.frameId;stage="DUAL_FIELD_VALIDATE";
 std::array<float,kFloats> check{};std::memcpy(check.data(),tdata,sizeof(check));LightParams light{.85,.15,.8,1.12,1,24,-.55,-.55,.63,6};code=Validate(f,light,check);
 if(code==Status::OK){
  if(g.dualXY.size()!=static_cast<size_t>(capacity)*2){if(g.dual.epoch)Destroy(ResetReason::RELEASE);try { g.dualXY.resize(capacity*2);g.dualMeta.resize(capacity*2); } catch(const std::bad_alloc&) { code=Status::GL_RESOURCE_FAILED; }}
  if(code!=Status::OK){napi_value result=Control(e,code);Put(e,result,"errorStage","DUAL_STAGING_CAPACITY");return result;}
  std::memcpy(g.storage->staging.data(),check.data(),sizeof(check));
  if(count){std::memcpy(g.dualXY.data(),xy,count*8);std::memcpy(g.dualMeta.data(),meta,count*8);}
  if(!g.dualBasic || !g.egl.composition){Destroy(ResetReason::RELEASE);code=g.egl.ConfigureComposition(true);g.dualBasic=code==Status::OK;}
  if(code==Status::OK){SurfaceInfo si{};code=g.egl.EnsureCurrent({static_cast<uint64_t>(f.surfaceEpoch),static_cast<int>(f.surfaceWidthPx),static_cast<int>(f.surfaceHeightPx)},si);
   if(code==Status::STALE_SURFACE)stage="DUAL_SURFACE_EPOCH";
   if(code==Status::OK){if(g.uploader.epoch!=g.egl.contextEpoch)code=g.uploader.Create(g.egl.contextEpoch);
    if(code==Status::OK)code=g.uploader.Upload(g.storage->staging.data(),f.frameId);
    if(code==Status::OK){t.uploadedFrameId=f.frameId;code=g.dual.Draw(f,g.uploader.Texture(),g.dualXY.data(),g.dualMeta.data(),count,capacity,enabled,visual,g.egl.contextEpoch);t.glError=g.dual.error;}
    if(code==Status::OK){t.submittedFrameId=f.frameId;code=g.egl.Swap(f.frameId);t.swapSucceeded=code==Status::OK;}
   }
   auto unbound=g.egl.Unbind();if(unbound!=Status::OK)code=unbound;
  }
 }
 }
 if(code==Status::OK&&t.swapSucceeded){g.lastSwap=f.frameId;g.dualRun=run;g.dualStateEpoch=stateEpoch;g.dualCommit=commit;}
 t.code=code;t.state=g.egl.state;t.contextEpoch=g.egl.contextEpoch;t.lastSwapFrameId=g.lastSwap;
 t.eglError=g.egl.error;t.recoveryRequired=static_cast<int>(code)>=200;
 t.errorStage=code==Status::OK?"NONE":static_cast<int>(code)>=300&&code!=Status::COLORSPACE_UNSUPPORTED?"DUAL_GPU_PASS":static_cast<int>(code)>=200?g.egl.errorStage:stage;
 g.lastError=code;
 if(static_cast<int>(code)>=200){Destroy(ResetReason::FAILURE);g.egl.state=State::FAILED;}
 napi_value result=Result(e,t);Put(e,result,"runId",run.c_str());Put(e,result,"stateEpoch",stateEpoch);Put(e,result,"physicsCommitId",commit);
 Put(e,result,"renderPipelineId","DUALFIELD_BASIC_V1");Put(e,result,"particleCount",count);
 Put(e,result,"particleUploadBytes",code==Status::OK&&enabled?count*16:0);
 Put(e,result,"bufferAllocations",static_cast<double>(g.dual.bufferAllocations));
 Put(e,result,"particleBufferBytes",static_cast<double>(g.dual.capacity*16));
 Put(e,result,"particleCapacity",capacity);Put(e,result,"colorPipelineVersion","BASIC_SRGB_DISPLAY_V1");
 char shaderHash[17]{};std::snprintf(shaderHash,sizeof(shaderHash),"%016llx",static_cast<unsigned long long>(g.dual.sourceHash));
 Put(e,result,"shaderHash",shaderHash);Put(e,result,"shaderHashAlgorithm","FNV1A64");
 size_t clipped=0;
 if(valid)for(size_t i=0;i<static_cast<size_t>(count);i++){
  const float* positions=static_cast<const float*>(xy);const double x=positions[2*i]/128*f.boxWidthPx;
  const double y=positions[2*i+1]/f.capacity*f.boxHeightPx;
  if(x<visual.radiusPx||x>f.boxWidthPx-visual.radiusPx||y<visual.radiusPx||y>f.boxHeightPx-visual.radiusPx)clipped++;
 }
 Put(e,result,"particleClippedCount",static_cast<double>(clipped));Put(e,result,"particleOffscreenCount",0.0);
 return result;
}
napi_value DrawFrame(napi_env e, napi_callback_info info) {
  const double start = NowMs();
  Telemetry t{};
  bool eglAttempted = false, drawAttempted = false;
  size_t n = 3;
  napi_value a[3]{};
  napi_get_cb_info(e, info, &n, a, nullptr, nullptr);
  FrameInput f{};
  LightParams l{};
  const char* parseStage = "FRAME_ARGUMENT_COUNT";
  bool parsed = n == 3 && ParseFrame(e, a[0], f, parseStage) && ParseLight(e, a[2], l, parseStage);
  std::string kernel;
  napi_value kv = nullptr;
  bool version =
      n == 3 && parsed &&
      napi_get_named_property(e, a[0], "slopeKernelVersion", &kv) == napi_ok &&
      String(e, kv, kernel);
  napi_typedarray_type type{};
  size_t length = 0, offset = 0;
  void* data = nullptr;
  napi_value buffer = nullptr;
  bool detached = false;
  const double copyStart = NowMs();
  bool isTyped = false;
  if (n == 3) napi_is_typedarray(e, a[1], &isTyped);
  const char* typedStage = "TDATA_NOT_TYPEDARRAY";
  bool typed = false;
  void* base = nullptr;
  size_t bytes = 0;
  if (n == 3 && isTyped) {
    if (napi_get_typedarray_info(e, a[1], &type, &length, &data, &buffer, &offset) != napi_ok)
      typedStage = "TDATA_INFO";
    else if (type != napi_float32_array) typedStage = "TDATA_TYPE";
    // Ark NAPI returns bytes; kFloats remains the exact element contract.
    else if (length != kFloats * sizeof(float)) typedStage = "TDATA_LENGTH";
    else if (!data) typedStage = "TDATA_DATA";
    else if (napi_is_detached_arraybuffer(e, buffer, &detached) != napi_ok)
      typedStage = "TDATA_DETACH_QUERY";
    else if (detached) typedStage = "TDATA_DETACHED";
    else if (napi_get_arraybuffer_info(e, buffer, &base, &bytes) != napi_ok || !base)
      typedStage = "TDATA_BUFFER_INFO";
    else if (offset > bytes || kFloats * sizeof(float) > bytes - offset)
      typedStage = "TDATA_BUFFER_RANGE";
    else if (offset % sizeof(float) != 0) typedStage = "TDATA_OFFSET";
    else if (data != static_cast<unsigned char*>(base) + offset) typedStage = "TDATA_POINTER";
    else typed = true;
  }
  const double lockStart = NowMs();
  std::unique_lock<std::mutex> lock(g.mutex);
  t.nativeTypedByteLength = length; t.nativeTypedByteOffset = offset;
  t.nativeBufferByteLength = bytes;
  t.lifecycleLockWaitMs = NowMs() - lockStart;
  t.compositionProfile = g.egl.composition;
  t.frame = f;
  t.light = l;
  t.inputLight = l;
  t.ownerThreadId = static_cast<uint64_t>(syscall(SYS_gettid));
  Status code = Owner();
  if (code == Status::OK && g.dualBasic) { code = Status::NOT_READY; t.errorStage = "DUAL_PROFILE_REQUIRES_DUAL_ABI"; }
  if (code == Status::OK && !g.ownerBound)
    code = g.released ? Status::RELEASED : Status::NOT_READY;
  if (code == Status::OK && f.sessionId != g.id) code = Status::RELEASED;
  if (code == Status::OK) {
    if (!SafeInteger(f.frameId) || f.frameId < 1)
      code = Status::INVALID_ARGUMENT;
    else if (f.frameId <= g.attempt)
      code = Status::STALE_FRAME;
    else {
      t.frameGap = f.frameId - g.attempt - 1;
      g.attempt = static_cast<uint64_t>(f.frameId);
    }
  }
  if (code == Status::OK && (!parsed || !typed || !version)) {
    code = Status::INVALID_ARGUMENT;
    t.errorStage = !parsed ? parseStage : !typed ? typedStage : "SLOPE_KERNEL_FIELD";
  }
  if (code == Status::OK && kernel != "E2_APPROX_P0") {
    code = Status::VERSION_MISMATCH;
    t.errorStage = "SLOPE_KERNEL_VERSION";
  }
  if (code == Status::OK) {
    const double ct = NowMs();
    std::memcpy(g.storage->staging.data(), data, kFloats * sizeof(float));
    t.bridgeCopyMs = NowMs() - ct;
    t.tdataHash = 14695981039346656037ULL;
    const auto* bytes =
        reinterpret_cast<const unsigned char*>(g.storage->staging.data());
    for (size_t i = 0; i < sizeof(g.storage->staging); i++) {
      t.tdataHash ^= bytes[i];
      t.tdataHash *= 1099511628211ULL;
    }
    const double vt = NowMs();
    code = Validate(f, l, g.storage->staging);
    if (code != Status::OK) t.errorStage = "FRAME_VALIDATE";
    t.bridgeValidateMs = NowMs() - vt;
  }
  const double readValidateMs = NowMs() - copyStart - t.lifecycleLockWaitMs;
  if (code == Status::OK && !g.egl.active) code = Status::INACTIVE;
  if (code == Status::OK) {
    const double norm = std::hypot(std::hypot(l.lightX, l.lightY), l.lightZ);
    l.lightX /= norm;
    l.lightY /= norm;
    l.lightZ /= norm;
    t.light = l;
    eglAttempted = true;
    SurfaceInfo si{};
    code = g.egl.EnsureCurrent({static_cast<uint64_t>(f.surfaceEpoch),
                                static_cast<int>(f.surfaceWidthPx),
                                static_cast<int>(f.surfaceHeightPx)},
                               si);
    if (code == Status::OK) {
      if (!g.egl.composition && g.shader.epoch != g.egl.contextEpoch) {
        ResetGpu();
        code =
            g.shader.Create({kP013Vertex, kP013Fragment}, g.egl.contextEpoch);
        if (code == Status::OK) code = g.uploader.Create(g.egl.contextEpoch);
        t.shaderBuildMs = g.shader.buildMs;
      }
      if (code == Status::OK && f.diagnosticEnabled && !g.egl.composition) {
        code = g.uploader.Upload(g.storage->staging.data(),
                                 static_cast<uint64_t>(f.frameId));
        t.uploadCpuMs = g.uploader.uploadMs;
        t.glError = g.uploader.error;
        if (code == Status::OK) t.uploadedFrameId = f.frameId;
      }
      if (code == Status::OK) {
        drawAttempted = true;
        code = g.egl.composition ? g.probe.Draw(f, g.egl.contextEpoch)
                                 : g.shader.Draw({f, l}, g.uploader.Texture());
        t.probeSourceHash = g.probe.sourceHash;
        t.glError = g.egl.composition ? g.probe.error : g.shader.error;
        if (code == Status::OK) t.submittedFrameId = f.frameId;
      }
      if (code == Status::OK) {
        code = g.egl.Swap(static_cast<uint64_t>(f.frameId));
        t.swapSucceeded = code == Status::OK;
      }
    }
    const Status unbound = g.egl.Unbind();
    if (unbound != Status::OK) code = unbound;
  }
  if (code == Status::OK && t.swapSucceeded)
    g.lastSwap = static_cast<uint64_t>(f.frameId);
  t.recoveryRequired = static_cast<int>(code) >= 200;
  if (t.recoveryRequired) {
    Destroy(ResetReason::FAILURE);
    g.egl.state = State::FAILED;
  }
  t.state = g.egl.state;
  t.compileLogHash = g.shader.compileLogHash;
  t.contextEpoch = g.egl.contextEpoch;
  t.lastSwapFrameId = g.lastSwap;
  t.code = code;
  g.lastError = code;
  if (eglAttempted) {
    t.eglError = g.egl.error;
    t.errorStage = g.egl.errorStage;
    t.eglInitMs = g.egl.initMs;
    t.makeCurrentMs = g.egl.currentMs;
    t.unbindMs = g.egl.unbindMs;
    t.swapMs = g.egl.swapMs;
  }
  if (drawAttempted) {
    t.uniformCpuMs = g.shader.uniformMs;
    t.drawCpuMs = g.shader.drawMs;
  }
  if (code != Status::OK && std::strcmp(t.errorStage, "NONE") == 0)
    t.errorStage = "FRAME_REJECTED";
  if (code == Status::INVALID_ARGUMENT && parsed && typed && version &&
      (l.lref - l.lmin > 1 || l.lmax - l.lref > 1))
    t.errorStage = "DIAG_RANGE";
  t.uploadPathMs = readValidateMs + t.uploadCpuMs;
  t.uploadCount = g.uploader.uploadCount;
  t.textureAllocCount = g.uploader.textureAllocCount;
  t.textureAbandonCount = g.uploader.textureAbandonCount;
  t.programAbandonCount = g.shader.programAbandonCount + g.probe.programAbandonCount;
  t.vaoAbandonCount = g.shader.vaoAbandonCount + g.probe.vaoAbandonCount;
  t.textureLiveCount = g.uploader.Texture() ? 1 : 0;
  t.programLiveCount = (g.shader.Program() ? 1 : 0) + (g.probe.Program() ? 1 : 0);
  t.vaoLiveCount = (g.shader.Vao() ? 1 : 0) + (g.probe.Vao() ? 1 : 0);
  t.surfaceCreateCount = g.egl.surfaceCreateCount;
  t.surfaceDestroyCount = g.egl.surfaceDestroyCount;
  t.contextCreateCount = g.egl.contextCreateCount;
  t.contextDestroyCount = g.egl.contextDestroyCount;
  t.contextLossCount = g.egl.contextLossCount;
  t.recoveryAttemptCount = g.egl.recoveryAttemptCount;
  t.staleLifecycleCount = g.egl.staleLifecycleCount;
  napi_value result = Result(e, t);
  t.bridgeCpuMs = NowMs() - start;
  Save(t);
  return result;
}
// G0 composition profile is explicit and separate from the unchanged P0 contract.
std::string ProbeToken() {
  if (!g.probe.captured) return "";
  const auto& f = g.probe.frame;
  return std::to_string(static_cast<uint64_t>(f.sessionId)) + ":" +
    std::to_string(static_cast<uint64_t>(f.frameId)) + ":" +
    std::to_string(static_cast<uint64_t>(f.snapshotId)) + ":" +
    std::to_string(static_cast<uint64_t>(f.surfaceEpoch)) + ":" + std::to_string(g.probe.epoch);
}
napi_value CompositionReceipt(napi_env e, Status code, const char* stage) {
  napi_value o = Control(e, code);
  Put(e,o,"compositionRevision",g.egl.composition ? "P013_P1_TEXTURE_STACK_V1" : "P0_LEGACY");
  Put(e,o,"requestedColorSpace",g.egl.composition ? "SRGB" : "LEGACY_UNSPECIFIED");
  Put(e,o,"actualColorSpace",g.egl.actualColorSpace == 0x3089 ? "SRGB" : "UNPROVEN");
  Put(e,o,"attachmentEncoding",g.egl.attachmentEncoding == GL_SRGB ? "GL_SRGB" : "UNPROVEN");
  Put(e,o,"drawFramebuffer",static_cast<double>(g.egl.drawFramebuffer));
  Put(e,o,"readFramebuffer",static_cast<double>(g.egl.readFramebuffer));
  Put(e,o,"eglAlphaSize",static_cast<double>(g.egl.alphaSize));
  Put(e,o,"srgbWriteControlSupported",g.egl.writeControlSupported);
  Put(e,o,"srgbWriteEnabled",g.egl.srgbWriteEnabled);
  Put(e,o,"profileStatus",code != Status::OK ? "UNPROVEN" : !g.egl.composition ? "P0_LEGACY" :
    g.egl.state == State::READY && g.egl.attachmentEncoding == GL_SRGB ? "READY" : "PENDING");
  Put(e,o,"errorStage",stage);
  Put(e,o,"eglVersion",g.egl.eglVersion.c_str());Put(e,o,"eglExtensions",g.egl.eglExtensions.c_str());
  Put(e,o,"glVersion",g.egl.glVersion.c_str());Put(e,o,"glExtensions",g.egl.glExtensions.c_str());
  Put(e,o,"eglError",static_cast<double>(g.egl.error));Put(e,o,"glError",static_cast<double>(g.probe.error));
  Put(e,o,"packAlignment",1.0);Put(e,o,"rowDirection","BOTTOM_UP");
  Put(e,o,"viewportWidth",g.probe.frame.surfaceWidthPx);Put(e,o,"viewportHeight",g.probe.frame.surfaceHeightPx);
  Put(e,o,"frameToken",ProbeToken().c_str());
  Put(e,o,"physicalSnapshotHash","UNPROVEN_PROBE_HAS_NO_PHYSICS");
  char hash[17]{};std::snprintf(hash,sizeof(hash),"%016llx",static_cast<unsigned long long>(g.probe.sourceHash));
  Put(e,o,"shaderHash",hash);Put(e,o,"shaderHashAlgorithm","FNV1A64");
  Put(e,o,"readbackStage","PRE_SWAP_DEFAULT_BACK");
  Put(e,o,"readbackStatus",g.probe.captured ? "MEASURED_NOT_CALIBRATED" : "UNPROVEN");
  napi_value points=nullptr, rgba=nullptr;
  napi_create_array_with_length(e,g.probe.captured ? 12 : 0,&points);
  napi_create_array_with_length(e,g.probe.captured ? 24 : 0,&rgba);
  if(g.probe.captured) {
    for(size_t i=0;i<12;i++){napi_value v; napi_create_double(e,g.probe.points[i],&v);napi_set_element(e,points,i,v);}
    for(size_t i=0;i<24;i++){napi_value v; napi_create_double(e,g.probe.rgba[i],&v);napi_set_element(e,rgba,i,v);}
  }
  napi_value linear=nullptr,srgb=nullptr;
  napi_create_array_with_length(e,g.probe.captured?24:0,&linear);napi_create_array_with_length(e,g.probe.captured?24:0,&srgb);
  if(g.probe.captured)for(size_t i=0;i<24;i++){
    napi_value v; napi_create_double(e,g.probe.linearRgba[i],&v);napi_set_element(e,linear,i,v);
    napi_create_double(e,g.probe.srgbRgba[i],&v);napi_set_element(e,srgb,i,v);
  }
  napi_set_named_property(e,o,"linearRgba8",linear);napi_set_named_property(e,o,"srgbRgba8",srgb);
  napi_set_named_property(e,o,"probePoints",points);napi_set_named_property(e,o,"rgba",rgba);
  return o;
}
napi_value RequestComposition(napi_env e, napi_callback_info info) {
  size_t n=1;napi_value a[1]{};napi_get_cb_info(e,info,&n,a,nullptr,nullptr);
  const char* names[]={"compositionRevision","componentType","colorPipelineVersion","alphaMode","hitMode"};
  const char* values[]={"P013_P1_TEXTURE_STACK_V1","TEXTURE","LIN_SRGB_REINHARD_V1","BINARY_MASK","NONE"};
  bool valid=n==1, probe=false;
  for(size_t i=0;valid && i<5;i++){napi_value v=nullptr;std::string value;
    valid=napi_get_named_property(e,a[0],names[i],&v)==napi_ok && String(e,v,value) && value==values[i];}
  if(valid)valid=Boolean(e,a[0],"probeOnly",probe);
  std::lock_guard<std::mutex> lock(g.mutex);Status code=Owner();
  if(code==Status::OK && !g.ownerBound)code=Status::NOT_READY;
  if(code==Status::OK && !valid)code=Status::INVALID_ARGUMENT;
  if(code==Status::OK && g.dualBasic)code=Status::SESSION_BUSY;
  if(code!=Status::OK)return CompositionReceipt(e,code,"COMPOSITION_PROFILE");
  // Full P1 material remains behind G1-G4; never present this color probe as material.
  if(!probe)return CompositionReceipt(e,Status::NOT_READY,"P1_MATERIAL_NOT_IMPLEMENTED");
  if(!g.egl.composition){Destroy(ResetReason::RELEASE);code=g.egl.ConfigureComposition(true);}
  return CompositionReceipt(e,code,code==Status::OK?"NONE":"COMPOSITION_SWITCH");
}
napi_value QueryComposition(napi_env e,napi_callback_info) {
  std::lock_guard<std::mutex> lock(g.mutex);
  Status code=Owner();if(code==Status::OK && static_cast<int>(g.lastError)>=200)code=g.lastError;
  return CompositionReceipt(e,code,g.egl.errorStage);
}
napi_value CaptureProbe(napi_env e,napi_callback_info info) {
  size_t n=2;napi_value a[2]{};napi_get_cb_info(e,info,&n,a,nullptr,nullptr);
  std::string token;bool valid=n==2 && String(e,a[0],token);
  napi_typedarray_type type{};size_t count=0,offset=0,bytes=0;void* data=nullptr;void* base=nullptr;
  napi_value buffer=nullptr;bool detached=false,isTyped=false;
  if(valid)valid=napi_is_typedarray(e,a[1],&isTyped)==napi_ok && isTyped &&
    napi_get_typedarray_info(e,a[1],&type,&count,&data,&buffer,&offset)==napi_ok &&
    type==napi_float32_array && count==12*sizeof(float) && data &&
    napi_is_detached_arraybuffer(e,buffer,&detached)==napi_ok && !detached &&
    napi_get_arraybuffer_info(e,buffer,&base,&bytes)==napi_ok && base && offset<=bytes &&
    12*sizeof(float)<=bytes-offset && offset%sizeof(float)==0 && data==static_cast<unsigned char*>(base)+offset;
  std::lock_guard<std::mutex> lock(g.mutex);Status code=Owner();
  if(code!=Status::OK)return CompositionReceipt(e,code,"PROBE_OWNER");
  if(!valid)return CompositionReceipt(e,Status::INVALID_ARGUMENT,"PROBE_POINTS");
  if(!g.egl.composition || !g.probe.captured || token!=ProbeToken() ||
    g.probe.frame.surfaceEpoch!=g.egl.surfaceEpoch || g.probe.epoch!=g.egl.contextEpoch ||
    g.probe.frame.frameId!=g.lastSwap)
    return CompositionReceipt(e,Status::STALE_FRAME,"PROBE_TOKEN");
  for(size_t i=0;i<12;i++)if(static_cast<float*>(data)[i]!=g.probe.points[i])
    return CompositionReceipt(e,Status::INVALID_ARGUMENT,"PROBE_POINTS_NOT_CAPTURED");
  return CompositionReceipt(e,Status::OK,"NONE");
}
napi_value Drain(napi_env e, napi_callback_info) {
  std::lock_guard<std::mutex> lock(g.mutex);
  if (Owner() != Status::OK) return Control(e, Status::WRONG_THREAD);
  napi_value batch = Control(e, Status::OK), rows = nullptr;
  napi_create_array_with_length(e, g.count, &rows);
  for (size_t i = 0; i < g.count; i++) {
    const auto& t = g.storage->ring[(g.head + i) % kTelemetryCapacity];
    napi_value o = Result(e, t);
#define METRIC(x) Put(e, o, #x, static_cast<double>(t.x))
    METRIC(bridgeValidateMs);
    METRIC(bridgeCopyMs);
    METRIC(bridgeCpuMs);
    METRIC(uploadCpuMs);
    METRIC(uploadPathMs);
    METRIC(eglInitMs);
    METRIC(makeCurrentMs);
    METRIC(unbindMs);
    METRIC(swapMs);
    METRIC(lifecycleLockWaitMs);
    METRIC(shaderBuildMs);
    METRIC(uniformCpuMs);
    METRIC(drawCpuMs);
    METRIC(ownerThreadId);
    METRIC(frameGap);
    METRIC(uploadCount);
    METRIC(textureAllocCount);
    METRIC(textureAbandonCount);
    METRIC(programAbandonCount);
    METRIC(vaoAbandonCount);
    METRIC(textureLiveCount);
    METRIC(programLiveCount);
    METRIC(vaoLiveCount);
    METRIC(surfaceCreateCount);
    METRIC(surfaceDestroyCount);
    METRIC(contextCreateCount);
    METRIC(contextDestroyCount);
    METRIC(contextLossCount);
    METRIC(recoveryAttemptCount);
    METRIC(staleLifecycleCount);
#undef METRIC
#define FRAME(x) Put(e, o, #x, t.frame.x)
    FRAME(sessionId);
    FRAME(frameId);
    FRAME(snapshotId);
    FRAME(surfaceEpoch);
    FRAME(fieldEpoch);
    FRAME(geometryEpoch);
    FRAME(continuityEpoch);
    FRAME(appearanceConfigRevision);
    FRAME(lightConfigRevision);
    FRAME(frameTimeNs);
    FRAME(T_dataLayoutVersion);
    FRAME(zOriginVersion);
    FRAME(surfaceWidthPx);
    FRAME(surfaceHeightPx);
    FRAME(boxXPx);
    FRAME(boxYPx);
    FRAME(boxWidthPx);
    FRAME(boxHeightPx);
    FRAME(capacity);
    FRAME(visualInverted);
    FRAME(diagnosticEnabled);
#undef FRAME
    Put(e, o, "contextEpoch", t.contextEpoch);
    Put(e, o, "schemaVersion", 1.0);
    Put(e, o, "slopeKernelVersion", "E2_APPROX_P0");
    Put(e, o, "mode",
        t.compositionProfile ? (t.frame.diagnosticEnabled ? "G0_COLOR_PROBE_ON" : "G0_COLOR_PROBE_OFF") :
        (t.frame.diagnosticEnabled ? "GLES_LMAT_DIAG_ON" : "GLES_LMAT_DIAG_OFF"));
    Put(e, o, "diagEncoding", t.compositionProfile ? "G0_LINEAR_COLOR_BLOCKS_V1" : "DIAG_GRAY_V1");
    Put(e, o, "profileKernel", t.compositionProfile ? "G0_COLOR_BINARY_V1" : "DISPLAY_4X3_V1");
    Put(e, o, "maskRevision", t.compositionProfile ? "G0_BINARY_BOX_V1" : "P013_CONTINUOUS_SAND_V1");
    Put(e, o, "lightFrame", t.compositionProfile ? "UNUSED_PROBE" : "CONTAINER_PRE_INVERT_V1");
    if(t.compositionProfile) {
      char probeHash[17]{};std::snprintf(probeHash,sizeof(probeHash),"%016llx",static_cast<unsigned long long>(t.probeSourceHash));
      Put(e,o,"shaderHash",probeHash);Put(e,o,"shaderHashAlgorithm","FNV1A64");
      Put(e,o,"compositionRevision","P013_P1_TEXTURE_STACK_V1");
    } else Put(e, o, "shaderHash", kP013ShaderHash);
    Put(e, o, "nativeTypedByteLength", static_cast<double>(t.nativeTypedByteLength));
    Put(e, o, "nativeTypedByteOffset", static_cast<double>(t.nativeTypedByteOffset));
    Put(e, o, "nativeBufferByteLength", static_cast<double>(t.nativeBufferByteLength));
    Put(e, o, "errorStage", t.errorStage);
    char hash[24]{};
    std::snprintf(hash, sizeof(hash), "%016llx",
                  static_cast<unsigned long long>(t.tdataHash));
    Put(e, o, "T_dataHash", hash);
    std::snprintf(hash, sizeof(hash), "%016llx",
                  static_cast<unsigned long long>(t.compileLogHash));
    Put(e, o, "compileLogHash", hash);
    Put(e, o, "uploadBytes", t.uploadedFrameId ? 2048.0 : 0.0);
    Put(e, o, "texturePayloadBytes", t.textureLiveCount ? 2048.0 : 0.0);
    Put(e, o, "nativeOwnedBytes",
        static_cast<double>(g.storage ? sizeof(FrameStorage) : 0));
    Put(e, o, "gpuTimerValid", false);
    Put(e, o, "gpuDisjoint", false);
    napi_value nullValue = nullptr;
    napi_get_null(e, &nullValue);
    napi_set_named_property(e, o, "gpuPassMs", nullValue);
    Put(e, o, "gpuQueryPendingCount", 0.0);
    napi_value light = Object(e);
#define LIGHT(x) Put(e, light, #x, t.inputLight.x)
    LIGHT(la);
    LIGHT(ld);
    LIGHT(lmin);
    LIGHT(lmax);
    LIGHT(lref);
    LIGHT(ellLightMaterial);
    LIGHT(lightX);
    LIGHT(lightY);
    LIGHT(lightZ);
    LIGHT(sigmaCandidate);
#undef LIGHT
    napi_set_named_property(e, o, "lightConfig", light);
    napi_value normalized = Object(e);
    Put(e, normalized, "lightX", t.light.lightX);
    Put(e, normalized, "lightY", t.light.lightY);
    Put(e, normalized, "lightZ", t.light.lightZ);
    napi_set_named_property(e, o, "normalizedLightDirection", normalized);
    Put(e, o, "gpuTimerStatus", "UNMEASURED_USE_SYSTEM_TRACE");
    napi_set_element(e, rows, i, o);
  }
  napi_set_named_property(e, batch, "records", rows);
  Put(e, batch, "droppedTelemetry", static_cast<double>(g.dropped));
  Put(e, batch, "telemetryLiveCount", static_cast<double>(g.count));
  g.head = g.count = 0;
  return batch;
}
}  // namespace
}  // namespace p013
EXTERN_C_START
static napi_value Init(napi_env e, napi_value exports) {
  using namespace p013;
  napi_property_descriptor d[] = {
    {"drawMaterialFrame",nullptr,DrawMaterialFrame,nullptr,nullptr,nullptr,napi_default,nullptr},
    {"bindMaterialConfiguration", nullptr, BindMaterialConfiguration, nullptr,nullptr,nullptr,napi_default,nullptr},
    {"stageMaterialBundle", nullptr, StageMaterialBundle, nullptr,nullptr,nullptr,napi_default,nullptr},
    {"pumpMaterialUploads", nullptr, PumpMaterialUploads, nullptr,nullptr,nullptr,napi_default,nullptr},
    {"canActivate", nullptr, CanActivateMaterial, nullptr,nullptr,nullptr,napi_default,nullptr},
    {"activateMaterial", nullptr, ActivateMaterial, nullptr,nullptr,nullptr,napi_default,nullptr},
    {"discardMaterialStage", nullptr, DiscardMaterialStage, nullptr,nullptr,nullptr,napi_default,nullptr},
    {"prepareDualFieldSurface", nullptr, PrepareDualFieldSurface, nullptr, nullptr, nullptr, napi_default, nullptr},
    {"drawDualFieldFrame", nullptr, DrawDualFieldFrame, nullptr, nullptr, nullptr, napi_default, nullptr},
                                  {"requestComposition", nullptr, RequestComposition, nullptr, nullptr, nullptr, napi_default, nullptr},
                                  {"queryCompositionProfile", nullptr, QueryComposition, nullptr, nullptr, nullptr, napi_default, nullptr},
                                  {"captureProbe", nullptr, CaptureProbe, nullptr, nullptr, nullptr, napi_default, nullptr},{"init", nullptr, InitSession, nullptr,
                                   nullptr, nullptr, napi_default, nullptr},
                                  {"getState", nullptr, GetState, nullptr,
                                   nullptr, nullptr, napi_default, nullptr},
                                  {"setActive", nullptr, SetActive, nullptr,
                                   nullptr, nullptr, napi_default, nullptr},
                                  {"drawFrame", nullptr, DrawFrame, nullptr,
                                   nullptr, nullptr, napi_default, nullptr},
                                  {"release", nullptr, Release, nullptr,
                                   nullptr, nullptr, napi_default, nullptr},
                                  {"drainTelemetry", nullptr, Drain, nullptr,
                                   nullptr, nullptr, napi_default, nullptr}};
  napi_define_properties(e, exports, sizeof(d) / sizeof(d[0]), d);
  Hook(e, exports);
  napi_add_env_cleanup_hook(e, Cleanup, nullptr);
  return exports;
}
EXTERN_C_END
static napi_module module = {1, 0, nullptr, Init, "p013renderer", nullptr, {0}};
extern "C" __attribute__((constructor)) void RegisterP013Renderer() {
  napi_module_register(&module);
}
