const char* EvidenceConfigError(napi_env e,napi_callback_info info,EvidenceConfig& c){
 napi_value o,keys;
 uint32_t length=0;
 if(!EvidenceJsonArg(e,info,o))return "json";
 if(napi_get_property_names(e,o,&keys)!=napi_ok||napi_get_array_length(e,keys,&length)!=napi_ok||length!=9)return "fieldCount";
 if(!EvidenceString(e,o,"packageSha256",c.packageSha256,64)||c.packageSha256.size()!=64||
    c.packageSha256.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)return "packageSha256";
 if(!EvidenceString(e,o,"expectedRevision",c.expectedRevision,64)||
    (c.expectedRevision!="GRS_M1_PROGRESS_CLOCK_V1"&&c.expectedRevision!="GRS_M1_PROGRESS_CLOCK_V2"))return "expectedRevision";
 if(!EvidenceU64(e,o,"epoch",c.epoch))return "epoch";
 if(!EvidenceWord(e,o,"runLo",c.runLo))return "runLo";
 if(!EvidenceWord(e,o,"runHi",c.runHi))return "runHi";
 if(!EvidenceU64(e,o,"pageSession",c.pageSession))return "pageSession";
 if(!EvidenceU64(e,o,"durationNs",c.durationNs)||c.durationNs<2000000000ULL||c.durationNs>5000000000ULL)return "durationNs";
 if(!EvidenceWord(e,o,"maxRows",c.maxRows)||!c.maxRows||c.maxRows>4096)return "maxRows";
 if(!EvidenceWord(e,o,"maxBytes",c.maxBytes)||!c.maxBytes||c.maxBytes>1048576)return "maxBytes";
 return c.Valid()?nullptr:"validation";
}
napi_value beginEvidenceCapture(napi_env e,napi_callback_info info){
 EvidenceConfig config;
 if(const char* field=EvidenceConfigError(e,info,config)){
  return text(e,std::string("{\"state\":\"REJECTED\",\"reason\":\"INVALID_CAPTURE_CONFIG\",\"invalidField\":\"")+field+"\"}");
 }
