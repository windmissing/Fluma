"""Exercise the real runner and process boundary with synthetic app/screen commands."""
import copy
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
from fixtures import artifact, write
from capture_hold import sha, verify_bundle


def command(root, output, action, png):
    source = json.loads((root/'fixture.json').read_text());spec=source['spec']
    state_path=root/'state.json';s=json.loads(state_path.read_text())
    if action=='reserve':s.update(state='HOLD_OPEN',startMonoMs=time.monotonic_ns()/1e6,actionSerial=2)
    if action=='export':
        m=json.loads((root/spec['manifest']).read_text());m['capture']=copy.deepcopy(s)
        p=output/spec['manifest'];write(p,m);digest=sha(p);p.with_suffix('.sha256').write_text(digest+'  '+p.name)
        for k in ('candidate','back'):shutil.copyfile(root/m[k]['name'],output/m[k]['name'])
        s.update(archiveState='NATIVE_ARCHIVE_SAVED',manifestSha256=digest,actionSerial=3)
        response={'name':p.name,'sha256':digest}
    elif action=='reserve':response={**s,'revision':'T413_CAPTURE_REPLY_V2','state':'REJECTED' if source['mode']=='old-open' else 'RESERVED','holdId':''}
    elif action=='clock':
        s['sampleSerial']=s.get('sampleSerial',0)+1
        response={**{k:s[k] for k in ('clockId','bootSession','attemptId','holdId','sampleSerial')},
                  'revision':'T413_CLOCK_SAMPLE_V2','unit':'ms','appMonoMs':time.monotonic_ns()/1e6,'continuousForeground':True}
    elif action=='pull':
        if source['mode']=='pull-fail':return 17
        if source['mode']!='missing-png':shutil.copyfile(root/spec['png'],png)
        response={}
    elif action=='seal':
        s.update(state='SEALED',endMonoMs=time.monotonic_ns()/1e6,endFrame=s['frame'],endToken=s['token'],actionSerial=4)
        p=output/spec['hold'];write(p,{'revision':'T413_DISPLAY_HOLD_V2','valid':True,'firstFailure':'','hold':s,
                                      **{k:s[k] for k in ('geometry','startMonoMs','endMonoMs','endFrame','endToken')}})
        response={'name':p.name,'sha256':sha(p)}
    elif action=='release':s.update(state='RELEASED');response=s
    elif action=='cancel':s.update(state='INVALID');response=s
    else:response=s
    write(state_path,s);print(json.dumps(response));return 0


def run():
    results=[]
    with tempfile.TemporaryDirectory() as tmp:
        for mode in ('success','pull-fail','missing-png','old-open'):
            root=Path(tmp)/mode;root.mkdir();output=root/'output'
            model={'native':[118,68],'coverage':[4,4,118,68],'rule':'PIXEL_CENTER_NEAREST','mirrorX':False,'mirrorY':False}
            path=artifact(root,'cal',model);spec=json.loads(path.read_text());hold=json.loads((root/spec['hold']).read_text())['hold']
            hold.update(state='IDLE',archiveState='NATIVE_ARCHIVE_PENDING',manifestSha256='')
            write(root/'state.json',hold);write(root/'fixture.json',{'spec':spec,'mode':mode})
            spec.update(registeredGeometry=hold['geometry'],deviceId='SYNTHETIC',toolConfiguration='SYNTHETIC_PNG')
            cfg={'deviceId':'SYNTHETIC','toolConfiguration':'SYNTHETIC_PNG','clockUncertaintySource':'same-host monotonic fixture',
                 'clockQuantizationMs':.000001,'spec':spec,'commands':{action:[sys.executable,str(Path(__file__).resolve()),str(root),
                  '{output}',action,'{localPng}'] for action in ('reserve','status','export','clock','screenshot','pull','seal','cancel','terminal','release')}}
            config=root/'config.json';write(config,cfg)
            p=subprocess.run([sys.executable,str(Path(__file__).with_name('capture_hold.py')),str(config),str(output)],capture_output=True,text=True)
            if mode=='success':
                if p.returncode!=0:
                    verify_bundle(output/'spec.json')
                    raise AssertionError((p.stdout,p.stderr))
                assert verify_bundle(output/'spec.json')['samples']==3
            else:
                assert p.returncode==1,(p.stdout,p.stderr)
                assert not list(output.glob('*-seal.json'))
                assert json.loads((output/'failure.json').read_text())['reason']=={'pull-fail':'COMMAND_FAILED_pull','missing-png':'PNG_MISSING','old-open':'RESERVE_REJECTED'}[mode]
                if mode=='old-open':assert not list(output.glob('*-cancel.json'))
            results.append({'mode':mode,'result':'PASS'})
    print(json.dumps({'revision':'T414_RUNNER_PROCESS_UNITS_V2','passed':len(results),'failed':0,'cases':results,'device':'NOT_RUN'}))


if __name__=='__main__':
    if len(sys.argv)>1:raise SystemExit(command(Path(sys.argv[1]),Path(sys.argv[2]),sys.argv[3],Path(sys.argv[4])))
    run()
