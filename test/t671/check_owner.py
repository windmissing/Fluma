"""T671 offline owner/ticket binding. Existing T413 verifier owns HOLD/PNG gates."""
from pathlib import Path
import json,sys,copy
FIELDS=json.loads((Path(__file__).parents[1]/'reports/t659/owner-contract.json').read_text())['ownerTuple']
def owner(o):
 assert set(FIELDS)<=o.keys(),'OWNER_FIELDS'
 for k in FIELDS[:-1]:assert type(o[k]) is int and 0<=o[k]<=9007199254740991,'OWNER_SAFE_INTEGER'
 for k in ('controller','session','surface','context'):assert o[k]>0,'OWNER_NATIVE_ZERO'
 assert type(o['run']) is str and o['run'],'OWNER_RUN'
 return json.dumps([o[k] for k in FIELDS],ensure_ascii=False,separators=(',',':'))
def ticket(record):
 before,after=record['before'],record['fresh'];key=owner(before)
 assert owner(after)==key,'OWNER_CHANGED'
 t,issued=record['ticket'],record['issued']
 assert t['valid'] is True and record['reserveResult']=='RESERVED' and record['consumeCount']==1,'TICKET_LIFECYCLE'
 for k in ('prepareId','pageGeneration','observationSerial','inputSerial','ownerSerial','owner'):assert t[k]==issued[k], 'ISSUED_TICKET_MISMATCH'
 assert t['prepareId']==before['prepareId'] and t['pageGeneration']==before['pageGeneration'] and t['owner']==key,'TICKET_OWNER'
 assert t['ownerSerial']>0 and t['observationSerial']>0,'TICKET_SERIAL'
 return {'binding':'PASS','holdPNG':'NOT_EVALUATED_USE_T413_VERIFY_BUNDLE'}
def selftest():
 o={k:i+1 for i,k in enumerate(FIELDS)};o['run']='synthetic-not-device';t={'valid':True,'prepareId':o['prepareId'],'pageGeneration':o['pageGeneration'],'observationSerial':1,'inputSerial':1,'ownerSerial':1,'owner':owner(o)}
 r={'before':o,'fresh':dict(o),'ticket':t,'issued':dict(t),'reserveResult':'RESERVED','consumeCount':1};ticket(r);n=1
 for k in FIELDS:
  b=copy.deepcopy(r);b['fresh'][k]=b['fresh'][k]+1 if k!='run' else 'foreign'
  try:ticket(b)
  except AssertionError:n+=1
  else:raise AssertionError(k)
 for k,v in [('reserveResult','REJECTED'),('consumeCount',2)]:
  b=copy.deepcopy(r);b[k]=v
  try:ticket(b)
  except AssertionError:n+=1
  else:raise AssertionError(k)
 print(json.dumps({'fixtureCases':n,'result':'PASS','device':'NOT_RUN'}))
if __name__=='__main__':
 if sys.argv[1:] == ['--selftest']:selftest()
 else:print(json.dumps(ticket(json.loads(Path(sys.argv[1]).read_text(encoding='utf8')))))
