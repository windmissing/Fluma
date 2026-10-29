// Isolated bounded CPU stage accounting. No readback or logging on draw.
#pragma once
#include <chrono>
#include <array>
#include <mutex>
#include <sstream>
#include <iomanip>
namespace t248 {
struct Row {uint64_t token=0;const char* stage="";double begin=0,end=0;int code=0;};
inline std::array<Row,256> rows{};inline size_t count=0;inline uint64_t dropped=0;inline std::mutex mutex;
inline double now(){return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();}
inline void timing(const char* token,const char* stage,double begin,int code){uint64_t hash=1469598103934665603ULL;while(*token){hash^=uint8_t(*token++);hash*=1099511628211ULL;}std::lock_guard<std::mutex> l(mutex);if(count==rows.size()){++dropped;return;}rows[count++]={hash,stage,begin,now(),code};}
inline void sample(const char*){} // Pixel probes require a separate explicit cold capture.
inline std::string drain(){std::lock_guard<std::mutex> l(mutex);std::ostringstream o;o<<std::setprecision(17)<<"{\"revision\":\"T266_NATIVE_STAGES_V2\",\"gpuTimer\":\"NOT_MEASURED\",\"dropped\":"<<dropped<<",\"rows\":[";for(size_t i=0;i<count;i++){if(i)o<<',';const auto& r=rows[i];o<<"{\"tokenHash\":\""<<r.token<<"\",\"stage\":\""<<r.stage<<"\",\"beginMs\":"<<r.begin<<",\"endMs\":"<<r.end<<",\"code\":"<<r.code<<"}";}o<<"]}";count=0;dropped=0;return o.str();}
}
