// T550 strict input decode contract (problem T546, architecture v1 S2-S3).
// Shared per-stage observation decoder for M3A/M3B/G9 typedarray inputs.
// All three routes delegate here; the old opaque one-short-circuit bool is
// replaced by ordered stage observations with first-failure locking.
// No N/type/offset/bytes/range semantics are changed by this contract -
// the acceptance predicate is bit-identical to the previous m3Words/m5Words;
// only the diagnostic observation layer is new.
#ifndef GRS_NAPI_WORDS_CONTRACT_H
#define GRS_NAPI_WORDS_CONTRACT_H
#include <napi/native_api.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
namespace grs::words {

enum class Route : uint8_t { M3A = 0, M3B = 1, G9 = 2 };
inline const char* RouteName(Route r) {
  switch (r) { case Route::M3A: return "M3A"; case Route::M3B: return "M3B"; case Route::G9: return "G9"; }
  return "UNKNOWN";
}
enum class Stage : uint8_t {
  CB_INFO = 1, IS_TYPEDARRAY, TYPEDARRAY_INFO, ELEMENT_TYPE, ELEMENT_COUNT,
  BYTE_OFFSET, DATA_POINTER, DETACHED_API, DETACHED_VALUE, BACKING_INFO,
  BASE_POINTER, BACKING_BYTES, WORD_COPY, API_SEMANTIC
};
inline const char* StageName(Stage s) {
  switch (s) {
    case Stage::CB_INFO: return "CB_INFO"; case Stage::IS_TYPEDARRAY: return "IS_TYPEDARRAY";
    case Stage::TYPEDARRAY_INFO: return "TYPEDARRAY_INFO"; case Stage::ELEMENT_TYPE: return "ELEMENT_TYPE";
    case Stage::ELEMENT_COUNT: return "ELEMENT_COUNT"; case Stage::BYTE_OFFSET: return "BYTE_OFFSET";
    case Stage::DATA_POINTER: return "DATA_POINTER"; case Stage::DETACHED_API: return "DETACHED_API";
    case Stage::DETACHED_VALUE: return "DETACHED_VALUE"; case Stage::BACKING_INFO: return "BACKING_INFO";
    case Stage::BASE_POINTER: return "BASE_POINTER"; case Stage::BACKING_BYTES: return "BACKING_BYTES";
    case Stage::WORD_COPY: return "WORD_COPY"; case Stage::API_SEMANTIC: return "API_SEMANTIC";
  }
  return "UNKNOWN";
}
// Tri-state output qualification (T546 S2: VALID / API_FAILED / NOT_EVALUATED).
enum class Tri : uint8_t { NOT_EVALUATED = 0, VALID = 1, API_FAILED = 2 };

struct Observation {
  // Raw statuses in call order; -1 = NOT_EVALUATED, >=0 = raw napi status.
  int32_t cbInfoStatus = -1;
  int32_t isTypedarrayStatus = -1;
  int32_t typedarrayInfoStatus = -1;
  int32_t isDetachedStatus = -1;
  int32_t arraybufferInfoStatus = -1;
  // Tri-state values - only meaningful after their stage ran.
  Tri typed = Tri::NOT_EVALUATED;
  Tri detached = Tri::NOT_EVALUATED;
  // TYPEDARRAY_INFO outputs (valid only when typedarrayInfoStatus == napi_ok).
  int32_t elementKind = -1;      // napi_typedarray_type raw value
  uint64_t elementLength = 0;    // SDK: length is in ELEMENTS
  int64_t byteOffset = -1;
  bool dataPresent = false;
  // BACKING_INFO outputs.
  bool basePresent = false;
  bool baseEqualsData = false;
  uint64_t backingBytes = 0;
  // CB_INFO argc observation.
  uint64_t argc = 0;
  bool copyComplete = false;
};

template <size_t N>
struct ReadWordsResult {
  std::array<uint32_t, N> words{};
  Observation observed{};
  Stage firstFailedStage = Stage::CB_INFO;  // valid when ok == false
  bool ok = false;
  bool argcObserved = false;
};

// ---------------- T550 S3 diagnostic snapshots (512B per snapshot) --------
struct Snapshot {
  static constexpr size_t kRawBytes = 512;
  bool used = false;
  uint64_t attemptId = 0;
  uint64_t bootStamp = 0;
  uint64_t diagGeneration = 1;
  Route route = Route::M3A;
  size_t expectedN = 0;
  Stage firstFailedStage = Stage::CB_INFO;
  bool copyComplete = false;
  Observation observed{};
  std::array<uint32_t, 16> configHeader{};  // first 16 words when copied
  std::array<char, kRawBytes> raw{};
  uint16_t rawBytesUsed = 0;

