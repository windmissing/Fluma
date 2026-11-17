napi_value recordInputStageV1(napi_env e,napi_callback_info info){
 size_t n=2;napi_value args[2];double code=0;size_t bytes=0;
 if(napi_get_cb_info(e,info,&n,args,nullptr,nullptr)!=napi_ok||n!=2||napi_get_value_double(e,args[0],&code)!=napi_ok||!std::isfinite(code)||std::floor(code)!=code||code<1||code>9||napi_get_value_string_utf8(e,args[1],nullptr,0,&bytes)!=napi_ok)return text(e,"INVALID_INPUT_STAGE");
 auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);if(bytes>=440){a.inputStages.Add(uint16_t(code),a.DiagnosticPointLocked("T537_INPUT_STAGE"),nullptr,bytes);return text(e,"INPUT_STAGE_DROPPED");}
 std::array<char,440> raw{};size_t copied=0;if(napi_get_value_string_utf8(e,args[1],raw.data(),raw.size(),&copied)!=napi_ok||copied!=bytes)return text(e,"INVALID_INPUT_STAGE");
 const auto before=a.inputStages.dropped;a.inputStages.Add(uint16_t(code),a.DiagnosticPointLocked("T537_INPUT_STAGE"),raw.data(),bytes);
 a.resources->HostStorage("T537_INPUT_STAGE_LEDGER",reinterpret_cast<uintptr_t>(&a.inputStages),sizeof(a.inputStages),sizeof(a.inputStages));return text(e,before==a.inputStages.dropped?"INPUT_STAGE_RECORDED":"INPUT_STAGE_DROPPED");
}
napi_value pixelDiagnosticStart(napi_env e,napi_callback_info info){
 size_t n=1;napi_value arg;size_t bytes=0;if(napi_get_cb_info(e,info,&n,&arg,nullptr,nullptr)!=napi_ok||n!=1||napi_get_value_string_utf8(e,arg,nullptr,0,&bytes)!=napi_ok||!bytes||bytes>8192)return text(e,"INVALID_DIAGNOSTIC_GEOMETRY");
 std::string geometry(bytes+1,0);size_t copied=0;if(napi_get_value_string_utf8(e,arg,geometry.data(),geometry.size(),&copied)!=napi_ok||copied!=bytes)return text(e,"INVALID_DIAGNOSTIC_GEOMETRY");geometry.resize(bytes);
 auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);if(!a.active||!a.window||a.epoch==UINT64_MAX)return text(e,"DIAGNOSTIC_WINDOW_REQUIRED");
 a.pixelSelected=true;a.pixelPulseOpen=false;a.pixelFailed=false;a.pixelDrawn=false;a.pixelLastNative=a.pixelLastUi=0;a.pixel={};a.pixelGeometry=std::move(geometry);a.requiresReset=false;a.InvalidateLocked();a.wake.notify_one();return text(e,"DIAGNOSTIC_NEW_SCOPE_REQUESTED");
}
napi_value pixelDiagnosticPulse(napi_env e,napi_callback_info info){
 size_t n=1;napi_value arg;double raw=0;if(napi_get_cb_info(e,info,&n,&arg,nullptr,nullptr)!=napi_ok||n!=1||napi_get_value_double(e,arg,&raw)!=napi_ok||!std::isfinite(raw)||raw<0||std::floor(raw)!=raw||raw>9007199254740991.0)return text(e,"INVALID_DIAGNOSTIC_PULSE");
 auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);if(!a.pixelSelected||!a.active||!a.window)return text(e,"DIAGNOSTIC_INACTIVE");if(a.pixelFailed)return text(e,"DIAGNOSTIC_FAULT_NEW_SCOPE_REQUIRED");
 const uint64_t now=a.Stamp(),ui=uint64_t(raw);
 if(a.pixelPulseOpen&&(now<a.pixelLastNative||ui<=a.pixelLastUi||now-a.pixelLastNative>33333333||ui-a.pixelLastUi>33333333)){a.pixelFailed=true;a.pixelPulseOpen=false;return text(e,"DIAGNOSTIC_CLOCK_GAP_OR_REVERSED");}
 a.pixelLastNative=now;a.pixelLastUi=ui;a.pixelPulseOpen=true;a.wake.notify_one();return text(e,a.pixelDrawn?(now>a.pixel.until?"DIAGNOSTIC_HOLD_EXPIRED":"DIAGNOSTIC_HOLD"):"DIAGNOSTIC_WAIT_FRAME");
}
napi_value pixelDiagnosticStop(napi_env e,napi_callback_info){auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);if(!a.pixelSelected)return text(e,"NO_DIAGNOSTIC_SCOPE");a.pixelSelected=false;a.pixelPulseOpen=false;a.requiresReset=false;a.InvalidateLocked();a.wake.notify_one();return text(e,"DIAGNOSTIC_RETIRED_NEW_EMPTY_SCOPE");}
napi_value pixelDiagnosticSnapshot(napi_env e,napi_callback_info){
 auto& a=app();input537::Pixel copy;cold503::Scope scope;std::string geometry;bool current=false;uint64_t now=0;
 {std::lock_guard<std::mutex> lock(a.mutex);copy=a.pixel;scope=a.ColdScopeLocked();geometry=a.pixelGeometry;now=a.Stamp();current=a.PixelAllowedLocked(a.epoch,a.window,now)&&copy.epoch==a.epoch;}
 std::array<char,65536> buffer{};cold503::Writer w(buffer.data(),buffer.size());w.Raw("{\"revision\":\"T537_PIXEL_CAPTURE_ENVELOPE_V1\",\"pixel\":");copy.Json(w,now,current);w.Raw(",\"scope\":");cold503::ScopeJson(w,scope);w.Raw(",\"geometryRaw\":");w.String(geometry.data(),geometry.size());w.Raw("}");if(!w.Good())return text(e,"{\"state\":\"INCOMPLETE_UTF8_LIMIT\"}");return text(e,std::string(w.Data(),w.Size()));
}
