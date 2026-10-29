"""Offline package/routing gate; window execution belongs to device testing."""
from pathlib import Path
import hashlib,json,zipfile
root=Path(__file__).resolve().parents[2]
hap=root/'entry/build/default/outputs/default/entry-default-unsigned.hap'
with zipfile.ZipFile(hap) as z:
    names=z.namelist()
    libs=[n for n in names if n.endswith('/libgrsm0.so')]
    assert len(libs)==1,libs
    native=z.read(libs[0])
    for marker in [b'RegisterGrsM0',b'PRODUCT_COMPLETION',b'GRS_SCENE_SWAP',b'ENQUEUED_NOT_ACK']:
        assert marker in native,marker
    pages=json.loads(z.read('resources/base/profile/main_pages.json'))
    assert 'pages/GpuResidentM0' in pages['src']
    assert any(n.endswith('.abc') for n in names)
files=['entry/src/main/cpp/gpu_resident/app_bridge.cpp','entry/src/main/ets/pages/GpuResidentM0.ets','entry/src/main/ets/entryability/EntryAbility.ets','entry/src/main/cpp/CMakeLists.txt']
result={'revision':'T236_APP_ROUTE_OFFLINE_V1','packageSha256':hashlib.sha256(hap.read_bytes()).hexdigest(),'nativeLibrary':libs[0],'nativeBytes':len(native),'sourceSha256':{n:hashlib.sha256((root/n).read_bytes()).hexdigest() for n in files},'packageRouteAndLinkedProductCode':'PASS','windowExecution':'NOT_RUN','device':'NOT_RUN'}
out=root/'test/reports/t236/resume/app-route-package.json'
out.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