  void Clear() { used = false; rawBytesUsed = 0; std::memset(raw.data(), 0, raw.size()); }
  // Returns false when the 512B budget is exhausted (writer.Good() == false).
  template <size_t N>
  bool Build(const ReadWordsResult<N>& r, Route route, uint64_t attemptId,
             uint64_t bootStamp, uint64_t diagGeneration, uint64_t attemptCounter) {
    used = true; this->route = route; this->attemptId = attemptId;
    this->bootStamp = bootStamp; this->diagGeneration = diagGeneration;
    expectedN = N; firstFailedStage = r.firstFailedStage;
    copyComplete = r.observed.copyComplete; observed = r.observed;
    if (r.observed.copyComplete) {
      constexpr size_t header = N < 16 ? N : 16;
      for (size_t i = 0; i < header; ++i) configHeader[i] = r.words[i];
    } else {
      configHeader.fill(0);
    }
    (void)attemptCounter;
    return JsonEncode();
  }
  bool JsonEncode();
};

struct RouteLedger {
  std::mutex mutex;
  Snapshot lastAttempt{};
  Snapshot firstFailure{};
  uint64_t attemptCounter = 0;      // monotonic attempt ids
  uint64_t firstFailureAttempt = 0; // attemptId owning the firstFailure slot
  bool firstFailureLatched = false; // first failure of this generation
};

struct DiagnosticStore {
  std::mutex mutex;                 // small lock, never taken under App lock
  std::array<RouteLedger, 3> routes{};
  uint64_t bootStamp = 0;
  uint64_t diagGeneration = 1;

