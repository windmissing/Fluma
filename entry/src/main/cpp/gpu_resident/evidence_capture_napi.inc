// T409 cold NAPI parsing and three capture APIs (no GL or waiting here).
bool EvidenceJsonArg(napi_env e,napi_callback_info info,napi_value& object){
 size_t n=1;napi_value arg;size_t size=0;
 if(napi_get_cb_info(e,info,&n,&arg,nullptr,nullptr)!=napi_ok||n!=1||
  napi_get_value_string_utf8(e,arg,nullptr,0,&size)!=napi_ok||size==0||size>2048)return false;
 napi_value global,json,parse;
 if(napi_get_global(e,&global)!=napi_ok||napi_get_named_property(e,global,"JSON",&json)!=napi_ok||napi_get_named_property(e,json,"parse",&parse)!=napi_ok)return false;
 if(napi_call_function(e,json,parse,1,&arg,&object)!=napi_ok){napi_value exception;napi_get_and_clear_last_exception(e,&exception);return false;}
 napi_valuetype type;return napi_typeof(e,object,&type)==napi_ok&&type==napi_object;
}
bool EvidenceString(napi_env e,napi_value o,const char* key,std::string& out,size_t max){
 napi_value v;std::array<char,129> data{};size_t size=0,fullLength=0;
 if(max>128||napi_get_named_property(e,o,key,&v)!=napi_ok||napi_get_value_string_utf8(e,v,nullptr,0,&fullLength)!=napi_ok||fullLength>max||napi_get_value_string_utf8(e,v,data.data(),data.size(),&size)!=napi_ok||size!=fullLength)return false;
 out.assign(data.data(),size);return true;
}
bool EvidenceU64(napi_env e,napi_value o,const char* key,uint64_t& out){napi_value v;return napi_get_named_property(e,o,key,&v)==napi_ok&&PageU64(e,v,out);}
bool EvidenceNumber(napi_env e,napi_value o,const char* key,double& out){napi_value v;return napi_get_named_property(e,o,key,&v)==napi_ok&&napi_get_value_double(e,v,&out)==napi_ok&&std::isfinite(out);}
bool EvidenceWord(napi_env e,napi_value o,const char* key,uint32_t& out){double n;if(!EvidenceNumber(e,o,key,n)||n<0||n>UINT32_MAX||n!=std::floor(n))return false;out=uint32_t(n);return true;}
bool EvidenceConfigArg(napi_env e,napi_callback_info info,EvidenceConfig& c){
 napi_value o,keys;uint32_t length=0;if(!EvidenceJsonArg(e,info,o)||napi_get_property_names(e,o,&keys)!=napi_ok||napi_get_array_length(e,keys,&length)!=napi_ok||length!=9)return false;
 return EvidenceString(e,o,"packageSha256",c.packageSha256,64)&&EvidenceString(e,o,"expectedRevision",c.expectedRevision,64)&&
  EvidenceU64(e,o,"epoch",c.epoch)&&EvidenceU64(e,o,"pageSession",c.pageSession)&&EvidenceU64(e,o,"durationNs",c.durationNs)&&
  EvidenceWord(e,o,"runLo",c.runLo)&&EvidenceWord(e,o,"runHi",c.runHi)&&EvidenceWord(e,o,"maxRows",c.maxRows)&&EvidenceWord(e,o,"maxBytes",c.maxBytes)&&c.Valid();
}
napi_value beginEvidenceCapture(napi_env e,napi_callback_info info){
 EvidenceConfig config;if(!EvidenceConfigArg(e,info,config))return text(e,"{\"state\":\"REJECTED\",\"reason\":\"INVALID_CAPTURE_CONFIG\"}");
#ifdef GRS_PROGRESS_V2_EXPERIMENT
 auto& a=app();std::string out;
 {std::lock_guard<std::mutex> l(a.mutex);out=a.ReserveEvidenceLocked(std::move(config));}
 return text(e,out);
#else
 return text(e,"{\"state\":\"REJECTED\",\"reason\":\"CAPTURE_UNSUPPORTED\"}");
#endif
}
napi_value captureRequestApi(napi_env e,napi_callback_info info,bool cancel){
#ifdef GRS_PROGRESS_V2_EXPERIMENT
 size_t n=1;napi_value v;uint64_t id=0;
 if(napi_get_cb_info(e,info,&n,&v,nullptr,nullptr)!=napi_ok||n!=1||!PageU64(e,v,id))return text(e,App::CaptureReject("INVALID_CAPTURE_REQUEST_ID"));
 auto& a=app();std::string out;
 {std::lock_guard<std::mutex> l(a.mutex);out=a.ReadCaptureRequestLocked(id,cancel);}
 return text(e,out);
#else
 return text(e,"{\"state\":\"REJECTED\",\"reason\":\"CAPTURE_UNSUPPORTED\"}");
#endif
}
napi_value evidenceCaptureRequest(napi_env e,napi_callback_info info){return captureRequestApi(e,info,false);}
napi_value cancelEvidenceCaptureRequest(napi_env e,napi_callback_info info){return captureRequestApi(e,info,true);}
napi_value endEvidenceCapture(napi_env e,napi_callback_info info){
 size_t n=1;napi_value v;uint64_t id=0;
 if(napi_get_cb_info(e,info,&n,&v,nullptr,nullptr)!=napi_ok||n!=1||!PageU64(e,v,id))return text(e,"{\"state\":\"REJECTED\",\"reason\":\"INVALID_CAPTURE_ID\"}");
 auto& a=app();std::lock_guard<std::mutex> l(a.mutex);return text(e,a.EndEvidenceLocked(id));
}
napi_value evidenceCapturePage(napi_env e,napi_callback_info info){
 size_t n=2;napi_value v[2];uint64_t id=0;double cursor=0;
 if(napi_get_cb_info(e,info,&n,v,nullptr,nullptr)!=napi_ok||n!=2||!PageU64(e,v[0],id)||napi_get_value_double(e,v[1],&cursor)!=napi_ok||!std::isfinite(cursor)||cursor<0||cursor>1033||std::floor(cursor)!=cursor)return text(e,"{\"state\":\"REJECTED\",\"reason\":\"INVALID_CAPTURE_CURSOR\"}");
 auto& a=app();std::lock_guard<std::mutex> l(a.mutex);
 if(!a.evidence||id!=a.evidence->id)return text(e,"{\"state\":\"REJECTED\",\"reason\":\"STALE_CAPTURE_ID\"}");
 return text(e,a.evidence->Page(uint32_t(cursor)));
}
napi_value recordSourceAttempt(napi_env e,napi_callback_info info){
 napi_value o,v;EvidenceDown d;std::string why;double epoch=0;
 if(!EvidenceJsonArg(e,info,o)||!EvidenceU64(e,o,"sampleSerial",d.sampleSerial)||!EvidenceU64(e,o,"pageSession",d.session)||
  !EvidenceNumber(e,o,"epoch",epoch)||epoch<0||epoch>9007199254740991.||epoch!=std::floor(epoch)||
  !EvidenceWord(e,o,"runLo",d.runLo)||!EvidenceWord(e,o,"runHi",d.runHi)||!EvidenceWord(e,o,"kind",d.kind)||d.kind>1||
  !EvidenceNumber(e,o,"x",d.x)||!EvidenceNumber(e,o,"y",d.y)||!EvidenceNumber(e,o,"width",d.width)||!EvidenceNumber(e,o,"height",d.height)||
  !EvidenceString(e,o,"reason",why,48)||napi_get_named_property(e,o,"accepted",&v)!=napi_ok||napi_get_value_bool(e,v,&d.accepted)!=napi_ok)return text(e,"INVALID_SOURCE_ATTEMPT");
 for(char c:why)if(!((c>='A'&&c<='Z')||c=='_'))return text(e,"INVALID_SOURCE_ATTEMPT_REASON");
 napi_value finite;if(napi_get_named_property(e,o,"coordinatesFinite",&finite)!=napi_ok||napi_get_value_bool(e,finite,&d.coordinatesFinite)!=napi_ok)return text(e,"INVALID_SOURCE_ATTEMPT");
 d.epoch=uint64_t(epoch);d.valid=true;auto& a=app();std::lock_guard<std::mutex> l(a.mutex);a.CaptureDownLocked(d,why.c_str());return text(e,"SOURCE_ATTEMPT_RECORDED");
}
