#include <cstdint>
#include <cstring>
#include <cassert>
#include <iostream>
#include "argument_shadow.h"
int main(){
 using namespace t496shadow;
 auto* s=find(shaders,4,true);auto old=s->generation;
 const char* text[]={"abc","def"};int lengths[]={3,2};source(4,2,text,lengths);
 assert(s->source==hash("abcde",5));*s={};s=find(shaders,4,true);assert(s->generation>old);
 auto* b=find(buffers,8,true);old=b->generation;bind(0x90d2,8);allocate(0x90d2,4096);assert(b->bytes==4096);
 *b={};b=find(buffers,8,true);assert(b->generation>old&&b->bytes==0);
 auto* p=find(programs,4,true);p->source=hash("abcde",5);current=4;roles[0]={p->source,"TEST_ROLE"};bindings[0]=8;
 uniform(1,"77");assert(describe().find("role=TEST_ROLE")!=std::string::npos);assert(describe().find("u1=77")!=std::string::npos);
 for(auto& u:uniforms)if(u.program==4)u={};assert(describe().find("u1=")==std::string::npos);
 for(unsigned i=1;i<600;++i)find(buffers,i,true);assert(!complete);
 std::cout<<"{\"result\":\"PASS\",\"checks\":\"source length SHA, object reuse generation, binding allocation, uniform retirement, table exhaustion\",\"device\":\"NOT_RUN\"}\n";
}
