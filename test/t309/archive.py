from pathlib import Path
import json,hashlib
r=Path(__file__).resolve().parents[2];p=r/'test/reports/t309';project=r/'.codex/local-test/t309-final'
sha=lambda b:hashlib.sha256(b).hexdigest()
unit=json.loads((p/'units.json').read_text());reg=json.loads((p/'regression.json').read_text());assert unit['failed']==reg['failed']==0
sign=json.loads((p/'sign.json').read_text());assert sign['sign']==sign['verify']=='PASS'
log=(r/'.codex/local-test/t309-final-package.log').read_text(errors='replace');assert 'BUILD SUCCESSFUL' in log
(p/'package-tail.txt').write_text('\n'.join(log.splitlines()[-6:]))
manifest=json.loads((project/'t309-manifest.json').read_text());files=['physics/clearwater/diagnostics/ClearWaterDiagnostic.ets','physics/clearwater/diagnostics/ClearWaterDiagnosticV1.ets','physics/clearwater/diagnostics/CriticalCold.ets','physics/clearwater/P011ClearWaterEngine.ets','pages/ClearWaterDemo.ets']
manifest['finalBuiltSources']=[{'file:f,'sha256':sha((project/'entry/src/main/ets'/f).read_bytes())} for f in files];(p/'manifest.json').write_text(json.dumps(manifest,indent=2))
hap=r/'.codex/local-test/t257-sign/T309/T309-signed.hap';assert sha(hap.read_bytes())==sign['signedSha']
(p/'delivery.json').write_text(json.dumps({'task':'T309','contract':'CW_C_DIAG_V2','units':unit['passed'],'regression':reg['passed'],'failed':0,'build':'PASS','sign':sign,'signedHap':str(hap),'project':str(project),'typedPayloadBytes':8576704,'criticalStringBudgetBytes':8388608,'coldSegmentBudgetBytes':33554432,'fortySecond60HzCriticalRows':14401,'criticalCapacity':16384,'lowCapacity':256,'device':'NOT_RUN','opticalVisibility':'NOT_EVALUATED','driverRss':'NOT_MEASURED'},indent=2))
