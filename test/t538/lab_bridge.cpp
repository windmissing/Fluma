#include "napi/native_api.h"
#include "gpu_resident/resource_lab/lab.h"
using grs::resource538::Lab;
static Lab& lab(){static Lab instance;return instance;}
static napi_value text(napi_env e,const std::string& s){napi_value v;napi_create_string_utf8(e,s.data(),s.size(),&v);return v;}
static bool argument(napi_env e,napi_value v,std::string& s,size_t max){size_t n=0;if(napi_get_value_string_utf8(e,v,nullptr,0,&n)!=napi_ok||n>max)return false;s.resize(n+1);if(napi_get_value_string_utf8(e,v,s.data(),s.size(),&n)!=napi_ok)return false;s.resize(n);return true;}
static napi_value start(napi_env e,napi_callback_info i){size_t n=2;napi_value v[2];std::string name,path;if(napi_get_cb_info(e,i,&n,v,nullptr,nullptr)!=napi_ok||n!=2||!argument(e,v[0],name,64)||!argument(e,v[1],path,4096))return text(e,"INVALID_ARGUMENT");try{return text(e,lab().Start(name,path));}catch(const std::exception& error){return text(e,std::string("LAB_START_FAILED:")+error.what());}}
template<unsigned Action>static napi_value command(napi_env e,napi_callback_info i){size_t n=1;napi_value v;std::string id;if(napi_get_cb_info(e,i,&n,&v,nullptr,nullptr)!=napi_ok||n!=1||!argument(e,v,id,20)||id.empty()||id.find_first_not_of("0123456789")!=std::string::npos)return text(e,"INVALID_JOB");uint64_t value=0;auto parsed=std::from_chars(id.data(),id.data()+id.size(),value);if(parsed.ec!=std::errc())return text(e,"INVALID_JOB");if(!lab().Matches(value))return text(e,"STALE_JOB");if constexpr(Action==1)lab().Release();if constexpr(Action==2)lab().Cancel();return text(e,lab().Poll(value));}
static napi_value init(napi_env e,napi_value exports){napi_property_descriptor p[]={
 {"resourceLabStartV1",nullptr,start,nullptr,nullptr,nullptr,napi_default,nullptr},
 {"resourceLabPollV1",nullptr,command<0>,nullptr,nullptr,nullptr,napi_default,nullptr},
 {"resourceLabReleaseBarrierV1",nullptr,command<1>,nullptr,nullptr,nullptr,napi_default,nullptr},
 {"resourceLabCancelV1",nullptr,command<2>,nullptr,nullptr,nullptr,napi_default,nullptr},
 {"resourceLabExportV1",nullptr,command<0>,nullptr,nullptr,nullptr,napi_default,nullptr}};napi_define_properties(e,exports,5,p);return exports;}
static napi_module module={1,0,nullptr,init,"grs_resource_lab",nullptr,{0}};
extern "C" __attribute__((constructor)) void RegisterResourceLab(){napi_module_register(&module);}
