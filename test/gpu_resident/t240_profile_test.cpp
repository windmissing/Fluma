#include "m1/profile.h"
#include <cassert>
#include <iostream>
int main(){using namespace grs::m1;int count=0;for(const auto& p:profiles){ProfilePlan out;
 assert(Plan(p.K,p.Kw,3,2,256,1ULL<<30,out)==ConfigureCode::ProtocolReadyShaderNotImplemented);assert(out.profile==&p&&out.eventStride>=uint64_t(p.events)*256&&out.eventStride%256==0);++count;
 assert(Plan(p.K,p.Kw,3,2,256,1024,out)==ConfigureCode::InvalidRange&&out.profile==nullptr);++count;
 assert(Plan(p.K,p.Kw,7,2,256,1ULL<<30,out)==ConfigureCode::Unsupported);++count;
 assert(Plan(p.K,p.Kw,3,1,256,1ULL<<30,out)==ConfigureCode::Unsupported);++count;
 }std::cout<<"{\"passed\":"<<count<<",\"failed\":0,\"M1Shader\":\"NOT_IMPLEMENTED\"}\n";}
