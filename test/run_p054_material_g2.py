"""T54 offline execution consumes T53 frozen expectations; never rewrites T53 assets."""
import json, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'test/reports/t54'
OUT.mkdir(exist_ok=True)
def run(name,cmd):
    p=subprocess.run(cmd,cwd=ROOT,capture_output=True)
    output=p.stdout.decode('utf-8',errors='replace')+p.stderr.decode('utf-8',errors='replace')
    (OUT/name).write_text(output,encoding='utf-8')
    if p.returncode: raise RuntimeError(f'{cmd}: exit {p.returncode}\n{output[-2500:]}')
    return output
core=run('core.txt',['cmd','/c','test\\run_p054_material_core.cmd'])
bridge=run('bridge.txt',['cmd','/c','test\\run_p054_material_bridge.cmd'])
controller=run('controller.json',['node','test/run_p054_material_controller_units.cjs'])
frozen=json.loads((ROOT/'test/reports/p053_g2_uploader_case_check.json').read_text(encoding='utf-8'))
blocked={'A04':'appearanceConfigRevision comparison authority unspecified (architect ruling requested)',
         'A05':'materialBundleId/generation conflict definition unspecified (architect ruling requested)',
         'C02':'domain and zero-work/pending retention verified, exact receipt code unresolved; temporary explicit exception, not final contract'}
partial={'B02':'active descriptor/textures retained; actual material draw belongs G3',
         'C01':'complete-row and 65536-byte limit proven; old active material draw requires G3',
         'D02':'atomic owned generation+metadata/token proven; complete palette/uniform/fog consumption requires G3/G4',
         'E01':'isolated same-tick callbacks proven; product page integration is G4',
         'E02':'isolated same-tick callbacks proven; actual profile/swap/G4 presentation not tested'}
cases=[]
for c in frozen['cases']:
    key=c['id'].split('-')[-1]
    status='BLOCKED_CONTRACT' if key in blocked else 'PARTIAL' if key in partial else 'PASS_OFFLINE'
    cases.append(dict(id=c['id'],expectation=c['expectation'],status=status,
        detail=blocked.get(key,partial.get(key,'Production native CPU/core/bridge paths executed with synthetic GPU/JS boundary; no device claim'))))
report=dict(scope='G2 isolated implementation; synthetic opaque bytes, NAPI/GPU boundary; NOT_DEVICE_VALIDATED',
    cases=cases,summary={s:sum(c['status']==s for c in cases) for s in ['PASS_OFFLINE','PARTIAL','BLOCKED_CONTRACT']},
    pending='A04/A05/exact C02 receipt ruling required before T54 completion/interface freeze',
    preservation='T53 checker/case JSON and T09 fixtures not modified by this executor')
(OUT/'cases.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report['summary']))
