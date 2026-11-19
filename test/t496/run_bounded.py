"""Single named C arm; bounded parent wait handle, no device access."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,sys,time,struct
import psutil
ROOT=Path(__file__).resolve().parents[2]
def fingerprint(p):
 p=Path(p);return {'path':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'bytes':p.stat().st_size}
def run(arm,attempt):
 assert arm in ('C0','C1','C2','D1_OFF','D1_ON','D2') and attempt and all(c.isalnum() or c in '-_' for c in attempt)
 out=ROOT/'test/reports/t496'/attempt;out.mkdir(parents=True,exist_ok=False)
 exe=ROOT/'.codex/local-test/t496/t496.exe';base=ROOT/'.codex/local-test/t496'
 from source_manifest import collect
 (out/'source-closure.json').write_text(json.dumps(collect(),indent=2))
 env=os.environ.copy();env['PATH']='C:/Users/admin/AppData/Local/Programs/CokePLUS;'+env['PATH']
 start=time.monotonic();deadline=start+360 # leave 60s of total420 for cleanup
 parent=psutil.Process();launch={'arm':arm,'attempt':attempt,'scriptRevision':'T496_SCRIPT_V4_MILESTONE_COMPLETE','scriptWallBudgetSeconds':120,'drainWallBudgetSeconds':30,'runId':'t496-'+attempt,'argv':[str(exe),arm,attempt],'cwd':str(ROOT),'parentPid':os.getpid(),'parentStart':parent.create_time(),'PE':fingerprint(exe),'clock':'PYTHON_MONOTONIC_SECONDS','launch':start,'killDeadline':deadline,'totalDeadline':start+420,'path':env['PATH']}
 with (out/'stdout.bin').open('xb') as stdout,(out/'stderr.bin').open('xb') as stderr:
  p=subprocess.Popen(launch['argv'],cwd=ROOT,env=env,stdout=stdout,stderr=stderr,creationflags=0x08000000)
  child=psutil.Process(p.pid);launch.update(pid=p.pid,childStart=child.create_time());(out/'launch.json').write_text(json.dumps(launch,indent=2))
  modules=[];samples=[];kill=None;reason=None
  while p.poll() is None and time.monotonic()<deadline:
   elapsed=time.monotonic()-start
   sink=base/('gltrace-'+attempt+'.ndjson');active=base/(attempt+'.active')
   if elapsed>5 and (not sink.exists() or not active.exists()):reason='MISSING_SAME_ATTEMPT_SINK_OR_ACTIVE';break
   if sink.exists() and sink.stat().st_size:
    with sink.open('rb') as trace:first=trace.readline(4096)
    try:identity=json.loads(first)
    except Exception:identity={}
    if identity.get('pid')!=p.pid or identity.get('createAttemptId')!=attempt or not identity.get('ownerTid'):reason='SINK_IDENTITY_MISSING_OR_MISMATCH';break
   if int(elapsed*10)%10==0:
    try:
     paths={m.path for m in child.memory_maps() if any(k in m.path.lower() for k in ('angle','egl','gles','d3d','vulkan'))}
     known={m['path'] for m in modules};modules += [fingerprint(x) for x in sorted(paths-known) if Path(x).is_file()]
    except (psutil.Error,OSError) as e:launch['moduleReadError']=str(e)
   if active.exists():
    raw=active.read_bytes()
    if len(raw)>=56 and struct.unpack_from('<Q',raw,48)[0]:reason='TRACE_EVIDENCE_BUDGET';break
    if len(raw)>=48:
     values=struct.unpack('<6Q',raw[:48]);samples.append({'at':time.monotonic(),'version':values[0],'phase':values[1],'function':values[2],'callId':values[3],'ownerTid':values[4],'hostSteadyNs':str(values[5]),'consistentEvenVersion':values[0]%2==0})
   if len(samples)>4200:reason='SAMPLE_BUDGET';break
   time.sleep(.1)
  if p.poll() is None:
   # Popen retains the actual process handle; creation time check guards reuse.
   matched=child.is_running() and child.create_time()==launch['childStart'];kill={'request':time.monotonic(),'targetPid':p.pid,'startMatched':matched,'reason':reason or 'WATCHDOG_DEADLINE'}
   if matched:p.kill();kill['apiResult']='RETURNED'
  code=p.wait(timeout=max(.1,start+420-time.monotonic()))
 end=time.monotonic();active=base/(attempt+'.active');evidenceBudget=active.exists() and active.stat().st_size>=56 and struct.unpack_from('<Q',active.read_bytes(),48)[0]!=0
 result={'exitCodeSigned':code,'exitCodeUnsigned':code&0xffffffff,'exitCodeHex':f'0x{code&0xffffffff:08x}','waitObservedAt':end,'elapsedSeconds':end-start,'watchdog':kill,'evidenceBudgetExceeded':evidenceBudget,'modules':modules,'stackTool':{'path':'C:/Program Files (x86)/Windows Kits/10/Debuggers/x64/cdb.exe','available':False,'result':'NOT_AVAILABLE; unhandled exceptions use same-child MiniDumpNormal, no driver attribution'},'samples':samples,'productPASS':False,'outcome':'CHILD_EXITED' if not kill else 'WATCHDOG_TERMINATED','device':'NOT_RUN'}
 for name in ['gltrace-'+attempt+'.ndjson','t496_log_'+attempt+'.ndjson','t496-stages_'+attempt+'.ndjson',attempt+'.active',attempt+'.dmp']:
  source=base/name
  if source.exists():
   limit=67108864 if name.endswith('.dmp') else 33554432
   assert source.stat().st_size<=limit;data=source.read_bytes();(out/name).write_bytes(data)
 result['artifacts']=[fingerprint(x) for x in out.iterdir() if x.is_file()]
 (out/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps({k:v for k,v in result.items() if k not in ('samples','artifacts','modules')}));return 0 if code==0 and not evidenceBudget else 1
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('arm',choices=['C0','C1','C2','D1_OFF','D1_ON','D2']);p.add_argument('attempt');a=p.parse_args();sys.exit(run(a.arm,a.attempt))
