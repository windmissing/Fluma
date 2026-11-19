"""Verify FilesV2 original index/page SHA and exact contiguous binary streams."""
from pathlib import Path
import base64,hashlib,json,sys,copy
sha=lambda b:hashlib.sha256(b).hexdigest()
def verify(rows):
 entries={};chunks={};export=None;lastPage=-1;done=False;states=[]
 for raw in rows:
  r=json.loads(raw) if isinstance(raw,str) else raw;states.append(r.get('state'))
  if r.get('revision')!='RESOURCE_LAB_FILES_V2' or r.get('state')!='OK':continue
  assert len(json.dumps(r).encode())<=16384,'JSON_BUDGET'
  if 'entries' in r:
   if done:continue # later negative tests use a new export
   if export is None:export=r['exportId']
   assert r['exportId']==export and r['page']==lastPage+1,'INDEX_SEQUENCE';lastPage=r['page'];done=r['done']
   for e in r['entries']:
    assert e['fileId'] not in entries and 0<e['bytes']<=2097152 and len(entries)<32,'INDEX_ENTRY'
    assert Path(e['name']).name==e['name'] and e['name'] not in ('.','..'),'INDEX_PATH';entries[e['fileId']]=e;chunks.setdefault(e['fileId'],bytearray())
  elif 'base64' in r:
   assert r['exportId']==export and r['fileId'] in entries,'PAGE_IDENTITY';e=entries[r['fileId']];b=base64.b64decode(r['base64'],validate=True)
   assert r['offset']==len(chunks[r['fileId']]) and r['offset']%4096==0 and r['totalBytes']==e['bytes'],'PAGE_OFFSET'
   assert len(b)==r['nBytes']==min(4096,e['bytes']-r['offset']) and sha(b)==r['chunkSHA256'],'CHUNK_SHA'
   assert r['eof']==(r['offset']+len(b)==e['bytes']),'PAGE_EOF';chunks[r['fileId']].extend(b)
 assert done and entries and sum(e['bytes'] for e in entries.values())<=67108864,'INCOMPLETE_INDEX'
 for i,e in entries.items():assert len(chunks[i])==e['bytes'] and sha(chunks[i])==e['sha256'],'ORIGINAL_FILE_SHA'
 return {'result':'VERIFIED_FILES_NOT_PRODUCT_PASS','files':len(entries),'bytes':sum(map(len,chunks.values())),'states':states}
def selftest():
 data=b'fixture';s=sha(data);rows=[{'revision':'RESOURCE_LAB_FILES_V2','state':'OK','exportId':'9007199254740993','page':0,'done':True,'entries':[{'fileId':0,'name':'raw.bin','bytes':len(data),'sha256':s}]},{'revision':'RESOURCE_LAB_FILES_V2','state':'OK','exportId':'9007199254740993','fileId':0,'offset':0,'totalBytes':len(data),'nBytes':len(data),'eof':True,'chunkSHA256':s,'base64':base64.b64encode(data).decode()}]
 verify(rows);cases=1
 for field,value in [('exportId','foreign'),('offset',4096),('base64','!!!'),('chunkSHA256','0'*64),('eof',False)]:
  bad=copy.deepcopy(rows);bad[1][field]=value
  try:verify(bad)
  except Exception:cases+=1
  else:raise AssertionError(field)
 try:verify(rows[:1])
 except AssertionError:cases+=1
 else:raise AssertionError('MISSING_PAGE')
 print(json.dumps({'result':'PASS','fixtureCases':cases,'device':'NOT_RUN'}))
if __name__=='__main__':
 if sys.argv[1:]==['--selftest']:selftest()
 else:print(json.dumps(verify([x for x in Path(sys.argv[1]).read_text(encoding='utf8').splitlines() if x.startswith('{')]),indent=2))
