#include <cassert>
#include <iostream>
#include "gl_sink.inc"
int main(int argc,char**argv){
 assert(argc==2);assert(t496_sink_initialize("fixture-budget","UNIT",argv[1]));
 assert(t496sink::EmitRecord(1,"ENTER",1,"fixture-only"));
 auto size=std::filesystem::file_size(t496sink::TracePath());
 t496sink::BudgetBytes().store(t496sink::kAttemptFileBytesMax-1);
 assert(!t496sink::EmitRecord(1,"EXIT",1,"must reject before write"));
 assert(t496sink::BudgetExceeded().load());assert(t496diag::words[6]==1);
 assert(std::filesystem::file_size(t496sink::TracePath())==size);
 assert(!t496sink::EmitRecord(1,"EXIT",1,"still rejected"));
 std::cout<<"{\"result\":\"PASS\",\"checks\":\"cap rejects before write, persistent mapped flag, later writes refused\",\"device\":\"NOT_RUN\"}\n";
}
