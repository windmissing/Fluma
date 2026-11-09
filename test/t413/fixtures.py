"""Synthetic developer fixtures; never device evidence."""
import copy
import json
from pathlib import Path
import sys
from screen import legacy

# Reuse unchanged T406 independent raster fixtures, not its V1 time loader.
source_path = Path(__file__).parents[1]/'t406'/'units.py'
source = source_path.read_text(encoding='utf-8-sig').split('\nwith TemporaryDirectory()')[0]
source = source.replace('from screen import calibrate, plan, pair, single, pattern, mapping_point, digest', '')
ns = {'__file__': str(source_path), **{k:getattr(legacy,k) for k in ('calibrate','plan','pair','single','pattern','mapping_point','digest')}}
exec(compile(source,str(source_path),'exec'), ns)


def write(path, data):
    path.write_text(json.dumps(data), encoding='utf8')


def upgrade(spec_path):
    root = spec_path.parent;spec = legacy.read(spec_path);manifest_path = root/spec['manifest'];m = legacy.read(manifest_path)
    p, phase = m['probe'], m['phase'];old = phase['hold'];seq = p['requestId']
    geometry = {'revision':'T413_GEOMETRY_V2','globalX':spec['bounds'][0],'globalY':spec['bounds'][1],
                'widthVp':p['windowWidth'],'heightVp':p['windowHeight'],'vp2px':1,'geometry':p['geometryEpoch'],
                'visible':True,'configuration':'SYNTHETIC_PROTOCOL'}
    hold = {'state':'SEALED','attemptId':old['token']+'-a','holdId':old['token']+'-h','requestId':seq,
            'phaseSequence':seq,'token':old['token'],'frame':seq,'run':'test','owner':'owner-fixture',
            'geometry':geometry,'startMonoMs':10,'endMonoMs':100,'endToken':old['token'],'endFrame':seq,
            'archiveState':'NATIVE_ARCHIVE_SAVED','actionSerial':6,'clockId':'SYNTHETIC_APP_MS','bootSession':'fixture',
            'manifestSha256':''}
    c = copy.deepcopy(hold);c.update(state='HOLD_OPEN',actionSerial=2,archiveState='NATIVE_ARCHIVE_PENDING')
    m['capture'] = c;phase['sample']['frame']=seq
    write(manifest_path,m);hold['manifestSha256']=legacy.digest(manifest_path.read_bytes())
    manifest_path.with_suffix('.sha256').write_text(hold['manifestSha256']+'  '+manifest_path.name)
    write(root/spec['hold'], {'revision':'T413_DISPLAY_HOLD_V2','valid':True,'firstFailure':'','hold':hold,
                              'geometry':geometry,'startMonoMs':10,'endMonoMs':100,'endToken':old['token'],'endFrame':seq})
    clock=[]
    for i,(lo,hi,d) in enumerate(((10,12,11),(14,16,15),(40,42,41))):
        sample={'revision':'T413_CLOCK_SAMPLE_V2','clockId':hold['clockId'],'bootSession':hold['bootSession'],
                'holdId':hold['holdId'],'attemptId':hold['attemptId'],'unit':'ms','sampleSerial':i+1,
                'appMonoMs':d,'continuousForeground':True}
        command={'argv':['SYNTHETIC_CLOCK'],'startMonoNs':lo*1000000,'endMonoNs':hi*1000000,
                 'exitCode':0,'stdout':json.dumps(sample),'stderr':''}
        name=f'{old["token"]}-clock-{i}.json';write(root/name,command)
        clock.append({'sample':sample,'hostBeforeNs':command['startMonoNs'],'hostAfterNs':command['endMonoNs'],
                      'quantizationMs':0,'source':'APP_SAMPLE_HOLD_CLOCK','clockUncertaintySource':'EXACT_SYNTHETIC',
                      'rawCommand':name})
    name=old['token']+'-clocks.json';write(root/name,clock);spec['clockSamples']=name
    pre=copy.deepcopy(hold);pre.update(state='HOLD_OPEN',actionSerial=4)
    receipt={'revision':'T413_CAPTURE_CALL_V2','state':'COMPLETE','clock':'PYTHON_MONOTONIC_NS',
             **{k:hold[k] for k in ('attemptId','holdId','requestId','phaseSequence')},
             'remotePng':'/synthetic/unique.png','localPng':spec['png'],'fileBytes':(root/spec['png']).stat().st_size,
             'fileSha256':legacy.digest((root/spec['png']).read_bytes()),'startMonoNs':20000000,'endMonoNs':30000000,
             'preStatus':pre,'postStatus':copy.deepcopy(pre),
             'screenshot':{'argv':['SYNTHETIC_SCREEN'],'exitCode':0,'startMonoNs':21000000,'endMonoNs':22000000},
             'pull':{'argv':['SYNTHETIC_PULL'],'exitCode':0,'startMonoNs':23000000,'endMonoNs':29000000},
             'sealCommand':{'argv':['SYNTHETIC_SEAL'],'exitCode':0,'startMonoNs':50000000,'endMonoNs':51000000}}
    write(root/spec['capture'],receipt);spec.pop('clockMap');write(spec_path,spec)
    return spec_path


def artifact(root,name,model,kind='CAL',seq=1,tiles=None,registered=None):
    nw,nh=model['native']
    return upgrade(ns['artifact'](root,name,model,kind,seq,tiles or [[0,0,16,16],[nw//2-8,25,16,16],[nw-16,52,16,16]],registered=registered))
