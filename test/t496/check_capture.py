"""Check actual T417/T409 busy-cut capture receipts, no synthetic success."""
import hashlib,json,sys
from pathlib import Path
def check(directory):
 root=Path(directory);launch=json.loads((root/'launch.json').read_text());attempt=launch['attempt']
 result=json.loads((root/'result.json').read_text());rows=[json.loads(l) for l in (root/f't496_log_{attempt}.ndjson').read_text().splitlines()]
 receipts={r['path'].replace('\\','/').rsplit('/',1)[-1]:r for r in result['artifacts']}
 for name in ('launch.json',f't496_log_{attempt}.ndjson',f'gltrace-{attempt}.ndjson'):
  assert hashlib.sha256((root/name).read_bytes()).hexdigest()==receipts[name]['sha256'],'RAW_ARTIFACT_SHA'
 def selected(phase):return [r['detail'] for r in rows if r.get('phase')==phase]
 assert selected('RESERVE_AT_FLUSH_BUSY')==['captureRootBusy=1 Allowed=1'],'REAL_BUSY_CUT'
 reserve=json.loads(selected('RESERVE_RAW')[-1]);end=json.loads(selected('END_RAW')[-1])
 assert reserve['state']=='RESERVED' and end['state']=='FROZEN','RESERVATION_TERMINAL'
 requests=[json.loads(r) for r in selected('REQUEST_RAW')];armed=next(r for r in requests if r['state']=='ARMED')
 assert int(armed['safePointSerial'])>0 and armed['requestId']==reserve['requestId'],'SAFE_POINT'
 pages=[json.loads(r) for r in selected('PAGE_RAW')]
 assert len(pages)==pages[0]['pageCount'] and [p['cursor'] for p in pages]==list(range(len(pages))),'PAGE_COUNT_ORDER'
 assert pages[-1]['complete'] and all(not p['incomplete'] for p in pages),'INCOMPLETE'
 identity=('captureId','packageSha256','epoch','runLo','runHi','pageSession','startNs','endNs')
 assert all(all(p[k]==end[k] for k in identity) for p in pages),'PAGE_IDENTITY'
 assert end['packageSha256']==launch['PE']['sha256'],'PE_IDENTITY'
 kinds=[r['kind'] for p in pages for r in p.get('rows',[])]
 assert 'BEGIN' in kinds and 'END' in kinds,'BEGIN_END_EVENTS' # native Freeze emits END, state is FROZEN
 assert result['exitCodeUnsigned']==0 and not result['evidenceBudgetExceeded'] and result['watchdog'] is None,'EXIT_OR_BUDGET'
 trace=root/f'gltrace-{attempt}.ndjson';assert trace.stat().st_size<=33554432,'TRACE_BUDGET'
 return {'result':'PASS','scope':'actual busy reservation -> safe point ARMED -> complete BEGIN/END pages with FROZEN state','requestId':reserve['requestId'],'safePointSerial':armed['safePointSerial'],'pages':len(pages),'kinds':kinds,'traceBytes':trace.stat().st_size,'elapsedSeconds':result['elapsedSeconds'],'PE':launch['PE']['sha256'],'device':'NOT_RUN','productPASS':False}
if __name__=='__main__':print(json.dumps(check(sys.argv[1]),indent=2))
