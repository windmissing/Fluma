import copy
import json
from pathlib import Path
import tempfile
from capture_hold import verify_bundle, preflight
from clock import envelope
from screen import legacy, load
from fixtures import artifact, write

cases=[]
root=Path('.codex/local-test/t414-fixtures');root.mkdir(exist_ok=True)


def reject(name, operation, expected=None):
    try:operation()
    except (ValueError,KeyError,OSError,TypeError) as error:
        if expected:assert str(error)==expected,(name,str(error),expected)
        cases.append({'name':name,'result':'REJECTED','reason':str(error)});return
    raise AssertionError(name+' accepted')


model={'native':[118,68],'coverage':[4,4,118,68],'rule':'PIXEL_CENTER_NEAREST','mirrorX':False,'mirrorY':False}
# Each precise negative starts from an independent fresh session and file set.
for i,kind in enumerate(('positive','sealBefore','emptySha','missingPng','oldRevision','sampleScope','repeatedSample','rawMismatch','unbounded','envelopeOutside','manifestMissing','geometryMismatch','pullFailure','badPng')):
    out=root/kind
    if out.exists():
        # Never overwrite previous windows; use a distinct run folder.
        import time
        out=root/(kind+'-'+str(time.monotonic_ns()))
    out.mkdir();spec_path=artifact(out,'cal',model);spec=legacy.read(spec_path)
    cap_path=out/spec['capture'];cap=legacy.read(cap_path);hold_path=out/spec['hold'];hold=legacy.read(hold_path)
    if kind=='positive':
        result=verify_bundle(spec_path);assert result['samples']==3;assert load(spec_path)[1]['probe']['calibration'];
        calibration=legacy.calibrate(spec_path,out/'calibration.json');assert calibration['validationPoints']==186
        cases.append({'name':kind,'result':'PASS','envelope':result});continue
    expected=None
    if kind=='sealBefore':cap['sealCommand']['startMonoNs']=25000000;write(cap_path,cap);expected='CAPTURE_INTERVAL_OUTSIDE_HOLD'
    elif kind=='emptySha':cap['fileSha256']='';write(cap_path,cap);expected='CAPTURE_TOOL_RECEIPT'
    elif kind=='missingPng':(out/spec['png']).unlink()
    elif kind=='oldRevision':cap['revision']='T406_CAPTURE_CALL_V1';write(cap_path,cap);expected='CAPTURE_TOOL_RECEIPT'
    elif kind in ('sampleScope','repeatedSample','rawMismatch','unbounded'):
        samples=legacy.read(out/spec['clockSamples'])
        if kind=='sampleScope':samples[1]['sample']['holdId']='other'
        elif kind=='repeatedSample':samples[1]['sample']['sampleSerial']=samples[0]['sample']['sampleSerial']
        elif kind=='unbounded':samples[1]['sample']['appMonoMs']=samples[0]['sample']['appMonoMs']
        else:samples[1]['hostBeforeNs']+=1
        if kind!='rawMismatch':
            p=out/samples[1]['rawCommand'];record=legacy.read(p);record['stdout']=json.dumps(samples[1]['sample']);write(p,record)
        write(out/spec['clockSamples'],samples);expected='CLOCK_MAPPING_UNKNOWN'
    elif kind=='envelopeOutside':
        hold['hold']['endMonoMs']=hold['endMonoMs']=29;write(hold_path,hold);expected='CAPTURE_INTERVAL_OUTSIDE_HOLD'
    elif kind=='manifestMissing':(out/spec['manifest']).unlink()
    elif kind=='geometryMismatch':cap['postStatus']['geometry']['globalX']+=1;write(cap_path,cap);expected='HOLD_IDENTITY'
    elif kind=='pullFailure':cap['pull']['exitCode']=1;write(cap_path,cap);expected='CAPTURE_TOOL_RECEIPT'
    elif kind=='badPng':
        p=out/spec['png'];p.write_bytes(b'not png');cap['fileBytes']=p.stat().st_size;cap['fileSha256']=legacy.digest(p.read_bytes());write(cap_path,cap)
    reject(kind,lambda:verify_bundle(spec_path),expected)
# Explicit no-overwrite and all command-path checks happen before subprocesses.
reject('existing output rejects stale PNG',lambda:preflight({},root),'FRESH_OUTPUT_REQUIRED')
config={'deviceId':'synthetic','toolConfiguration':'fixture','clockUncertaintySource':'fixture','clockQuantizationMs':0,
        'spec':{'rotation':0,'screenshotScale':1,'bounds':[0,0,10,10],'screenshotSize':[10,10],'registeredGeometry':{'visible':True}},
        'commands':{key:['definitely-does-not-exist-t414'] for key in ('reserve','status','export','clock','screenshot','pull','seal','cancel','terminal','release')}}
reject('missing command path preflight',lambda:preflight(config,root/'must-not-create'),'COMMAND_PATH_reserve')
assert not (root/'must-not-create').exists()
report={'revision':'T414_TOOL_UNITS_V2','passed':len(cases),'failed':0,'device':'NOT_RUN','cases':cases}
Path('test/reports/t414').mkdir(exist_ok=True);write(Path('test/reports/t414/tools.json'),report)
print(json.dumps(report))
