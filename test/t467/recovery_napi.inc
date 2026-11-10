bool PageU64(napi_env,napi_value,uint64_t&);
napi_value recoveryState(napi_env e,napi_callback_info){auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);std::ostringstream out;a.RecoveryJson(out,false);return text(e,out.str());}
napi_value beginRecoverySession(napi_env e,napi_callback_info info){
 size_t n=5;napi_value args[5];uint64_t v[5]{};
 if(napi_get_cb_info(e,info,&n,args,nullptr,nullptr)!=napi_ok||n!=5)return text(e,"0");
 for(unsigned i=0;i<5;++i)if(!PageU64(e,args[i],v[i]))return text(e,"0");
 if(v[2]>UINT32_MAX||v[3]>UINT32_MAX)return text(e,"0");
 auto& a=app();std::lock_guard<std::mutex> lock(a.mutex);
 return text(e,std::to_string(a.RecoverySessionLocked(v[0],v[1],{uint32_t(v[2]),uint32_t(v[3])},v[4])));
}
