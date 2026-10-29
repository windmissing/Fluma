from pathlib import Path
import zipfile,json,hashlib
root=Path(__file__).resolve().parents[2];project=root/'.codex/local-test/t266-v9'
hap=project/'entry/build/default/outputs/default/entry-default-unsigned.hap'
with zipfile.ZipFile(hap) as z:
    pages=json.loads(z.read('resources/base/profile/main_pages.json'))
    assert pages['src']==['pages/P051DerivedV2Demo']
    binary=z.read('libs/arm64-v8a/libp013renderer.so')
    assert b'T266_NATIVE_STAGES_V2' in binary and b'UPLOAD_EXCLUSIVE' in binary and b't266Drain' in binary
    assert any(n.endswith('.abc') for n in z.namelist())
sources=['entry/src/main/ets/physics/WaterSandField.ets','entry/src/main/ets/physics/derived/P051DerivedDemoV2.ets','entry/src/main/ets/physics/derived/P051DerivedFacadeV2.ets','entry/src/main/ets/pages/P051DerivedV2Demo.ets','entry/src/main/ets/entryability/EntryAbility.ets','entry/src/main/cpp/p013renderer/napi_bridge.cpp','entry/src/main/cpp/p013renderer/scene_gpu.cpp','entry/src/main/cpp/p013renderer/t248_observation.h']
result={'revision':'T266_PACKAGE_V2','packageSha256':hashlib.sha256(hap.read_bytes()).hexdigest(),'sourceSha256':{n:hashlib.sha256((project/n).read_bytes()).hexdigest() for n in sources},'nativeSha256':hashlib.sha256(binary).hexdigest(),'route':'pages/P051DerivedV2Demo','routeAndNativeCodePackaged':'PASS','deviceColdStart':'NOT_RUN'}
(root/'test/reports/t266/delivery.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
