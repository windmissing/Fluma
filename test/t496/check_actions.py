"""Offline action identity checks. Not a replacement for nonempty GPU evidence."""
import copy, hashlib, json, re, struct, sys
from pathlib import Path

def check(rows):
 actions=[r for r in rows if str(r.get('actionId','')).isdigit()]
 assert len(actions)==52, 'MISSING_OR_EXTRA_ACTION'
 assert [int(r['actionId']) for r in actions]==list(range(1,53)), 'ORDER_OR_ID'
 identity={(r['runId'],r['arm']) for r in actions}; assert len(identity)==1, 'IDENTITY'
 last=-1; pulse=0; source=0
 for r in actions:
  stamp=int(r['declaredScriptNs']); assert stamp>=last, 'STAMP_ORDER'; last=stamp
  if r['phase']=='PULSE':
   pulse+=1; assert f'pulseSeq={pulse} ' in r['detail'], 'PULSE_SEQ'
   assert 'PAGE_PULSE_ACCEPTED' in r['detail'], 'PULSE_REJECTED'
  if r['phase'] in ('SOURCE_SAND','SOURCE_WATER'):
   source+=1; assert f'srcSeq={source} ' in r['detail'], 'SOURCE_SEQ'
   match=re.search(r'payloadSHA=([0-9a-f]{64}) words=([0-9,]+)',r['detail']); assert match,'PAYLOAD'
   words=[int(x) for x in match[2].rstrip(',').split(',')];assert len(words)==32,'LAYOUT'
   assert hashlib.sha256(struct.pack('<32I',*words)).hexdigest()==match[1],'PAYLOAD_SHA'
   assert words[6]==source and words[0:2]==[1129534023,2], 'SOURCE_LAYOUT'
 assert source==2 and pulse==48,'ACTION_COUNTS'
 return {'result':'ACTION_TABLE_VALID','device':'NOT_RUN','productPASS':False}

def selftest():
 rows=[];p=s=0
 for i in range(1,53):
  kind='PULSE';detail='';stamp=i*16000000
  if i in (7,29):
   s+=1;kind='SOURCE_SAND' if s==1 else 'SOURCE_WATER';w=[1129534023,2]+[0]*30;w[6]=s
   detail=f'srcSeq={s} payloadSHA='+hashlib.sha256(struct.pack('<32I',*w)).hexdigest()+' words='+','.join(map(str,w))+','
  elif i in (19,43):kind='RESERVE_OFF_NOOP'
  else:p+=1;detail=f'pulseSeq={p} code=PAGE_PULSE_ACCEPTED'
  rows.append(dict(actionId=str(i),phase=kind,detail=detail,declaredScriptNs=str(stamp),runId='fixture',arm='fixture'))
 check(rows)
 variants=[]
 variants.append(rows[:-1]);variants.append(rows+[rows[-1]])
 x=copy.deepcopy(rows);x[0],x[1]=x[1],x[0];variants.append(x)
 for at,change in [(0,lambda r:r.update(detail='pulseSeq=99 code=PAGE_PULSE_ACCEPTED')),(6,lambda r:r.update(detail=r['detail'].replace('words=1129534023','words=1129534024'))),(6,lambda r:r.update(detail=r['detail'].rstrip(',').rsplit(',',1)[0]))]:
  x=copy.deepcopy(rows);change(x[at]);variants.append(x)
 for x in variants:
  try:check(x)
  except AssertionError:pass
  else:raise AssertionError('NEGATIVE_ACCEPTED')
 print(json.dumps({'result':'PASS','cases':7,'device':'NOT_RUN'}))

if __name__=='__main__':
 if sys.argv[1:]==['--selftest']:selftest()
 else:print(json.dumps(check([json.loads(l) for l in Path(sys.argv[1]).read_text().splitlines()]),indent=2))