  static DiagnosticStore& Instance() { static DiagnosticStore store; return store; }
  void BeginGeneration(uint64_t stamp) {
    std::lock_guard<std::mutex> lock(mutex);
    if (bootStamp == 0) bootStamp = stamp;
    ++diagGeneration;
    for (auto& route : routes) {
      std::lock_guard<std::mutex> routeLock(route.mutex);
      route.lastAttempt.Clear();
      route.firstFailure.Clear();
      route.attemptCounter = 0;
      route.firstFailureAttempt = 0;
      route.firstFailureLatched = false;
    }
  }
  // Called with the App lock NOT held (App mutation happens before/after, the
  // diagnostic record itself never under the App lock - T546 S3 lock order).
  template <size_t N>
  uint64_t Record(Route route, const ReadWordsResult<N>& result, uint64_t nowStamp) {
    auto& ledger = routes[static_cast<size_t>(route)];
    std::lock_guard<std::mutex> lock(ledger.mutex);
    const uint64_t attemptId = ++ledger.attemptCounter;
    ledger.lastAttempt.Build(result, route, attemptId, bootStamp, diagGeneration, attemptId);
    if (!result.ok && !ledger.firstFailureLatched) {
      ledger.firstFailureLatched = true;
      ledger.firstFailureAttempt = attemptId;
      ledger.firstFailure.Build(result, route, attemptId, bootStamp, diagGeneration, attemptId);
    }
    (void)nowStamp;
    return attemptId;
  }
};

// One decode attempt: allocates a fresh attemptId and runs the ordered stages.
template <size_t N>
uint64_t BeginInputCall(Route route) {
  auto& store = DiagnosticStore::Instance();
  auto& ledger = store.routes[static_cast<size_t>(route)];
  std::lock_guard<std::mutex> lock(ledger.mutex);
  return ++ledger.attemptCounter;
}

template <size_t N>
ReadWordsResult<N> ReadWords(napi_env env, napi_callback_info info, Route route, uint64_t attemptId) {
  ReadWordsResult<N> result{};
  Observation& o = result.observed;
  // Stage 1: CB_INFO / ARG_COUNT - read capacity 2 to detect extra args
  // without treating a capacity-1 truncation as proof of argc==1.
  napi_value argv[2] = {nullptr, nullptr};
  size_t argc = 2;
  const napi_status cbInfo = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  o.cbInfoStatus = static_cast<int32_t>(cbInfo);
  o.argc = argc;
  if (cbInfo != napi_ok) { result.firstFailedStage = Stage::CB_INFO; return result; }
  if (argc != 1) { result.firstFailedStage = Stage::CB_INFO; return result; }
  // Stage 2: IS_TYPEDARRAY - no implicit conversion from number[]/DataView/etc.
  bool typed = false;
  const napi_status isTyped = napi_is_typedarray(env, argv[0], &typed);
  o.isTypedarrayStatus = static_cast<int32_t>(isTyped);
  if (isTyped != napi_ok) { result.firstFailedStage = Stage::IS_TYPEDARRAY; return result; }
  o.typed = typed ? Tri::VALID : Tri::VALID;  // API succeeded; value below
  if (!typed) { o.typed = Tri::VALID; result.firstFailedStage = Stage::IS_TYPEDARRAY; return result; }
  // Stage 3: TYPEDARRAY_INFO - outputs only read after API success.
  napi_typedarray_type kind = static_cast<napi_typedarray_type>(-1);
  size_t length = 0;
  void* data = nullptr;
  napi_value backing = nullptr;
  size_t offset = 0;
  const napi_status infoStatus = napi_get_typedarray_info(env, argv[0], &kind, &length, &data, &backing, &offset);
  o.typedarrayInfoStatus = static_cast<int32_t>(infoStatus);
  if (infoStatus != napi_ok) { result.firstFailedStage = Stage::TYPEDARRAY_INFO; return result; }
  o.elementKind = static_cast<int32_t>(kind);
  o.elementLength = length;
  o.byteOffset = static_cast<int64_t>(offset);
  o.dataPresent = data != nullptr;
  // Stage 4: ELEMENT_TYPE - must be exactly napi_uint32_array.
  if (kind != napi_uint32_array) { result.firstFailedStage = Stage::ELEMENT_TYPE; return result; }
  // Stage 4b: ELEMENT_COUNT - length is in ELEMENTS (SDK native_api.h), never bytes.
  if (length != N) { result.firstFailedStage = Stage::ELEMENT_COUNT; return result; }
  // Stage 5: BYTE_OFFSET / DATA_POINTER - zero offset, external data present.
  if (offset != 0) { result.firstFailedStage = Stage::BYTE_OFFSET; return result; }
  if (data == nullptr) { result.firstFailedStage = Stage::DATA_POINTER; return result; }
  // Stage 6: DETACHED_API / DETACHED_VALUE - unsupported/invalid stay named rejections.
  bool detached = false;
  const napi_status detachedStatus = napi_is_detached_arraybuffer(env, backing, &detached);
  o.isDetachedStatus = static_cast<int32_t>(detachedStatus);
  if (detachedStatus != napi_ok) { result.firstFailedStage = Stage::DETACHED_API; return result; }
  o.detached = detached ? Tri::VALID : Tri::VALID;
  if (detached) { result.firstFailedStage = Stage::DETACHED_VALUE; return result; }
  // Stage 7: BACKING_INFO.
  void* base = nullptr;
  size_t bytes = 0;
  const napi_status backingStatus = napi_get_arraybuffer_info(env, backing, &base, &bytes);
  o.arraybufferInfoStatus = static_cast<int32_t>(backingStatus);
  if (backingStatus != napi_ok) { result.firstFailedStage = Stage::BACKING_INFO; return result; }
  o.basePresent = base != nullptr;
  o.baseEqualsData = base == data;
  o.backingBytes = bytes;
  // Stage 8: BASE_POINTER / BACKING_BYTES - base==data and bytes==N*4.
  if (base != data) { result.firstFailedStage = Stage::BASE_POINTER; return result; }
  if (bytes != N * 4) { result.firstFailedStage = Stage::BACKING_BYTES; return result; }
  // Stage 9: WORD_COPY - synchronous copy into the fixed native array; no JS
  // pointer is retained across the NAPI return / async owner.
  std::memcpy(result.words.data(), data, N * 4);
  o.copyComplete = true;
  result.ok = true;
  result.firstFailedStage = Stage::WORD_COPY;  // not failed; ok == true
  auto& store = DiagnosticStore::Instance();
  store.template Record<N>(route, result, 0);
  (void)attemptId;
  return result;
}

// Bounded JSON snapshot writer for the gpuInputDiagnosticsV1 cold endpoint.
bool SnapshotJson(const std::string& routeText, const std::string& attemptText, std::string& out);

}  // namespace grs::words
#endif
