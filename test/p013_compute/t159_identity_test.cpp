// Expected golden bytes are independently packed by Python struct, not this serializer/schema.
#include "pair_identity_v2.h"
#include "pair_sha256_v1.h"
#include "pair_owner_v1.h"
#include <fstream>
#include <cstdio>
using namespace p011gpu;
#define CHECK(x) do{if(!(x)){std::printf("FAIL line=%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 Header h;for(uint32_t i=0;i<64;i++)h.w[i]=0x12340000u+i*0x101u;
 RequestIdentity id;id.identityEncodingRevision=kIdentityEncodingV2;id.runId="run-\xe6\xb2\x99";id.stateId="state";id.configId="cfg|x=1";id.buildManifestSha="000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
 id.paramsRevision=4294967297.;id.inputRevision=9007199254740991.;id.seed=-0.;
 id.ownerGeneration=0x1111111122222222ULL;id.contextEpoch=0x3333333344444444ULL;id.payloadSerial=0x5555555566666666ULL;
 id.poolMutationSerial=0x7777777788888888ULL;id.attemptSerial=0x99999999aaaaaaaaULL;id.substepSerial=0xbbbbbbbbccccccccULL;
 id.candidateRole=3;id.candidateOrdinal=29;id.slotIndex=1;
 std::array<uint8_t,32> snapshot{},payload{};for(int i=0;i<32;i++){snapshot[i]=uint8_t(0x80+i);payload[i]=uint8_t(0xc0+i);}
 std::vector<uint8_t> bytes;CHECK(canonicalIdentityV2(h,id,.5,snapshot,payload,bytes));
 std::ifstream golden("test/p013_compute/fixtures/t159_identity_golden.hex");std::string expected;golden>>expected;
 static const char* digits="0123456789abcdef";std::string actual;for(auto b:bytes){actual+=digits[b>>4];actual+=digits[b&15];}CHECK(actual==expected);
 Sha256 sha;sha.update(bytes.data(),bytes.size());auto digest=sha.finish();std::printf("IDENTITY_BYTES=%zu SHA256=",bytes.size());for(auto b:digest)std::printf("%02x",b);std::puts("");
 id.buildManifestSha="bad";CHECK(!canonicalIdentityV2(h,id,.5,snapshot,payload,bytes));id.buildManifestSha=std::string(64,'a');id.configId=std::string(8193,'x');CHECK(!canonicalIdentityV2(h,id,.5,snapshot,payload,bytes));
 std::puts("PASS canonical T138 UTF8/raw F64/high words/raw32 build SHA/invalid boundaries");return 0;
}
