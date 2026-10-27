"""Compile all isolated phase-A modules with the real ArkTS checker, then restore
the temporary worker registration byte for byte. Does not install/run a HAP.
Run from the project root. Do not run another profile editor concurrently.
"""
from pathlib import Path
import subprocess,os
profile=Path('entry/build-profile.json5')
probe=Path('entry/src/main/ets/physics/clearwater/T188CompileProbe.ets')
source="""import { P011ClearWaterEngine } from './P011ClearWaterEngine';
import { ClearWaterRunConfig } from './P011ClearWaterTypes';
export function compileClearWater(config: ClearWaterRunConfig): P011ClearWaterEngine {
  return new P011ClearWaterEngine(config);
}
"""
original=profile.read_bytes();needle=b'"workers": ['
assert original.count(needle)==1 and not probe.exists()
modified=original.replace(needle,needle+b'\n        "./src/main/ets/physics/clearwater/T188CompileProbe.ets",',1)
env=os.environ.copy();env['JAVA_HOME']=r'C:\Program Files\Huawei\DevEco Studio\jbr';env['PATH']=env['JAVA_HOME']+r'\bin;'+env['PATH']
try:
 probe.write_text(source,encoding='utf-8');profile.write_bytes(modified)
 with open('test/reports/t188/compile.txt','w',encoding='utf-8') as log:
  code=subprocess.run(['powershell','-NoProfile','-Command',"& 'C:/Program Files/Huawei/DevEco Studio/tools/hvigor/bin/hvigorw.bat' --mode module -p product=default -p module=entry@default default@CompileArkTS --no-daemon"],env=env,stdout=log,stderr=subprocess.STDOUT).returncode
finally:
 if profile.read_bytes()==modified:profile.write_bytes(original)
 else:raise RuntimeError('profile concurrently modified; refusing overwrite')
 if probe.exists() and probe.read_text(encoding='utf-8')==source:probe.unlink()
print('CompileArkTS exit',code)
raise SystemExit(code)
