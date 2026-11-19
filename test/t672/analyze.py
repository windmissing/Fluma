"""T672 offline strict raw-page decoder. Never makes device calls."""
from pathlib import Path
import base64,hashlib,json,struct,sys,tempfile
sha=lambda b:hashlib.sha256(b).hexdigest()
def read(p):
 raw=p.read_bytes();assert len(raw)<=16384,'RECORD_BUDGET';return raw,json.loads(raw)
def page(p,expected):
 raw,r=read(p);assert r['schema']=='G13InitDiagnosticV1' and r['state']=='SNAPSHOT' and r['page']==expected and r['offsetBytes']==expected*8192 and r['rawBytes']==8192,'PAGE_SCHEMA'
 b=base64.b64decode(r['base64'],validate=True);assert len(b)==8192,'PAGE_BYTES';return b,{'path':p.name,'jsonSHA':sha(raw),'decodedSHA':sha(b),'sampleSeq':r['sampleSeq']}
def analyze(root,round=1):
 root=Path(root);assert sum(p.stat().st_size for p in root.iterdir() if p.is_file())<=1048576,'DIRECTORY_BUDGET'
 raw,c=read(root/'configure.json');attempt=c.get('attempt');assert type(attempt) is str and attempt.isdecimal() and 0<int(attempt)<2**64,'CONFIGURE_TICKET'
 statusFiles=list(root.glob('*status*.json'));assert len(statusFiles)<=4,'STATUS_BUDGET'
 _,s=read(root/f'r{round}-status.json');ticket=s['initialization'].get('firstFailureAttempt','0');ticket=ticket if ticket!='0' else attempt
 before,ba=page(root/f'r{round}-anchor-before.json',3);after,aa=page(root/f'r{round}-anchor-after.json',3)
 parts=[];receipts=[]
 for i in range(4):b,r=page(root/f'r{round}-page{i}.json',i);parts.append(b);receipts.append(r)
 allbytes=b''.join(parts);control=parts[3][4096:];boot,active=struct.unpack_from('<QQ',control,56)
 assert active==int(attempt) or ticket!=attempt,'CONFIG_TICKET_NOT_INSTALLED'
 consistent=before==parts[3]==after
 stages=[]
 for i in range(64):
  row=allbytes[4096+i*256:4096+(i+1)*256];seq,ns,b,a,t,q,w,c,tid=struct.unpack_from('<9Q',row)
  if not seq:continue
  if b!=boot:consistent=False
  stages.append({'seq':str(seq),'hostSteadyNs':str(ns),'boot':str(b),'attempt':str(a),'ticket':str(t),'queue':str(q),'window':str(w),'context':str(c),'tid':str(tid),'rawNapi':struct.unpack_from('<q',row,72)[0],'rawEGL':struct.unpack_from('<q',row,80)[0],'rawGL':struct.unpack_from('<q',row,88)[0],'fence':struct.unpack_from('<q',row,96)[0],'ctrl255':struct.unpack_from('<q',row,104)[0],'ctrl7':struct.unpack_from('<q',row,112)[0],'op':struct.unpack_from('<I',row,120)[0],'apiReturn':struct.unpack_from('<i',row,124)[0],'phase':row[128:160].split(b'\0')[0].decode('utf8'),'edge':row[160:176].split(b'\0')[0].decode('utf8'),'sourceSHA':row[176:241].split(b'\0')[0].decode('ascii')})
 return {'schema':'T672_OFFLINE_DECODE_V1','result':'ANCHORS_EQUAL_NOT_ATOMIC' if consistent else 'INCONSISTENT_SNAPSHOT','productPASS':False,'configureAttempt':attempt,'selectedTicket':ticket,'controlBoot':str(boot),'controlTicket':str(active),'pages':receipts,'anchors':[ba,aa],'stages':stages,'capability':s.get('native',{}).get('capability'),'logs':[{'offset':o,'sha256':sha(allbytes[o:o+4096]),'base64':base64.b64encode(allbytes[o:o+4096]).decode()} for o in (20480,24576)],'rawControlSHA':sha(control),'sampleSeqIsAtomicVersion':False}
def selftest():
 with tempfile.TemporaryDirectory() as d:
  root=Path(d);(root/'configure.json').write_text('{"attempt":"9007199254740993"}');(root/'r1-status.json').write_text('{"initialization":{"firstFailureAttempt":"0"},"native":{"capability":{}}}')
  payload=bytearray(32768);struct.pack_into('<QQ',payload,28672+56,71,9007199254740993)
  for n,i in [(f'r1-page{i}.json',i) for i in range(4)]+[('r1-anchor-before.json',3),('r1-anchor-after.json',3)]:
   (root/n).write_text(json.dumps({'schema':'G13InitDiagnosticV1','state':'SNAPSHOT','page':i,'offsetBytes':i*8192,'rawBytes':8192,'sampleSeq':'0','base64':base64.b64encode(payload[i*8192:(i+1)*8192]).decode()}))
  assert analyze(root)['result']=='ANCHORS_EQUAL_NOT_ATOMIC'
  p=root/'r1-anchor-after.json';r=json.loads(p.read_text());b=bytearray(base64.b64decode(r['base64']));b[0]=1;r['base64']=base64.b64encode(b).decode();p.write_text(json.dumps(r));assert analyze(root)['result']=='INCONSISTENT_SNAPSHOT'
  r['base64']='not base64';p.write_text(json.dumps(r))
  try:analyze(root)
  except Exception:pass
  else:raise AssertionError('BAD_BASE64_ACCEPTED')
 print(json.dumps({'result':'PASS','cases':['wide ticket exact','four complete pages','anchor mismatch','invalid base64'],'device':'NOT_RUN'}))
if __name__=='__main__':
 if sys.argv[1:]==['--selftest']:selftest()
 else:print(json.dumps(analyze(sys.argv[1],int(sys.argv[2]) if len(sys.argv)>2 else 1),indent=2))
