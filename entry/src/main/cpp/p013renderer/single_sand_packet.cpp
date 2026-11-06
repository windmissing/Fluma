#include "single_sand_packet.h"
#include "material_uploader.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace p013 {
namespace {
std::string Hash(const void* bytes,size_t size){return MaterialSha256(static_cast<const uint8_t*>(bytes),size);}
void Append(std::string& out,const std::string& s){out+=std::to_string(s.size())+":"+s;}
void AppendDouble(std::string& out,double d){
 uint64_t bits;std::memcpy(&bits,&d,8);
 const char* hex="0123456789abcdef";
 for(int i=0;i<8;i++){unsigned b=(bits>>(8*i))&255;out+=hex[b>>4];out+=hex[b&15];}
}
float Linear(double b){const auto s=b/255;return float(s<=.04045?s/12.92:std::pow((s+.055)/1.055,2.4));}
}
bool SingleHash(const std::string& s){
 return s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});
}
const char* SingleSandConfig::Validate(){
 const auto& v=values;
 if(!revision||!session||!SingleHash(grainHash)||!SingleHash(generatorHash)||!SingleHash(digest))return "SINGLE_CONFIG_INVALID";
 for(auto x:v)if(!std::isfinite(x))return "SINGLE_CONFIG_INVALID";
 if(v[0]!=145||v[1]!=168||v[2]!=111||v[3]!=16||v[4]!=16||v[5]!=36||
    v[6]<1.5||v[6]>2.5||(v[7]!=2&&v[7]!=3)||v[8]<0||v[8]>.12||v[9]<0||v[9]>.10||
    v[10]<4||v[10]>12||v[11]!=-.35||v[12]!=0||v[13]!=.82||v[14]<0||v[14]>.10||
    v[15]<0||v[15]>.16||v[16]!=.60||v[17]!=.95||v[18]!=20261022)return "SINGLE_CONFIG_INVALID";
 const auto upper=std::max(1+v[8]+v[9],1+v[14]);
 if(v[8]+v[9]>.22||v[14]+v[15]>.30||1-v[8]-v[9]<=0||1-v[14]-v[15]<=0)return "COLOR_GAMUT_UNSUPPORTED";
 const auto length=std::hypot(v[11],v[13]);
 for(int i=0;i<3;i++){
  albedo[i]=Linear(v[i]);background[i]=Linear(v[3+i]);light[i]=float(v[11+i]/length);
  if(albedo[i]*upper>1)return "COLOR_GAMUT_UNSUPPORTED";
 }
 std::string canonical;Append(canonical,SingleSandVersions);Append(canonical,std::to_string(revision));
 Append(canonical,generatorHash);Append(canonical,grainHash);
 for(double x:v)AppendDouble(canonical,x);
 return Hash(canonical.data(),canonical.size())==digest?"OK":"SINGLE_CONFIG_INVALID";
}
std::string SingleIdentity(const SingleSandPacket& p){
 return p.run+":"+std::to_string(p.epoch)+":"+std::to_string(p.commit)+":"+std::to_string(p.frame)+
        ":"+std::to_string(p.generation)+":"+p.digest+":"+p.displayHash+":"+p.pbHash+":"+p.particleHash;
}
const char* SingleSandPacket::Validate(){
 if(!session||!surface||!context||!geometry||!continuity||!epoch||!commit||!frame||!certificate||!seal||
    !revision||!generation||run.empty()||token!=SingleIdentity(*this)||
    (kind!="STEP"&&kind!="EXTERNAL_ADMISSION"&&kind!="RESET"&&kind!="RESTORE"&&kind!="INVERT"&&kind!="ROUTE_CHANGE")||
    !SingleHash(rawHash)||!SingleHash(digest)||!SingleHash(grainHash))return "SINGLE_PUBLICATION_MISMATCH";
 if(width<1||height<1||capacity!=256||!std::isfinite(logicalW)||!std::isfinite(logicalH)||logicalW<=0||logicalH<=0)
  return "SINGLE_GEOMETRY_UNSUPPORTED";
 for(double x:box)if(!std::isfinite(x)||!std::isfinite(float(x)))return "SINGLE_GEOMETRY_UNSUPPORTED";
 const double sx=box[2]/logicalW,sy=box[3]/logicalH;
 if(box[2]<=0||box[3]<=0||!std::isfinite(sx)||!std::isfinite(sy)||sx<=0||sy<=0||
    std::abs(sx-sy)>1e-6*std::max({1.,std::abs(sx),std::abs(sy)}))return "SINGLE_GEOMETRY_UNSUPPORTED";
 scale=sx;
 if(count<0||slots<2||slots>512||count>slots)return "SINGLE_TYPED_RANGE";
 for(int i=0;i<128;i++)if(!std::isfinite(profile[i])||profile[i]<0||profile[i]>capacity||!std::isfinite(profile[128+i]))return "SINGLE_TYPED_RANGE";
 for(int i=0;i<count;i++){
  if((meta[2*i]&255)!=0)return "UNSUPPORTED_MATERIAL_SET";
  if((meta[2*i]!=256&&meta[2*i]!=512)||!meta[2*i+1]||
     !std::isfinite(xy[2*i])||!std::isfinite(xy[2*i+1])||xy[2*i]<0||xy[2*i]>=128||xy[2*i+1]<0||xy[2*i+1]>capacity)
   return "SINGLE_TYPED_RANGE";
  for(int j=0;j<i;j++)if(meta[2*i+1]==meta[2*j+1])return "SINGLE_TYPED_RANGE";
 }
 std::array<uint8_t,8192> particles{};
 std::memcpy(particles.data(),xy.data(),count*8);
 std::memcpy(particles.data()+count*8,meta.data(),count*8);
 if(Hash(profile.data(),512)!=displayHash||Hash(profile.data()+128,512)!=pbHash||Hash(particles.data(),count*16)!=particleHash)
  return "SINGLE_PUBLICATION_MISMATCH";
 return "OK";
}
}
