napi_value m2CreateDiagnostics(napi_env e,napi_callback_info info){
 size_t n=1;napi_value arg;uint64_t serial=0;
 if(napi_get_cb_info(e,info,&n,&arg,nullptr,nullptr)!=napi_ok||(n&& !PageU64(e,arg,serial)))return text(e,"{\"state\":\"INVALID_CREATE_SERIAL\"}");
 create468::Attempt snapshot;if(!create468::Global().Copy(serial,snapshot))return text(e,"{\"state\":\"NO_CREATE_ATTEMPT\"}");
 std::ostringstream out;create468::Json(out,snapshot);return text(e,out.str());
}
napi_value m2CreateLog(napi_env e,napi_callback_info info){
 size_t n=2;napi_value args[2];uint64_t serial=0;double object=0;
 if(napi_get_cb_info(e,info,&n,args,nullptr,nullptr)!=napi_ok||n!=2||!PageU64(e,args[0],serial)||
    napi_get_value_double(e,args[1],&object)!=napi_ok||!std::isfinite(object)||object<0||object>2||std::floor(object)!=object)return text(e,"{\"state\":\"INVALID_CREATE_LOG\"}");
 create468::Attempt snapshot;if(!create468::Global().Copy(serial,snapshot))return text(e,"{\"state\":\"STALE_CREATE_SERIAL\"}");
 const auto& p=snapshot.programs[size_t(object)];std::ostringstream out;
 out<<"{\"state\":\"CREATE_LOG\",\"bootSerial\":\""<<snapshot.serial<<"\",\"object\":"<<uint32_t(object)<<",\"hex\":\"";
 const char* h="0123456789abcdef";for(uint32_t i=0;i<p.used;++i)out<<h[p.raw[i]>>4]<<h[p.raw[i]&15];out<<"\"}";return text(e,out.str());
}
