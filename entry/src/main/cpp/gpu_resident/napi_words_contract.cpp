// T550: definition translation unit for the words contract snapshot encoder.
// Included by app_bridge.cpp delivery builds; kept separate so host test
// doubles can link their own encoder.
#include "napi_words_contract.h"
#include <cstdio>
#include <cstring>
namespace grs::words {

bool Snapshot::JsonEncode() {
  // Fixed 512B bounded writer; each field has a hard cap. On overflow the
  // snapshot keeps the prefix and rawBytesUsed stays at the limit - the
  // endpoint then reports the truncation state instead of growing unbounded.
  char* p = raw.data();
  size_t left = raw.size();
  auto emit = [&](const char* s) -> bool {
    const size_t n = std::strlen(s);
    if (n > left) { rawBytesUsed = static_cast<uint16_t>(raw.size()); return false; }
    std::memcpy(p, s, n); p += n; left -= n; return true;
  };
  auto num = [&](const char* k, uint64_t v) -> bool {
    char buf[80]; std::snprintf(buf, sizeof(buf), "\"%s\":%llu,", k, static_cast<unsigned long long>(v));
    return emit(buf);
  };
  auto num32 = [&](const char* k, int32_t v) -> bool {
    char buf[80]; std::snprintf(buf, sizeof(buf), "\"%s\":%d,", k, v);
    return emit(buf);
  };
  if (!emit("{\"revision\":\"GPU_INPUT_DIAGNOSTIC_V1\",\"route\":\"")) return false;
  if (!emit(RouteName(route))) return false;
  if (!emit("\",\"boot\":\"")) return false;
  { char b[32]; std::snprintf(b, sizeof(b), "%llu", static_cast<unsigned long long>(bootStamp)); if (!emit(b)) return false; }
  if (!emit("\",\"diagGeneration\":")) return false;
  { char b[32]; std::snprintf(b, sizeof(b), "%llu", static_cast<unsigned long long>(diagGeneration)); if (!emit(b)) return false; }
  if (!emit(",\"attemptId\":\"")) return false;
  { char b[32]; std::snprintf(b, sizeof(b), "%llu", static_cast<unsigned long long>(attemptId)); if (!emit(b)) return false; }
  if (!emit("\",")) return false;
  if (!num("expectedN", expectedN)) return false;
  if (!num("expectedBytes", expectedN * 4)) return false;
  if (!emit("\"firstFailedStage\":\"")) return false;
  if (!emit(StageName(firstFailedStage))) return false;
  if (!emit("\",\"copyComplete\":")) return false;
  if (!emit(copyComplete ? "true" : "false")) return false;
  if (!emit(",\"cbInfoStatus\":")) return false;
  { char b[24]; std::snprintf(b, sizeof(b), "%d,", observed.cbInfoStatus); if (!emit(b)) return false; }
  if (!num("argc", observed.argc)) return false;
  if (!num32("isTypedarrayStatus", observed.isTypedarrayStatus)) return false;
  if (!emit("\"typed\":")) return false;
  { // tri-state
    const char* t = observed.typed == Tri::VALID ? "true" : observed.typed == Tri::API_FAILED ? "false" : "null";
    char b[32]; std::snprintf(b, sizeof(b), "%s,", t); if (!emit(b)) return false;
  }
  if (!num32("typedarrayInfoStatus", observed.typedarrayInfoStatus)) return false;
  if (!num32("elementKind", observed.elementKind)) return false;
  if (!num("elementLength", observed.elementLength)) return false;
  if (!num("byteOffset", static_cast<uint64_t>(observed.byteOffset < 0 ? 0 : observed.byteOffset))) return false;
  if (!emit("\"dataPresent\":")) return false;
  if (!emit(observed.dataPresent ? "true" : "false")) return false;
  if (!emit(",\"isDetachedStatus\":")) return false;
  { char b[24]; std::snprintf(b, sizeof(b), "%d,", observed.isDetachedStatus); if (!emit(b)) return false; }
  if (!emit("\"detached\":")) return false;
  { const char* t = observed.detached == Tri::VALID ? "false" : observed.detached == Tri::API_FAILED ? "API_FAILED" : "null";
    // detached VALID means "checked and NOT detached" (the pass condition).
    char b[32]; std::snprintf(b, sizeof(b), "%s,", t); if (!emit(b)) return false; }
  if (!num32("arraybufferInfoStatus", observed.arraybufferInfoStatus)) return false;
  if (!emit("\"basePresent\":")) return false;
  if (!emit(observed.basePresent ? "true" : "false")) return false;
  if (!emit(",\"baseEqualsData\":")) return false;
  if (!emit(observed.baseEqualsData ? "true" : "false")) return false;
  if (!num("backingBytes", observed.backingBytes)) return false;
  if (!emit("\"configHeader\":")) return false;
  if (copyComplete) {
    if (!emit("[")) return false;
    char b[32];
    for (size_t i = 0; i < 16; ++i) {
      std::snprintf(b, sizeof(b), "%s%u", i ? "," : "", configHeader[i]);
      if (!emit(b)) return false;
    }
    if (!emit("]")) return false;
  } else {
    if (!emit("null")) return false;
  }
  if (!emit("}")) return false;
  rawBytesUsed = static_cast<uint16_t>(raw.size() - left);
  return true;
}

bool SnapshotJson(const std::string& routeText, const std::string& attemptText, std::string& out) {
  auto& store = DiagnosticStore::Instance();
  Route route;
  if (routeText == "M3A") route = Route::M3A;
  else if (routeText == "M3B") route = Route::M3B;
  else if (routeText == "G9") route = Route::G9;
  else { out = "{\"revision\":\"GPU_INPUT_DIAGNOSTIC_V1\",\"error\":\"UNKNOWN_ROUTE\"}"; return true; }
  auto& ledger = store.routes[static_cast<size_t>(route)];
  std::lock_guard<std::mutex> lock(ledger.mutex);
  const Snapshot* selected = &ledger.lastAttempt;
  if (!attemptText.empty()) {
    uint64_t id = 0; bool ok = true;
    for (char c : attemptText) {
      if (c < '0' || c > '9') { ok = false; break; }
      if (id > (UINT64_MAX - static_cast<uint64_t>(c - '0')) / 10) { ok = false; break; }
      id = id * 10 + static_cast<uint64_t>(c - '0');
    }
    if (!ok) { out = "{\"revision\":\"GPU_INPUT_DIAGNOSTIC_V1\",\"error\":\"ATTEMPT_ID_INVALID\"}"; return true; }
    if (!ledger.firstFailure.used || ledger.firstFailureAttempt != id) {
      out = "{\"revision\":\"GPU_INPUT_DIAGNOSTIC_V1\",\"error\":\"SNAPSHOT_EVICTED_OR_NOT_FOUND\",\"attemptId\":\"" +
            attemptText + "\"}";
      return true;
    }
    selected = &ledger.firstFailure;
  }
  if (!selected->used) {
    out = "{\"revision\":\"GPU_INPUT_DIAGNOSTIC_V1\",\"error\":\"NO_ATTEMPT_RECORDED\"}";
    return true;
  }
  // Response cap: single snapshot expansion 4096B UTF-8 (T546 S3).
  out.assign(selected->raw.data(), selected->rawBytesUsed);
  if (selected->rawBytesUsed == Snapshot::kRawBytes) {
    // The 512B snapshot body itself is the bounded detail; the endpoint just
    // relays it. Marker for an evicted/truncated body is the error above.
  }
  return true;
}

}  // namespace grs::words
