import hashlib,json,zipfile
from pathlib import Path

rows=[]
for profile in ('regular','experiment'):
    receipt=json.loads(Path(f'test/reports/t257/T456-{profile}-finalized.json').read_text(encoding='utf-8'))
    hap=Path(receipt['signed']['path'])
    assert hashlib.sha256(hap.read_bytes()).hexdigest()==receipt['signed']['sha256']
    with zipfile.ZipFile(hap) as z:
        lib=z.read('libs/arm64-v8a/libgrsm1.so')
        assert b'T451_GPU_BOOT_PROBE_V1' in lib and b'bootDiagnostics' in lib
        assert (b'PROGRESS_V2_NEXT_RUN' in lib)==(profile=='experiment')
    rows.append({'profile':profile,'sha256':receipt['signed']['sha256'],'probeRevision':'T451_GPU_BOOT_PROBE_V1'})
probe=json.loads(Path('.codex/local-test/t456-host/probe.json').read_text(encoding='utf-8'))
assert probe['attempts'][1]['firstFailure']['rawEgl']==0x3003
assert probe['attempts'][1]['shaderLog']=='line "foo"\nnext'
assert probe['attempts'][2]['firstFailure']['stage']==''
assert not probe['attempts'][2]['complete'] and int(probe['attempts'][2]['dropped'])>0
host=Path('.codex/local-test/t456-host-build-v3.txt').read_text(encoding='utf-8',errors='replace')
assert 'T456 generated bridge EGL/READY/accept/Swap verified' in host
out={'schema':'T456_DEVELOPER_OFFLINE_V1','device':'NOT_RUN','signedProfiles':rows,'generatedBridge':{'egl':True,'ready':True,'rootAccept':True,'swap':True,'window':'ANGLE_PBUFFER','clock':'SCRIPTED_OFFLINE'},'negativeControls':['WAIT_WINDOW','same-thread EGL failure before cleanup','late old failure isolated','enter without exit readable','bounded overflow retains first failure','shader log escaped']}
Path('test/reports/t456').mkdir(exist_ok=True)
Path('test/reports/t456/validation.json').write_text(json.dumps(out,indent=2)+'\n',encoding='utf-8')
print(json.dumps(out))
