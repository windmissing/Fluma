// T333 non-machine regression for the cw_commit label fix (T327 contract §2).
// Mock-GL harness identical in spirit to t270/native_test.cpp; exercises the
// FIXED writer (native_diag.h) against the Engine header contract
// (P011ClearWaterEngine.ets :157/:159-160) and the token layout
// (P013ClearWaterTypes.ets :10). Judgements 1-3 of the four; judgement 4
// (V1 archive adaptation rule) lives in v1_archive_check.py.
#include <cstdint>
#include "native_diag.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
static int calls=0,reads=0;
extern "C" void GL_APIENTRY glGetIntegerv(GLenum name,GLint* value){++calls;*value=name==GL_FRAMEBUFFER_BINDING?7:name==GL_PACK_ALIGNMENT?4:0;}
extern "C" void GL_APIENTRY glReadPixels(GLint,GLint,GLsizei w,GLsizei h,GLenum,GLenum,void* p){++reads;std::memset(p,reads,w*h*4);}
extern "C" GLenum GL_APIENTRY glGetError(){return GL_NO_ERROR;}
static std::vector<std::string> split(const std::string& s,char d){std::vector<std::string> o;std::string cur;for(char c:s){if(c==d){o.push_back(cur);cur.clear();}else cur+=c;}o.push_back(cur);return o;}
int main(){
  p013::ClearWaterPacket p;p.frame.frameId=5;p.frame.surfaceWidthPx=p.frame.surfaceHeightPx=64;
  // Engine contract: h[5]=c.physicsCommit, h[6]=c.sourceCommit (values from the
  // T310 device run: physics=586, source=2 — the exact pair the V1 writer swapped).
  p.header[5]=586;p.header[6]=2;
  // Ledger contract: l[2]=c.sourceCommit, l[3]=c.physicsCommit; TERMINAL receipt
  // (cHasTerminal) l[16]=r.sourceCommit, l[17]=r.physicsCommitId.
  p.ledger[2]=2;p.ledger[3]=586;p.ledger[15]=1;p.ledger[16]=2;p.ledger[17]=491;
  // Real token layout: ['CW_FRAME_TOKEN_C1',run,h[0],h[1],h[5],h[6],h[2],h[10],h[11],h[12],h[13],h[14],...]
  // token[4]=h[5]=physicsCommit(586), token[5]=h[6]=sourceCommit(2).
  p.token="CW_FRAME_TOKEN_C1|cw-run-t333-test|1|1|586|2|5|584|586|2|584|1";
  assert(cwdiag::arm(5,1,0,0,4,4));
  cwdiag::sample(2,p,7); // layer 2 within the armed window
  assert(cwdiag::count==1);
  const auto& r=cwdiag::samples[0];
  int passed=0;
  // Judgement 1: native sourceCommit == critical TERMINAL sourceCommit (ledger side l[16]).
  // V1 would have written header[5]=586 here (physics value) != l[16]=2.
  assert(r.sourceCommit==p.ledger[16]);assert(r.sourceCommit==2);passed++;
  // Judgement 2: native physicsCommit == header[5] (bridge physicsCommitLo reads the same slot).
  // V1 would have written header[6]=2 here != 586.
  assert(r.physicsCommit==p.header[5]);assert(r.physicsCommit==586);passed++;
  // Judgement 3: token field order [4..11] and the sample's two keys no longer mirror-swap.
  // token[4]=h[5]=physicsCommit must equal sample.physicsCommit (V1 mirrored: key 'sourceCommit' held it).
  auto tok=split(r.token,'|');
  assert(tok[4]=="586"&&r.physicsCommit==586);passed++;
  // token[5]=h[6]=sourceCommit must equal sample.sourceCommit.
  assert(tok[5]=="2"&&r.sourceCommit==2);passed++;
  // Whole [4..11] band carries header values in contract order (h[5],h[6],h[2],h[10..14]).
  assert(tok[4]=="586"&&tok[5]=="2"&&tok[6]=="5"&&tok[7]=="584"&&tok[8]=="586"&&tok[9]=="2"&&tok[10]=="584"&&tok[11]=="1");passed++;
  // Revision bump: exports are CW_C_DIAG_V2 and the two keys carry contract values.
  auto out=cwdiag::exportCold();
  assert(out.find("CW_C_DIAG_V2")!=std::string::npos);passed++;
  assert(out.find("CW_C_DIAG_V1")==std::string::npos);passed++;
  assert(out.find("\"sourceCommit\":2,\"physicsCommit\":586")!=std::string::npos);passed++;
  // V1 writer produced the mirrored pair; document that here as the negative shape.
  assert(out.find("\"sourceCommit\":586,\"physicsCommit\":2")==std::string::npos);passed++;
  // Ledger C-side cross-check: sample.sourceCommit == l[2] (C ledger sourceCommit) as well.
  assert(r.sourceCommit==p.ledger[2]);passed++;
  std::cout<<"{\"revision\":\"T333_CW_COMMIT_FIX_NATIVE_V1\",\"passed\":"<<passed<<",\"failed\":0,\"device\":\"NOT_RUN\"}\n";
}
