from assemble_kernels import ROOT,assemble
import hashlib
out=ROOT/'.codex/local-test/t522-native-host';out.mkdir(parents=True,exist_ok=True)
source=assemble('source_v4.comp');quantum=assemble('quantum_v4.comp')
def sha(s):return hashlib.sha256(s.encode()).hexdigest()
for name,symbol,text in [('m3a_source_kernel.h','Source',source),('m3a_quantum_kernel.h','Quantum',quantum)]:
    (out/name).write_text('#pragma once\nstatic constexpr const char* kM3a'+symbol+'=R"M3HOST('+text+')M3HOST";\nstatic constexpr const char* kM3a'+symbol+'Sha="'+sha(text)+'";\n',encoding='utf-8')
(out/'m3a_root_sha.h').write_text('#pragma once\nstatic constexpr const char* kM3aRootSha="'+sha(source+'\n'+quantum+'\nM3_OPCODE_SPECIALIZATION_0_14')+'";\n',encoding='utf-8')
