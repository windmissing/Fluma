#pragma once
#include <GLES3/gl3.h>
#include <array>
#include <string>
#include <sstream>
#include <mutex>
#include <chrono>
namespace t276pixel {
struct Row{std::string token;uint64_t frame=0;int fbo=0,x=0,y=0;uint32_t code=0;double ms=0;std::array<unsigned char,64> rgba{};};
inline std::array<Row,4> rows{};inline size_t count=0;inline uint64_t first=0;inline std::mutex mutex;
inline bool arm(uint64_t f){std::lock_guard<std::mutex> l(mutex);if(first||f==0)return false;first=f;return true;}
inline void sample(const char* token,uint64_t frame,int width,int height){std::lock_guard<std::mutex> l(mutex);if(!first||frame<first||frame-first>=4||count==4)return;auto start=std::chrono::steady_clock::now();auto& r=rows[count++];r.token=token;r.frame=frame;r.x=width/2;r.y=height/8;r.code=0;int pack=0,align=0,row=0,sr=0,sp=0;glGetIntegerv(GL_FRAMEBUFFER_BINDING,&r.fbo);glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING,&pack);glGetIntegerv(GL_PACK_ALIGNMENT,&align);glGetIntegerv(GL_PACK_ROW_LENGTH,&row);glGetIntegerv(GL_PACK_SKIP_ROWS,&sr);glGetIntegerv(GL_PACK_SKIP_PIXELS,&sp);if(r.fbo==0||pack||align>4||row||sr||sp||r.x+4>width||r.y+4>height)r.code=0xffffffffu;else{glReadPixels(r.x,r.y,4,4,GL_RGBA,GL_UNSIGNED_BYTE,r.rgba.data());r.code=glGetError();}r.ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();}
inline std::string exportCold(){std::lock_guard<std::mutex> l(mutex);std::ostringstream o;o<<"{\"revision\":\"T276_SAME_TOKEN_PIXELS_V1\",\"format\":\"SRGB8_ALPHA8_RGBA8_ENCODED\",\"rows\":[";const char* hex="0123456789abcdef";for(size_t i=0;i<count;i++){const auto& r=rows[i];if(i)o<<',';o<<"{\"frame\":"<<r.frame<<",\"tokenHex\":\"";for(unsigned char c:r.token)o<<hex[c>>4]<<hex[c&15];o<<"\",\"fbo\":"<<r.fbo<<",\"roi\":["<<r.x<<','<<r.y<<",4,4],\"raw\":"<<r.code<<",\"ms\":"<<r.ms<<",\"rgbaHex\":\"";if(r.code==0)for(auto c:r.rgba)o<<hex[c>>4]<<hex[c&15];o<<"\"}";}o<<"]}";return o.str();}
}
