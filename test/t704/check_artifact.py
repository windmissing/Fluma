from pathlib import Path
import hashlib
import json
import subprocess
import sys
import zipfile

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'offline_handoff'))
from package import build_id
sha=lambda raw:hashlib.sha256(raw).hexdigest()
original=json.loads(Path('test/reports/t690/compile.json').read_bytes())
handoff=json.loads(Path('test/reports/t704/handoff.json').read_bytes())
asm=Path(original['staticCompileCommand'][-1]).read_bytes()
assert sha(asm)==original['pipelineSHA256']
assert handoff['unsigned']['sha256']==original['hapSHA256']
with zipfile.ZipFile(handoff['signed']['path']) as package:
    elf=package.read('libs/arm64-v8a/libgrs_g13.so')
    abc=package.read('ets/modules.abc')
assert sha(elf)==original['nativeSHA256']
assert sha(abc)==original['abcSHA256']
assert asm in elf and b'coldFP32Probe' in asm
assert build_id(elf)=='0ce39297324fe8f1a2a6a14e3090ea6f2028503e'
project=Path('.codex/local-test/t690-final/entry/src/main')
transport=(project/'cpp/gpu_resident/g13flow/transport.cpp').read_text(encoding='utf8')
assert 'glGetShaderPrecisionFormat(GL_VERTEX_SHADER, GL_HIGH_FLOAT' in transport
assert 'glGetShaderPrecisionFormat(GL_COMPUTE_SHADER' not in transport
page=(project/'ets/pages/GpuResidentG13.ets').read_text(encoding='utf8')
collector=(project/'ets/physics/T672Collector.ets').read_text(encoding='utf8')
assert page.index('this.diagnostic.arm(')<page.index('native.g13Configure(')<page.index('this.diagnostic.configured(')
assert 'XComponent(' in page and 'T672_ONE_CONFIGURE_ONLY' in page
for marker in ('8192','16384','1048576','> 4','> 3'):
    assert marker in collector
run=subprocess.run(original['staticCompileCommand'],check=True,capture_output=True)
print(json.dumps({'staticCompileExit':run.returncode,'argv':run.args,
 'stdout':run.stdout.decode(),'stderr':run.stderr.decode(),'pipelineSHA':sha(asm),
 'elfSHA':sha(elf),'buildId':build_id(elf),'abcSHA':sha(abc),'device':'NOT_RUN'},indent=2))
