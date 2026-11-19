"""Validate actual T659 cold DTO + original ReserveAttemptWitnessV1 core, offline."""
from pathlib import Path
import json,sys
from check_owner import owner

def check(prepare,witness):
 assert prepare['revision']=='T659_PREPARE_OWNER_VALUE_V1','OWNER_REVISION';r=prepare['readiness'];assert not r.get('detailTruncated') and r['revision']==prepare['revision'],'OWNER_DETAIL_MISSING'
 o=r['owner'];key=owner(o);assert key==r['ownerKey'] and r['ownerSerial']>0,'OWNER_SERIAL_KEY'
 a=r['authority'];s=a['snapshot'];assert a['prepareId']==o['prepareId'] and a['pageGeneration']==o['pageGeneration'],'PREPARE_SCOPE'
 for k in ('windowId','windowGeneration','controller','session','surface','context','run'):assert s[k]==o[k],'NATIVE_OR_WINDOW_OWNER_CHANGED'
 assert witness['revision']=='ReserveAttemptWitnessV1' and witness['coreBytes']==256,'WITNESS_SCHEMA'
 cores=[x for x in [witness['first'],witness['last']] if len(x)==64 and x[1]==o['prepareId'] and x[2]==o['pageGeneration'] and x[14]==r['ownerSerial']]
 assert any(c[9]==1 and c[12]==1 and c[13]==1 and c[15]==0 for c in cores),'NO_CURRENT_RESERVED_CONSUMED_TICKET'
 return {'ownerTenFields':'CONSISTENT','reserveWitness':'CURRENT_RESERVED_AND_CONSUMED','privateTicket':'NOT_EXPORTED; actual gate one-shot separately exercised by owner_units.cjs','productPASS':False}
def selftest():
 from check_owner import FIELDS
 import copy
 o={k:i+1 for i,k in enumerate(FIELDS)};o['run']='fixture';r={'revision':'T659_PREPARE_OWNER_VALUE_V1','owner':o,'ownerKey':owner(o),'ownerSerial':1,'authority':{'prepareId':o['prepareId'],'pageGeneration':o['pageGeneration'],'snapshot':dict(o)}};p={'revision':r['revision'],'readiness':r};core=[0]*64;core[1]=o['prepareId'];core[2]=o['pageGeneration'];core[9]=core[12]=core[13]=core[14]=1;w={'revision':'ReserveAttemptWitnessV1','coreBytes':256,'first':core,'last':core};check(p,w);n=1
 for at in [1,2,9,12,13,14,15]:
  b=copy.deepcopy(w);b['first'][at]=b['last'][at]=99
  try:check(p,b)
  except AssertionError:n+=1
  else:raise AssertionError(at)
 print(json.dumps({'result':'PASS','fixtureCases':n,'device':'NOT_RUN'}))
if __name__=='__main__' and sys.argv[1:]==['--selftest']:selftest()
elif __name__=='__main__':print(json.dumps(check(*[json.loads(Path(p).read_text(encoding='utf8')) for p in sys.argv[1:3]]),indent=2))
