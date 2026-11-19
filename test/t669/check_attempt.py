"""T669 bounded same-attempt input diagnostic validator; offline only."""
from pathlib import Path
import copy,json,sys
KEYS=('route','api','bootStamp','diagGeneration','attemptId')
def check(record):
 assert record['revision']=='T582_INPUT_ATTEMPT_FILE_V1' and record['state']=='COMPLETE','INCOMPLETE_ATTEMPT'
 assert len(json.dumps(record).encode())<=16384,'RECORD_BUDGET'
 request,reply,selected=(json.loads(record[k]) for k in ('request','reply','selected'))
 identity=reply.get('inputCall',reply)
 assert request['route']==identity['route'] and request['api']==identity['api'],'REQUEST_SCOPE'
 assert selected['revision'] in ('GPU_INPUT_DIAGNOSTIC_V1','GPU_INPUT_DIAGNOSTIC_V2'),'DIAGNOSTIC_REVISION'
 for k in KEYS:assert selected[k]==identity[k],'DIFFERENT_ATTEMPT:'+k
 for k in KEYS[2:]:assert type(identity[k]) is str and identity[k].isdecimal() and 0<int(identity[k])<2**64,'WIDE_ID'
 ref=selected.get('firstFailure')
 if ref:
  first=json.loads(record['first']);assert all(first[k]==ref[k] for k in KEYS),'FIRST_FAILURE_SCOPE'
  assert ref['route']==identity['route'] and ref['bootStamp']==identity['bootStamp'] and ref['diagGeneration']==identity['diagGeneration'],'CROSS_PROCESS_FIRST_FAILURE'
 assert request['length']==len(request['words']),'REQUEST_LENGTH'
 return {'result':'SAME_ATTEMPT_EVIDENCE_VALID','stage':selected.get('firstFailedStage'),'rawReturnedLength':selected.get('rawReturnedLength',selected.get('elementLength')),'lengthUnit':selected.get('lengthUnit'),'productPASS':False}
def selftest():
 ident={'route':'M3A','api':'m3Configure','bootStamp':'9007199254740993','diagGeneration':'1','attemptId':'18446744073709551615'};d={**ident,'revision':'GPU_INPUT_DIAGNOSTIC_V2','firstFailedStage':'ELEMENT_COUNT','firstFailure':None};r={'revision':'T582_INPUT_ATTEMPT_FILE_V1','state':'COMPLETE','request':json.dumps({'route':'M3A','api':'m3Configure','length':1,'words':[0]}),'reply':json.dumps(ident),'selected':json.dumps(d),'first':''};check(r);count=1
 for k in KEYS:
  bad=copy.deepcopy(r);v=copy.deepcopy(d);v[k]='foreign';bad['selected']=json.dumps(v)
  try:check(bad)
  except AssertionError:count+=1
  else:raise AssertionError(k)
 print(json.dumps({'result':'PASS','fixtureCases':count,'device':'NOT_RUN'}))
if __name__=='__main__':
 if sys.argv[1:]==['--selftest']:selftest()
 else:print(json.dumps(check(json.loads(Path(sys.argv[1]).read_text(encoding='utf8'))),indent=2))
