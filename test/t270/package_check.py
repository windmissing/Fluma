from pathlib import Path
import json,zipfile,hashlib
root=Path(__file__).resolve().parents[2];p=root/'.codex/local-test/t270-v4';hap=p/'entry/build/default/outputs/default/entry-default-unsigned.hap'
with zipfile.ZipFile(hap) as z:
    assert json.loads(z.read('resources/base/profile/main_pages.json'))['src']==['pages/ClearWaterDemo']
    native=z.read('libs/arm64-v8a/libp013renderer.so')
    for word in [b'CW_C_DIAG_V2',b'cwDiagExport',b'cwDiagArm',b'SRGB8_ALPHA8_ENCODED_RGBA8_UNORM']:assert word in native,word
files=['entry/src/main/ets/physics/clearwater/P011ClearWaterSourceController.ets','entry/src/main/ets/render/P013ClearWaterController.ets','entry/src/main/ets/pages/ClearWaterDemo.ets','entry/src/main/cpp/p013renderer/clear_water_gpu.cpp','entry/src/main/cpp/p013renderer/napi_bridge.cpp']
r={'revision':'CW_C_DIAG_PACKAGE_V1','packageSha256':hashlib.sha256(hap.read_bytes()).hexdigest(),'nativeSha256':hashlib.sha256(native).hexdigest(),'sourceSha256':{n:hashlib.sha256((p/n).read_bytes()).hexdigest() for n in files},'routeAndNativeCode':'PASS','device':'NOT_RUN'}
(root/'test/reports/t270/delivery.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
