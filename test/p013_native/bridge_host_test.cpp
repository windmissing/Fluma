// Real bridge transaction, fake JS/GPU/platform boundary. Not Ark
// runtime/presentation.
#include <napi/native_api.h>

#include <cstring>
#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <vector>
struct napi_value__ {
  napi_valuetype type = napi_object;
  double number = 0;
  bool boolean = false, typed = false, detached = false;
  std::string text;
  std::map<std::string, napi_value> fields;
  std::vector<napi_value> elements;
  std::vector<unsigned char> bytes;
  napi_value buffer = nullptr;
  size_t offset = 0, length = 0;
  napi_typedarray_type arrayType = napi_float32_array;
};
struct napi_callback_info__ {
  std::vector<napi_value> args;
};
static std::vector<std::unique_ptr<napi_value__>> values;
static napi_value Value(napi_valuetype t = napi_object) {
  values.push_back(std::make_unique<napi_value__>());
  auto* v = values.back().get();
  v->type = t;
  return v;
}
extern "C" {
napi_status napi_create_object(napi_env, napi_value* r) {
  *r = Value();
  return napi_ok;
}
napi_status napi_create_double(napi_env, double d, napi_value* r) {
  *r = Value(napi_number);
  (*r)->number = d;
  return napi_ok;
}
napi_status napi_set_named_property(napi_env, napi_value o, const char* k,
                                    napi_value v) {
  o->fields[k] = v;
  return napi_ok;
}
napi_status napi_get_named_property(napi_env, napi_value o, const char* k,
                                    napi_value* v) {
  if (!o || o->type != napi_object) return napi_invalid_arg;
  auto i = o->fields.find(k);
  *v = i == o->fields.end() ? Value(napi_undefined) : i->second;
  return napi_ok;
}
napi_status napi_typeof(napi_env, napi_value v, napi_valuetype* t) {
  if (!v) return napi_invalid_arg;
  *t = v->type;
  return napi_ok;
}
napi_status napi_get_value_double(napi_env, napi_value v, double* d) {
  *d = v->number;
  return napi_ok;
}
napi_status napi_get_value_bool(napi_env, napi_value v, bool* d) {
  *d = v->boolean;
  return napi_ok;
}
napi_status napi_get_boolean(napi_env, bool b, napi_value* r) {
  *r = Value(napi_boolean);
  (*r)->boolean = b;
  return napi_ok;
}
napi_status napi_create_string_utf8(napi_env, const char* s, size_t n,
                                    napi_value* r) {
  *r = Value(napi_string);
  (*r)->text.assign(s, n == NAPI_AUTO_LENGTH ? std::strlen(s) : n);
  return napi_ok;
}
napi_status napi_get_value_string_utf8(napi_env, napi_value v, char* b,
                                       size_t n, size_t* out) {
  *out = std::min(n - 1, v->text.size());
  std::memcpy(b, v->text.data(), *out);
  b[*out] = 0;
  return napi_ok;
}
napi_status napi_has_named_property(napi_env, napi_value o, const char* k,
                                    bool* b) {
  *b = o->fields.count(k) > 0;
  return napi_ok;
}
napi_status napi_unwrap(napi_env, napi_value, void**) {
  return napi_invalid_arg;
}
napi_status napi_define_properties(napi_env, napi_value, size_t,
                                   const napi_property_descriptor*) {
  return napi_ok;
}
napi_status napi_get_cb_info(napi_env, napi_callback_info i, size_t* n,
                             napi_value* a, napi_value*, void**) {
  size_t count = std::min(*n, i->args.size());
  for (size_t k = 0; k < count; k++) a[k] = i->args[k];
  *n = count;
  return napi_ok;
}
napi_status napi_is_typedarray(napi_env, napi_value v, bool* b) {
  *b = v && v->typed;
  return napi_ok;
}
napi_status napi_get_typedarray_info(napi_env, napi_value v,
                                     napi_typedarray_type* t, size_t* n,
                                     void** d, napi_value* b, size_t* off) {
  *t = v->arrayType;
  // OpenHarmony returns ByteLength (unlike standard Node element count).
  *n = v->length * 4;
  *off = v->offset;
  *b = v->buffer;
  *d = v->buffer->bytes.data() + v->offset;
  return napi_ok;
}
napi_status napi_is_detached_arraybuffer(napi_env, napi_value v, bool* b) {
  *b = v->detached;
  return napi_ok;
}
napi_status napi_get_arraybuffer_info(napi_env, napi_value v, void** d,
                                      size_t* n) {
  *d = v->bytes.data();
  *n = v->bytes.size();
  return napi_ok;
}
napi_status napi_create_array_with_length(napi_env, size_t n, napi_value* r) {
  *r = Value();
  (*r)->elements.resize(n);
  return napi_ok;
}
napi_status napi_set_element(napi_env, napi_value a, uint32_t i, napi_value v) {
  a->elements.at(i) = v;
  return napi_ok;
}
napi_status napi_get_null(napi_env, napi_value* r) {
  *r = Value(napi_null);
  return napi_ok;
}
napi_status napi_add_env_cleanup_hook(napi_env, void (*)(void*), void*) {
  return napi_ok;
}
napi_status napi_throw_error(napi_env, const char*, const char*) {
  return napi_ok;
}
void napi_module_register(napi_module*) {}
}
#define main lifecycle_test_main
#include "lifecycle_test.cpp"
#undef main
#include "../../entry/src/main/cpp/p013renderer/napi_bridge.cpp"
static std::array<float, 512> gpuData{};
static bool uploadFailure = false;
namespace p013 {
Status ShaderPass::Create(const ShaderSources&, uint64_t e) {
  epoch = e;
  program = 1;
  vao = 1;
  return Status::OK;
}
Status ShaderPass::Draw(const FrameUniforms&, GLuint) { return Status::OK; }
void ShaderPass::Reset(bool current) {
  if (!current) {
    programAbandonCount += program ? 1 : 0;
    vaoAbandonCount += vao ? 1 : 0;
  }
  program = vao = 0;
  epoch = 0;
}
Status TDataUploader::Create(uint64_t e) {
  epoch = e;
  texture = 1;
  textureAllocCount++;
  return Status::OK;
}
Status TDataUploader::Upload(const float* p, uint64_t) {
  if (uploadFailure) return Status::GL_RESOURCE_FAILED;
  std::copy(p, p + 512, gpuData.begin());
  uploadCount++;
  return Status::OK;
}
void TDataUploader::Reset(bool current) {
  if (texture && !current) textureAbandonCount++;
  texture = 0;
  epoch = 0;
}
}  // namespace p013
extern "C" {
int32_t OH_NativeXComponent_GetXComponentId(OH_NativeXComponent*, char*,
                                            uint64_t*) {
  return -1;
}
int32_t OH_NativeXComponent_RegisterCallback(OH_NativeXComponent*,
                                             OH_NativeXComponent_Callback*) {
  return 0;
}
}
static napi_env env = reinterpret_cast<napi_env>(1);
static napi_value Num(double x) {
  napi_value v;
  napi_create_double(env, x, &v);
  return v;
}
static napi_value Text(const char* s) {
  napi_value v;
  napi_create_string_utf8(env, s, NAPI_AUTO_LENGTH, &v);
  return v;
}
static void Set(napi_value o, const char* key, double x) {
  o->fields[key] = Num(x);
}
static napi_value Call(napi_callback fn, std::initializer_list<napi_value> a) {
  napi_callback_info__ i;
  i.args = a;
  return fn(env, &i);
}
static double Read(napi_value v, const char* key) {
  return v->fields.at(key)->number;
}
namespace p013 {
Status DualFieldPass::Draw(const FrameInput&,GLuint,const float*,const uint32_t*,size_t count,size_t nmax,bool enabled,const DualVisual&,uint64_t generation){epoch=generation;capacity=nmax;uploadBytes=enabled?count*16:0;return Status::OK;}
void DualFieldPass::Reset(bool){epoch=0;capacity=0;}
void CompositionProbe::Reset(bool) { epoch=0;captured=false; }
Status CompositionProbe::Draw(const FrameInput& f,uint64_t generation) {
  epoch=generation;frame=f;captured=true;sourceHash=1;points.fill(0);rgba.fill(0);linearRgba.fill(0);srgbRgba.fill(0);
  return Status::OK;
}
}
int main() {
  CHECK(lifecycle_test_main() == 0);
  auto* component = reinterpret_cast<OH_NativeXComponent*>(1);
  void* window = reinterpret_cast<void*>(1);
  p013::g.component = component;
  p013::g.componentId = "test";
  p013::g.egl.OnWindowCreated(component, window);
  auto init = Call(p013::InitSession, {Text("test")});
  CHECK(Read(init, "code") == 0);
  const double session = Read(init, "sessionId");
  CHECK(Read(Call(p013::InitSession, {Text("other")}), "code") == 105);
  auto frame = Value();
  const char* fields[] = {"sessionId",
                          "frameId",
                          "snapshotId",
                          "surfaceEpoch",
                          "fieldEpoch",
                          "geometryEpoch",
                          "continuityEpoch",
                          "appearanceConfigRevision",
                          "lightConfigRevision",
                          "frameTimeNs",
                          "surfaceWidthPx",
                          "surfaceHeightPx",
                          "boxXPx",
                          "boxYPx",
                          "boxWidthPx",
                          "boxHeightPx",
                          "capacity",
                          "T_dataLayoutVersion",
                          "zOriginVersion"};
  for (auto* k : fields) Set(frame, k, 0);
  Set(frame, "sessionId", session);
  Set(frame, "frameId", 1);
  Set(frame, "surfaceEpoch", p013::g.egl.surfaceEpoch);
  Set(frame, "surfaceWidthPx", 64);
  Set(frame, "surfaceHeightPx", 64);
  Set(frame, "boxWidthPx", 64);
  Set(frame, "boxHeightPx", 64);
  Set(frame, "capacity", 192);
  Set(frame, "T_dataLayoutVersion", 1);
  Set(frame, "zOriginVersion", 1);
  frame->fields["slopeKernelVersion"] = Text("E2_APPROX_P0");
  napi_value b;
  napi_get_boolean(env, false, &b);
  frame->fields["visualInverted"] = b;
  napi_get_boolean(env, true, &b);
  frame->fields["diagnosticEnabled"] = b;
  auto light = Value();
  Set(light, "la", .85);
  Set(light, "ld", .15);
  Set(light, "lmin", .8);
  Set(light, "lmax", 1.12);
  Set(light, "lref", 1);
  Set(light, "ellLightMaterial", 24);
  Set(light, "lightX", -.55);
  Set(light, "lightY", -.55);
  Set(light, "lightZ", .63);
  Set(light, "sigmaCandidate", 6);
  auto buffer = Value();
  buffer->bytes.resize((512 + 8) * sizeof(float));
  auto view = Value();
  view->typed = true;
  view->buffer = buffer;
  view->length = 512;
  view->offset = 16;
  auto* data = reinterpret_cast<float*>(buffer->bytes.data() + view->offset);
  for (size_t i = 0; i < 128; i++) {
    data[i * 4] = 80 + static_cast<float>(i) * 0.125f;
    data[i * 4 + 1] = 160;
    data[i * 4 + 2] = 20;
    data[i * 4 + 3] = 40;
  }
  auto draw = [&] { return Call(p013::DrawFrame, {frame, view, light}); };
  // T41 root-cause regression: 512 Float32 elements report 2048 bytes.
  auto boundary = draw();
  CHECK(Read(boundary, "code") == 0);
  CHECK(Read(boundary, "nativeTypedByteLength") == 2048);
  CHECK(Read(boundary, "nativeTypedByteOffset") == 16);
  void* oversizedData = nullptr; const char* oversizedStage = "NONE";
  view->length = 2048;
  CHECK(!p013::TypedView(nullptr,view,napi_float32_array,512,oversizedData,oversizedStage));
  CHECK(std::string(oversizedStage) == "DUAL_TYPED_LENGTH");
  view->length = 512;
  CHECK(Read(boundary, "nativeBufferByteLength") == (512+8)*4);

  CHECK(gpuData[0] == 80 && gpuData[1] == 160 && gpuData[4] == 80.125f &&
        gpuData[508] == 95.875f);
  CHECK(!p013::g.egl.current);
  const auto before = gpuData;
  const auto uploads = p013::g.uploader.uploadCount;
  Set(frame, "frameId", 2);
  data[0] = std::numeric_limits<float>::quiet_NaN();
  CHECK(Read(draw(), "code") == 100);
  CHECK(Read(draw(), "code") == 102);
  CHECK(gpuData == before && p013::g.uploader.uploadCount == uploads);
  data[0] = 80;
  Set(frame, "frameId", 3);
  view->length = 511;
  CHECK(Read(draw(), "code") == 100);
  view->length = 512;
  Set(frame, "frameId", 4);
  buffer->detached = true;
  CHECK(Read(draw(), "code") == 100);
  buffer->detached = false;
  Set(frame, "frameId", 5);
  Set(frame, "T_dataLayoutVersion", 2);
  CHECK(Read(draw(), "code") == 101);
  Set(frame, "T_dataLayoutVersion", 1);
  Set(frame, "frameId", 6);
  Set(frame, "surfaceEpoch", 0);
  CHECK(Read(draw(), "code") == 103);
  Set(frame, "surfaceEpoch", p013::g.egl.surfaceEpoch);
  Set(frame, "frameId", 7);
  CHECK(Read(draw(), "code") == 0);
  Set(frame, "frameId", 8);
  auto wrong = 0.0;
  std::thread other([&] { wrong = Read(draw(), "code"); });
  other.join();
  CHECK(wrong == 104);
  CHECK(Read(draw(), "code") == 0);
  Set(frame, "frameId", 9);
  napi_get_boolean(env, false, &b);
  frame->fields["diagnosticEnabled"] = b;
  auto count = p013::g.uploader.uploadCount;
  auto off = draw();
  CHECK(Read(off, "code") == 0 && Read(off, "uploadedFrameId") == 0 &&
        Read(off, "submittedFrameId") == 9 &&
        p013::g.uploader.uploadCount == count);
  Set(frame, "frameId", 10);
  napi_get_boolean(env, true, &b);
  frame->fields["diagnosticEnabled"] = b;
  fail = 9;
  injectedError = EGL_BAD_ALLOC;
  auto failedSwap = draw();
  CHECK(Read(failedSwap, "code") == 303 &&
        !failedSwap->fields.at("swapSucceeded")->boolean &&
        Read(failedSwap, "lastSwapFrameId") == 9);
  fail = 0;
  auto batch = Call(p013::Drain, {});
  CHECK(batch->fields.at("records")->elements.size() >= 11);
  CHECK(Read(batch, "droppedTelemetry") == 0);
  CHECK(Read(Call(p013::Release, {}), "code") == 0);
  CHECK(Read(Call(p013::Release, {}), "code") == 0);
  auto state = Call(p013::GetState, {});
  CHECK(Read(state, "nativeOwnedBytes") == 0 &&
        Read(state, "textureLiveCount") == 0 &&
        Read(state, "programLiveCount") == 0);
  init = Call(p013::InitSession, {Text("test")});
  CHECK(Read(init, "code") == 0 && Read(init, "sessionId") > session);
  Set(frame, "sessionId", Read(init, "sessionId"));
  for (int i = 1; i <= 2051; i++) {
    Set(frame, "frameId", i);
    CHECK(Read(draw(), "code") == 0);
  }
  batch = Call(p013::Drain, {});
  CHECK(batch->fields.at("records")->elements.size() == 2048 &&
        Read(batch, "droppedTelemetry") == 3);
  Set(frame, "frameId", 2052);
  view->arrayType = napi_float64_array;
  CHECK(Read(draw(), "code") == 100);
  view->arrayType = napi_float32_array;
  Set(frame, "frameId", 2053);
  view->offset = 36;
  CHECK(Read(draw(), "code") == 100);
  view->offset = 16;
  Set(frame, "frameId", 2054);
  frame->fields["slopeKernelVersion"] = Text("WRONG_KERNEL");
  CHECK(Read(draw(), "code") == 101);
  frame->fields["slopeKernelVersion"] = Text("E2_APPROX_P0");
  Set(frame, "frameId", 2055);
  Set(light, "lref", 2);
  CHECK(Read(draw(), "code") == 100);
  Set(light, "lref", 1);
  Set(frame, "frameId", 2056);
  CHECK(Read(draw(), "code") == 0);
  // The G0 source packet has the correct names/kernel/512-float layout.
  // Validate its actual geometry/profiles independently of the host's 64x64 EGL surface.
  p013::FrameInput probe{};
  p013::LightParams probeLight{};
  const char* stage = "NONE";
  CHECK(p013::ParseFrame(env, frame, probe, stage));
  CHECK(p013::ParseLight(env, light, probeLight, stage));
  probe.surfaceWidthPx = 1280; probe.surfaceHeightPx = 2598;
  probe.boxXPx = 352; probe.boxYPx = 649;
  probe.boxWidthPx = 576; probe.boxHeightPx = 779;
  std::array<float, p013::kFloats> probeData{};
  for (size_t c = 0; c < 128; c++) {
    probeData[4*c] = 96; probeData[4*c+1] = 172;
  }
  CHECK(p013::Validate(probe, probeLight, probeData) == p013::Status::OK);
  // Each parse field is checked using the actual production bridge, with no GPU upload on rejection.
  int nextId = 2057;
  for (const auto* key : fields) {
    // Lifecycle identity is intentionally resolved before payload validation.
    if (std::string(key) == "sessionId" || std::string(key) == "frameId") continue;
    auto saved = frame->fields.at(key);
    Set(frame, "frameId", nextId++);
    frame->fields[key] = Text("wrong type");
    auto rejected = draw();
    CHECK(Read(rejected, "code") == 100);
    CHECK(rejected->fields.at("errorStage")->text == std::string("FRAME_FIELD_") + key);
    frame->fields[key] = saved;
  }
  for (const auto* key : {"visualInverted", "diagnosticEnabled"}) {
    auto saved = frame->fields.at(key);
    Set(frame, "frameId", nextId++);
    Set(frame, key, 1);
    auto rejected = draw();
    CHECK(Read(rejected, "code") == 100);
    CHECK(rejected->fields.at("errorStage")->text == std::string("FRAME_FIELD_") + key);
    frame->fields[key] = saved;
  }
  auto savedKernel = frame->fields.at("slopeKernelVersion");
  frame->fields.erase("slopeKernelVersion");
  Set(frame, "frameId", nextId++);
  auto missingKernel = draw();
  CHECK(Read(missingKernel, "code") == 100);
  CHECK(missingKernel->fields.at("errorStage")->text == "SLOPE_KERNEL_FIELD");
  frame->fields["slopeKernelVersion"] = savedKernel;
  const char* lightKeys[] = {"la", "ld", "lmin", "lmax", "lref", "ellLightMaterial", "lightX", "lightY", "lightZ", "sigmaCandidate"};
  for (const auto* key : lightKeys) {
    auto saved = light->fields.at(key);
    Set(frame, "frameId", nextId++);
    light->fields.erase(key);
    auto rejected = draw();
    CHECK(Read(rejected, "code") == 100);
    CHECK(rejected->fields.at("errorStage")->text == std::string("LIGHT_FIELD_") + key);
    light->fields[key] = saved;
  }
  Set(frame, "frameId", nextId++);
  view->length = 511;
  CHECK(draw()->fields.at("errorStage")->text == "TDATA_LENGTH");
  view->length = 512;
  Set(frame, "frameId", nextId++);
  buffer->detached = true;
  CHECK(draw()->fields.at("errorStage")->text == "TDATA_DETACHED");
  buffer->detached = false;
  Set(frame, "frameId", nextId++);
  view->arrayType = napi_float64_array;
  CHECK(draw()->fields.at("errorStage")->text == "TDATA_TYPE");
  view->arrayType = napi_float32_array;
  Set(frame, "frameId", nextId++);
  view->offset = 36;
  CHECK(draw()->fields.at("errorStage")->text == "TDATA_BUFFER_RANGE");
  view->offset = 16;
  Set(frame, "frameId", nextId++);
  frame->fields["slopeKernelVersion"] = Text("E2_APROX_P0");
  auto wrongKernel = draw();
  CHECK(Read(wrongKernel, "code") == 101);
  CHECK(wrongKernel->fields.at("errorStage")->text == "SLOPE_KERNEL_VERSION");
  frame->fields["slopeKernelVersion"] = Text("E2_APPROX_P0");
  Set(frame, "frameId", nextId++);
  CHECK(Read(draw(), "code") == 0);
  Set(frame, "frameId", nextId++);
  uploadFailure = true;
  CHECK(Read(draw(), "code") == 301);
  uploadFailure = false;
  // New profile is explicitly probe-only and isolated from P0.
  auto profile=Value();
  profile->fields["compositionRevision"]=Text("P013_P1_TEXTURE_STACK_V1");
  profile->fields["componentType"]=Text("TEXTURE");
  profile->fields["colorPipelineVersion"]=Text("LIN_SRGB_REINHARD_V1");
  profile->fields["alphaMode"]=Text("BINARY_MASK");profile->fields["hitMode"]=Text("NONE");
  napi_get_boolean(env,false,&b);profile->fields["probeOnly"]=b;
  CHECK(Read(Call(p013::RequestComposition,{profile}),"code")==1 && !p013::g.egl.composition);
  napi_get_boolean(env,true,&b);profile->fields["probeOnly"]=b;
  profile->fields["alphaMode"]=Text("FRACTIONAL");
  CHECK(Read(Call(p013::RequestComposition,{profile}),"code")==100);
  profile->fields["alphaMode"]=Text("BINARY_MASK");
  auto requested=Call(p013::RequestComposition,{profile});
  CHECK(Read(requested,"code")==0 && p013::g.egl.composition);
  const auto profileEpoch=p013::g.egl.surfaceEpoch;
  CHECK(Read(Call(p013::RequestComposition,{profile}),"code")==0 && p013::g.egl.surfaceEpoch==profileEpoch);
  Set(frame,"surfaceEpoch",profileEpoch);Set(frame,"frameId",nextId++);
  CHECK(Read(draw(),"code")==0);
  auto receipt=Call(p013::QueryComposition,{});
  CHECK(receipt->fields.at("profileStatus")->text=="READY");
  CHECK(receipt->fields.at("requestedColorSpace")->text=="SRGB");
  CHECK(receipt->fields.at("attachmentEncoding")->text=="GL_SRGB");
  auto probeBatch=Call(p013::Drain,{});
  auto lastProbe=probeBatch->fields.at("records")->elements.back();
  CHECK(lastProbe->fields.at("mode")->text=="G0_COLOR_PROBE_ON");
  CHECK(lastProbe->fields.at("diagEncoding")->text=="G0_LINEAR_COLOR_BLOCKS_V1");
  CHECK(lastProbe->fields.at("compositionRevision")->text=="P013_P1_TEXTURE_STACK_V1");
  auto token=receipt->fields.at("frameToken");
  view->length=12;std::memset(data,0,12*sizeof(float));
  CHECK(Read(Call(p013::CaptureProbe,{token,view}),"code")==0);
  CHECK(Read(Call(p013::CaptureProbe,{Text("old-token"),view}),"code")==102);
  data[0]=1;CHECK(Read(Call(p013::CaptureProbe,{token,view}),"code")==100);data[0]=0;
  p013::Changed(component,window);
  CHECK(Read(Call(p013::CaptureProbe,{token,view}),"code")==102);
  view->length=512;
  Call(p013::Release, {});
  CHECK(!p013::g.egl.composition);
  CHECK(Read(Call(p013::InitSession,{Text("test")}),"code")==0);
  CHECK(Call(p013::QueryComposition,{})->fields.at("profileStatus")->text=="P0_LEGACY");
  Call(p013::Release, {});
  CHECK(contexts == 0 && surfaces == 0);
  // Separate dual ABI: valid offset views, full-capacity objects and effective prefix.
  init=Call(p013::InitSession,{Text("test")});Set(frame,"sessionId",Read(init,"sessionId"));
  Set(frame,"frameId",1);Set(frame,"surfaceEpoch",p013::g.egl.surfaceEpoch);
  frame->fields["dualFieldPacketRevision"]=Text("P013_DUALFIELD_PACKET_V1");
  frame->fields["particleLayoutVersion"]=Text("PARTICLE_XY_META_SOA_V1");
  frame->fields["renderPipelineId"]=Text("DUALFIELD_BASIC_V1");
  frame->fields["colorPipelineVersion"]=Text("BASIC_SRGB_DISPLAY_V1");
  frame->fields["runId"]=Text("run-dual");Set(frame,"stateEpoch",1);Set(frame,"physicsCommitId",1);Set(frame,"contextEpoch",p013::g.egl.contextEpoch);
  Set(frame,"particleCount",1);Set(frame,"particleCapacity",4);
  napi_get_boolean(env,true,&b);frame->fields["particleLayerEnabled"]=b;
  auto makeView=[&](napi_typedarray_type type){auto buf=Value();buf->bytes.resize(16+8*4);auto v=Value();v->typed=true;v->arrayType=type;v->length=8;v->offset=16;v->buffer=buf;return v;};
  auto xy=makeView(napi_float32_array),meta=makeView(napi_uint32_array);
  auto* positions=reinterpret_cast<float*>(xy->buffer->bytes.data()+16);positions[0]=64.5f;positions[1]=100;
  auto* tags=reinterpret_cast<uint32_t*>(meta->buffer->bytes.data()+16);tags[0]=512;tags[1]=17;
  auto visual=Value();visual->fields["visualRevision"]=Text("P012_EXISTING_PHASE_CANDIDATES_V1");Set(visual,"radiusPx",1);
  for(auto key:{"heavyR","heavyG","heavyB","lightR","lightG","lightB","particleHeavyR","particleHeavyG","particleHeavyB","particleLightR","particleLightG","particleLightB"})Set(visual,key,.5);
  for(size_t c=0;c<128;c++){data[4*c]=80;data[4*c+1]=160;data[4*c+2]=20;data[4*c+3]=40;}
  auto dualDraw=[&](){return Call(p013::DrawDualFieldFrame,{frame,view,xy,meta,visual});};
  CHECK(Read(dualDraw(),"code")==103); // switching profile creates a new surface generation
  Set(frame,"surfaceEpoch",p013::g.egl.surfaceEpoch);Set(frame,"contextEpoch",p013::g.egl.contextEpoch);Set(frame,"frameId",2);
  auto dualGood=dualDraw();CHECK(Read(dualGood,"code")==0 && Read(dualGood,"particleUploadBytes")==16);
  Set(frame,"contextEpoch",p013::g.egl.contextEpoch);
  CHECK(p013::g.dualXY[0]==64.5f && p013::g.dualMeta[0]==512);positions[0]=10;
  CHECK(p013::g.dualXY[0]==64.5f); // no borrowed JS pointer retained
  int dualId=3;
  auto rejects=[&](const char* reason){Set(frame,"frameId",dualId++);auto r=dualDraw();return Read(r,"code")==100&&r->fields.at("errorStage")->text==reason;};
  xy->arrayType=napi_float64_array;CHECK(rejects("DUAL_TYPED_TYPE"));xy->arrayType=napi_float32_array;
  xy->length=6;CHECK(rejects("DUAL_TYPED_LENGTH"));xy->length=8;
  xy->buffer->detached=true;CHECK(rejects("DUAL_BUFFER_DETACHED"));xy->buffer->detached=false;
  xy->offset=20;CHECK(rejects("DUAL_BUFFER_RANGE"));xy->offset=16;
  tags[0]=768;CHECK(rejects("DUAL_PARTICLE_VALUE"));tags[0]=512;
  positions[0]=std::numeric_limits<float>::infinity();CHECK(rejects("DUAL_PARTICLE_VALUE"));positions[0]=64.5f;
  frame->fields["particleLayoutVersion"]=Text("old");CHECK(rejects("DUAL_PACKET_VERSION"));frame->fields["particleLayoutVersion"]=Text("PARTICLE_XY_META_SOA_V1");
  Set(frame,"particleCount",0);Set(frame,"frameId",dualId++);CHECK(Read(dualDraw(),"code")==0);
  Set(frame,"contextEpoch",p013::g.egl.contextEpoch+1);Set(frame,"frameId",dualId++);
  CHECK(Read(dualDraw(),"code")==103);Set(frame,"contextEpoch",p013::g.egl.contextEpoch);
  Set(frame,"physicsCommitId",0);CHECK(rejects("DUAL_PUBLICATION"));Set(frame,"physicsCommitId",1);
  Call(p013::Release,{});CHECK(contexts==0&&surfaces==0);
  std::puts(
      "PASS P013 actual bridge: offset Float32Array, "
      "length/detach/NaN/version/stale rejection, consumed IDs, wrong owner, "
      "no half upload, transparent OFF, swap failure, ring/drop, "
      "release/reinit");
  return 0;
}
