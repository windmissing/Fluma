// Uses the real program() read/hash early-return before any GL call.
#include "pair_executor_v1.cpp"
#include <cassert>
int main(int argc,char** argv){
 assert(argc==2);const std::string original="#version 310 es\nvoid main(){}\n";p011gpu::Sha256 h;h.update(original.data(),original.size());auto digest=h.finish();std::string expected;const char* hex="0123456789abcdef";for(auto b:digest){expected+=hex[b>>4];expected+=hex[b&15];}
 {std::ofstream f(argv[1],std::ios::binary);f<<original<<'\n';}
 GLuint programId=0;p011gpu::RequestDiagnostics d;p011gpu::ProgramDiagnosticsV3 report;
 assert(!p011gpu::program(argv[1],programId,d,&expected,report));
 assert(d.stage=="SHADER_SHA_MISMATCH"&&d.rawGlError==0&&programId==0);
 assert(report.expectedSha==expected&&report.actualSha!=expected&&report.sourceBytes==original.size()+1);
 assert(report.compileStatus=="NOT_RUN"&&report.linkStatus=="NOT_RUN");
 std::puts("PASS T171 actual private-file byte mutation -> real program read/SHA mismatch before GL compile; expected hash unchanged. No GPU execution claimed.");
}
