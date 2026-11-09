#pragma once
#include <string>
namespace grs::m1 {
#ifdef GRS_PROGRESS_V2_EXPERIMENT
inline constexpr bool kExperienceProgressV2=true;
#else
inline constexpr bool kExperienceProgressV2=false;
#endif
inline std::string ExperienceCapabilities(unsigned k,unsigned kw){
 const std::string profile=kExperienceProgressV2?"T438_EXPERIMENT_V2":"T438_REGULAR";
 return "{\"revision\":\"GRS_M1_EXPERIENCE_V1\",\"buildProfile\":\""+profile+
  "\",\"GRS_APP_M1\":true,\"GRS_PROGRESS_V2_EXPERIMENT\":"+(kExperienceProgressV2?"true":"false")+
  ",\"K\":"+std::to_string(k)+",\"Kw\":"+std::to_string(kw)+
  ",\"features\":3,\"layoutRevision\":2,\"numericRevision\":1,\"sceneRevision\":2,"
  "\"scenePolicy\":\"GRS_GPU_SCENE_Q20_M1_V2\",\"defaultProgress\":\"V1\","
  "\"M2\":\"UNSUPPORTED_M1_CONFIG\",\"M3\":\"UNSUPPORTED_M1_CONFIG\"}";
}
}
