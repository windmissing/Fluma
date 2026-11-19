"""Read existing T496 source/evidence only; never launches the fixture or a product."""
from pathlib import Path
import json,hashlib,sys
R=Path(__file__).resolve().parents[1]
def fp(p):
 b=(R/p).read_bytes();return dict(path=p,bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
def txt(p):return (R/p).read_text(encoding='utf-8-sig')
def marks(p,needles):
 lines=txt(p).splitlines();return dict(path=p,sha256=fp(p)['sha256'],matches=[dict(line=i+1,text=s) for i,s in enumerate(lines) if any(n in s for n in needles)])
def collect():
 driver='test/t496/external_source_diff.cpp';s=txt(driver)
 run='t496-1218097213334300'
 log='.codex/local-test/t496/t496_log.ndjson';stages='.codex/local-test/t496/t496-stages.ndjson'
 rows=[json.loads(x) for x in txt(log).splitlines() if x.strip()];rows=[x for x in rows if x.get('runId')==run]
 stage=[json.loads(x) for x in txt(stages).splitlines() if x.strip()];stage=[x for x in stage if x.get('runId')==run]
 assert len(rows)==3 and [x.get('phase') for x in rows]==['START','COORDINATOR_WAIT_ITER','COORDINATOR_WAIT_ITER']
 assert len(stage)==1 and stage[0]['stage']=='D1_OFF' and stage[0]['event']=='ENTER'
 assert 'ownerProgressSession="+std::to_string(a.progress.session)' in s
 init=s.index('if (!t496_sink_initialize(');main=s.index('int main(')
 assert init<main and 't496_sink_initialize(' not in s[main:]
 assert 'if (arm == "C0" || arm == "C1" || arm == "C2")' in s
 assert not (R/'.codex/local-test/t496/gltrace-62012.ndjson').exists()
 paths=[driver,'test/t496/t561_seams.h','test/t496/gl_sink.inc','test/t496/gl_forward_sink.cpp','test/t496/run_create_only.cmd','test/t496/build.cmd',
  'docs/test-report-T496-create-only-v6.md','docs/architecture-T620-v1.md',log,stages,'.codex/local-test/t496/t496-v6-summary.json','.codex/local-test/t496/build-t620.log','.codex/local-test/t496/t496.exe',
  '.codex/local-test/t492-m1-delivery/entry/src/main/cpp/gpu_resident/app_bridge.cpp','.codex/local-test/t492-m1-delivery/entry/src/main/cpp/gpu_resident/boot_probe.h',
  '.codex/local-test/t492-m1-delivery/entry/src/main/cpp/gpu_resident/m1/full_transport.cpp']
 return dict(schema='T646_SOURCE_AND_SAVED_EVIDENCE_V1',date='2026-11-19',deviceUsed=False,fixtureRun=False,productTestsRun=False,
  reportAttempt='62012',actualRunId=run,actualBranch='D1_OFF (not C0/C1/C2 CREATE_ONLY)',matchedRows=rows,matchedStages=stage,
  trace62012Present=False,currentLastGlEnter=None,currentLastGlExit=None,currentProgramRole=None,currentWaitChain=None,
  conclusion='TEST_FACILITY_REPAIR_AND_PRECISE_EVIDENCE_FIRST; no established new product stage gap',
  facts=['hostNow in RunD1 is real steady_clock; scriptNs field remains script domain1000000','ownerProgressSession is progress.session, not a progress counter',
   'captureCommittedEpoch0 is publication absent at two sampled cuts, not proof no resource allocation occurred','v4c unpaired411 belongs older identities, not this run',
   'default main does not initialize sink; only recognized create-only dispatch does','real source already has bootProbe.Copy, TRANSPORT_CREATE pair, submit observer, scene pair, Resources and capture publication cuts'],
  inputs=[fp(p) for p in paths],
  sourceCuts=[marks(driver,['hostNow=','hostNow =','ownerProgressSession=','COORDINATOR_WAIT_ITER','RESOURCE_COMMITTED','PagePulseLocked(bound','t496_sink_initialize(','int main(','arm == "C0"']),
   marks(paths[-3],['TRANSPORT_CREATE','SCENE_PROGRAM_PREPARE','recovery.Resources(','captureCommittedEpoch=seen','gpu->submitObserver=']),
   marks('test/t496/run_create_only.cmd',['ATTEMPTID','while(','Start-Process'])])
def main():
 out=R/'docs/analysis-T646-evidence-v1.json';value=collect()
 if '--record' in sys.argv:out.write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 else:assert json.loads(out.read_text(encoding='utf-8'))==value
 print('PASS: T646 saved evidence/source attribution only; no fixture/product/device run')
if __name__=='__main__':main()
