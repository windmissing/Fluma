"""Join original dry SOURCE sidecars and exercise independent evidence negatives."""
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
from tempfile import TemporaryDirectory

ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'test/reports/t830'

def require(ok,reason):
    if not ok:
        raise ValueError(reason)

def join(accepted,queued,retired):
    scope=('pid','processStartTick','ticket','ownerTid','surface','geometry','m1SourceModuleSHA')
    require(all(accepted[k]==queued[k]==retired[k] for k in scope),'STALE_SCOPE')
    w=accepted['m1Completion']
    require(len(w)==256 and tuple(w[:4])==(1381192263,2,256,3235823838),'BAD_COMPLETION')
    nonce=w[4]+(w[5]<<32)
    require(nonce and str(nonce)==accepted['m1Nonce']==queued['m1Nonce']==retired['m1Nonce'],'NONCE_MISMATCH')
    require(queued['m1Completion']==retired['m1Completion']==w,'COMPLETION_DRIFT')
    require(accepted['m1OwnerCode']==0 and accepted['m1QueueVkResult']==0 and
            accepted['m1FenceVkResult']==0 and 0<int(accepted['m1FenceNs'])<=int(accepted['m1DeadlineNs'])
            and accepted['m1AdmittedQ']==w[61]>0 and w[27]==1 and w[53]>w[52],'UNACCEPTED_OR_EMPTY')
    frame=queued['windowFrame']
    require(frame==retired['windowFrame'] and frame['acceptedRoot']==str(nonce)
            and int(frame['generation'])==queued['m1PublicationGeneration']
            and frame['publicationSlot']==w[35],'PRESENT_IDENTITY')
    require(all(frame[k]==0 for k in ('acquire','submit','fence','present','imagePresent'))
            and frame['pixel']==0xff00ff00 and int(frame['presentEnterQpc'])<=int(frame['presentExitQpc'])
            and retired['m1PublicationReleased'] is True,'PRESENT_OR_RETIRE')
    return {'nonce':str(nonce),'completionSHA256':hashlib.sha256(struct.pack('<256I',*w)).hexdigest(),
            'admittedQ20':w[61],'scope':{k:accepted[k] for k in scope}}

def main():
    raw=(BASE/'m1-source-window.jsonl').read_bytes()
    receipt=json.loads((BASE/'m1-source-window-receipt.json').read_bytes())
    digest=hashlib.sha256(raw).hexdigest()
    require(digest==receipt['stdoutSHA'] and receipt['exitCode']==0,'ORIGINAL_RECEIPT')
    rows=list(map(json.loads,raw.splitlines()))
    args=[next(r for r in rows if r['stage']=='M1_SOURCE_WSI' and r['state']==state)
          for state in ('RUNNING','M1_DRY_SOURCE_PRESENT_QUEUED','M1_DRY_SOURCE_PRESENT_RETIRED')]
    require(args[-1]['pid']==receipt['pid'],'PID_RECEIPT')
    positive=join(*args)
    controls=[
        (1,'geometry','999','STALE_SCOPE'),(2,'m1Nonce','99','NONCE_MISMATCH'),
        (0,'m1OwnerCode',1,'UNACCEPTED_OR_EMPTY'),(2,'m1PublicationReleased',False,'PRESENT_OR_RETIRE')]
    tests=[]
    for index,key,value,expected in controls+[(1,'windowFrame',dict(args[1]['windowFrame'],acceptedRoot='99'),'PRESENT_IDENTITY')]:
        altered=deepcopy(args)
        altered[index][key]=value
        try: join(*altered)
        except ValueError as error: require(str(error)==expected,'WRONG_REJECTION')
        else: raise AssertionError(expected)
        tests.append(expected)
    with TemporaryDirectory() as temp:
        from pathlib import Path
        bad=Path(temp)/'short.u32'
        bad.write_bytes(struct.pack('<256I',*args[0]['m1Completion']))
        reader=subprocess.run([sys.executable,str(ROOT/'test/t670/check.py'),str(bad)],capture_output=True,text=True)
        require(reader.returncode!=0 and 'COLD_HEADER' in reader.stderr,'BAD_ARCHIVE_ACCEPTED')
    regression=subprocess.run([sys.executable,str(ROOT/'test/t838/check.py'),'selftest'],check=True,capture_output=True,text=True)
    arms=[str(ROOT/'test/reports/t496'/('t693-t703-compact-'+name))
          for name in ('replay-off','replay-on','primary-off','primary-on')]
    historical=subprocess.run([sys.executable,str(ROOT/'test/t693/compare_replay.py'),*arms],
                              check=True,capture_output=True,text=True)
    print(json.dumps({'sourceSHA256':digest,'receiptPE':receipt['exeSHA'],'observedDrySource':positive,
                      'negatives':tests,'badArchive':'COLD_HEADER','t838Regression':json.loads(regression.stdout),
                      'nativeCold':'NOT_OBSERVED','quantum':'NOT_OBSERVED','D1D2':'T496_SEPARATE',
                      'historicalReplayRegression':json.loads(historical.stdout)},indent=2))

if __name__=='__main__':
    main()

