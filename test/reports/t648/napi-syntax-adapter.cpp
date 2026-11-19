// Compile-only legacy include adapter. No executable, no fake product evidence.
#include "g9dm/controller.h"
#include "cold_trace.h"
#include "napi_words_contract.h"
#include <condition_variable>
#include <memory>
#include <string>
struct OtherController {void Stop();};
struct LegacyAppCompileSurface {
 std::mutex mutex;std::condition_variable wake;grs::cold503::TraceManager coldTrace;
 std::shared_ptr<OtherController> m3Controller;bool m3Selected=false,requiresReset=false;
 uint32_t K=0,Kw=0;
 #include "g9dm/bridge_fields.inc"
 void InvalidateLocked();
};
LegacyAppCompileSurface& app();
napi_value text(napi_env,const std::string&);uint64_t now();
#include "g9dm/napi.inc"
