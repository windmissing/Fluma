#!/usr/bin/env python3
"""GNR1 cold archive decoder. No live clock, physics execution or device access.
Rejects old profiles; reports replay/physics/visible evidence separately.
"""
from pathlib import Path
import argparse,hashlib,importlib.util,json,mmap,struct,tempfile
ROOT=Path(__file__).resolve().parents[2]
def require(ok,why):
 if not ok:raise ValueError(why)
def digest(raw):return hashlib.sha256(raw).hexdigest()
def packed(words):return struct.pack('<'+'I'*len(words),*words)
def sha_words(words):return digest(packed(words))
def hexwords(words):return packed(words).hex()
def u64(w,at):return w[at]|w[at+1]<<32
def signed(w,at,n=2):
 value=sum(w[at+i]<<(32*i) for i in range(n));return value-(1<<(32*n)) if value>>(32*n-1) else value
def f64(w,at):return struct.unpack('<d',packed(w[at:at+2]))[0]
def words(raw):return list(struct.unpack('<'+'I'*(len(raw)//4),raw))
def eqhash(w,at,raw,why):require(hexwords(w[at:at+8])==digest(raw),why)
def geometry(record):return [0x4b475247,1,827477575,1]+record[8:26]+record[56:60]+record[142:144]+record[100:108]+record[76:82]+record[238:239]+record[144:160]+record[168:176]
def frame(record,commit,pub,pipeline):return [0x57465247,1,827477575]+record[8:26]+words(hashlib.sha256(packed(geometry(record))).digest())+[commit&0xffffffff,commit>>32,pub&0xffffffff,pub>>32]+pipeline

def record_check(r,role,inp,header,current):
 if not any(r):return False
 require(r[:8]==[859199570,1,256,827477575,role,2,3,1] and not any(r[240:]),'RECORD_HEADER_RESERVED')
 require(r[29]==1 and r[125]==1 and r[138]==0 and r[139]>0 and r[139]<=1000000 and r[239]==15,'RECORD_POLICY')
 rd,ru=u64(r,34),u64(r,36);require(ru>=rd and 8333334<=ru-rd<=1000000000 and u64(r,38)==ru-rd,'RECEIPT_DURATION')
 require(0<=u64(r,40)-rd<=8000000 and 0<=u64(r,42)-ru<=8000000 and u64(r,236)==u64(r,42)-ru,'VALIDATION_AGE')
 require(u64(r,126)==125000000 and 0<=u64(r,44)-ru<=250000000 and u64(r,234)==u64(r,44)-ru,'OFFER_AGE')
 require(u64(r,46)>=u64(r,48) and u64(r,50)==u64(r,48)+250000000 and u64(r,232)==u64(r,46)-ru and 0<=u64(r,232)<=250000000,'FREEZE_AGE')
 require(r[76:82]==r[94:100] and f64(r,78)==0 and f64(r,80)==0 and r[137]<=256,'MAPPING_POLICY')
 for rawat,modelat in [(64,108),(82,112)]:
  x,y,scale=f64(r,rawat),f64(r,rawat+2),f64(r,rawat+12);left,top,width,height=[f64(r,i) for i in [100,102,104,106]]
  px,py=x*scale,y*scale;require(width>0 and height>0 and left<=px<left+width and top<=py<top+height,'HALF_OPEN_MAPPING')
  mx=((px-left)/width)*128;my=((top+height-py)/height)*256;require(packed(words(struct.pack('<d',mx)))==packed(r[modelat:modelat+2]) and struct.pack('<d',my)==packed(r[modelat+2:modelat+4]),'MAPPED_BINARY64')
 require(f64(r,116)==f64(r,112)-f64(r,108) and f64(r,118)==f64(r,114)-f64(r,110),'DISPLACEMENT')
 dx,dy=int(abs(f64(r,116))*1048576),int(abs(f64(r,118))*1048576);require(dx*dx+dy*dy>=262144**2,'DEADZONE')
 sx=2*dx>24*1048576;sy=f64(r,118)>0 and 2*dy>32*1048576;vx=min(2*dx,24*1048576);vy=min(2*dy,32*1048576) if f64(r,118)>0 else 0
 jx=65536*vx*(-1 if f64(r,116)<0 else 1);jy=65536*vy;cap=(vx*vx+vy*vy)<<43
 require(signed(r,128)==jx and u64(r,130)==jy and sum(r[132+i]<<(32*i) for i in range(4))==cap and r[136]==int(sx)+2*int(sy),'SPATIAL_RECIPE')
 for at,commit,pub in [(192,u64(header,32 if current else 36),u64(r,60)),(200,u64(header,34 if current else 38),u64(r,62))]:eqhash(r,at,packed(frame(r,commit,pub,header[120:128])),'FRAME_WITNESS')
 if current:
  require(inp[0:3]==[1481788999,3,827477575] and inp[40]==2 and inp[77]==63701 and inp[45]==8+r[136] and u64(inp,43)==ru and inp[56:64]==r[128:136],'INPUT_RECIPE_LINK')
  eqhash(r,176,packed(inp),'FROZEN_INPUT_SHA');stable=[0x43535247,1,inp[2]]+inp[4:6]+inp[12:14]+inp[26:34]+inp[40:128];require(len(stable)==103,'STABLE_DICTIONARY');eqhash(r,184,packed(stable),'STABLE_BODY_SHA')
  require(r[144:152]==header[144:152] and r[152:160]==header[152:160] and r[224:232]==header[216:224],'RECORD_CURRENT_IDENTITIES')
 return True

def state_check(state,k,config):
 require(state[:3]==[844317767,2,827477575] and state[24:28]==[k,128,64,64] and state[106:114]==config[12:20] and not any(state[114:256]),'STATE_IDENTITY')
 origins=[0]*4096;total=0;momentum=[0,0];packetMass=[0]*128;ids=set();source=(280576+256*k)//4;count=state[19];require(count<=4096,'ORIGIN_COUNT')
 for c in range(128):
  col=state[512+32*c:544+32*c];s,f=col[:2];p=signed(col,2);require(s+f<=268435456 and col[9]==s+f and col[4]==int(p!=0) and abs(p)<=f*1073741824,'COLUMN_GEOMETRY_MOMENTUM');total+=s+f;momentum[0]+=p
  for flow,expected,depth in [(False,s,col[6]),(True,f,col[7])]:
   require(depth<=64,'MAP_CAPACITY');summed=0;previous=None;base=(37376 if flow else 4608)+256*c
   for rank in range(64):
    row=state[base+4*rank:base+4*rank+4]
    if rank>=depth:require(not any(row),'MAP_ZERO_TAIL');continue
    origin,hi,sourceid,mass=row;require(1<=origin<=count and hi==0 and mass>0 and sourceid==state[source+16*(origin-1)+2],'ORIGIN_KEY');key=tuple(row[:3]);require(not flow or previous is None or previous<key,'FLOW_ORDER');previous=key;summed+=mass;origins[origin-1]+=mass
   require(summed==expected,'PHASE_MAP_MASS')
 for slot in range(k):
  row=state[(280576//4)+slot*64:(280576//4)+slot*64+64];mass=row[0]
  if not mass:require(not any(row[:7]+row[8:]),'FREE_SLOT_ZERO');continue
  require(0<mass<=244666 and row[5]<128 and row[1]//1048576==row[5] and row[6]>0 and row[7]>0,'PARTICLE_CERTIFICATE');require(0<row[8]<=count and row[9]==0,'PARTICLE_ORIGIN');require(row[6] not in ids and row[17:22]==[row[6],row[7],row[8],row[9],row[10]],'PARTICLE_ID_CERTIFICATE');ids.add(row[6]);column=state[512+32*row[5]:544+32*row[5]];require(2*(column[0]+column[1])+mass<=row[2]<=536870912-mass,'NONEMBEDDED_PARTICLE');packetMass[row[5]]+=mass;origins[row[8]-1]+=mass;total+=mass;momentum[0]+=mass*signed(row,3,1);momentum[1]+=mass*signed(row,4,1)
 for c in range(128):require(packetMass[c]==state[512+32*c+8],'COLUMN_PARTICLE_CACHE')
 declared=0
 for origin in range(4096):
  row=state[source+16*origin:source+16*(origin+1)]
  if origin>=count:require(not any(row),'SOURCE_ZERO_TAIL');continue
  require(row[0]==origin+1 and row[1]==0 and row[2]>0 and row[14]==1 and u64(row,4)==origins[origin],'PER_ORIGIN_MASS');declared+=u64(row,4)
 require(total==declared==u64(state,40),'GLOBAL_MASS');return momentum

def stream_check(stream,event_count,path_count,profile,nonce):
 require(event_count<=profile['eventBytes']//256 and path_count<=(profile['arenaBytes']-profile['pathByteOffset'])//64,'STREAM_CAPACITY')
 dm=0;predict=0;spec=importlib.util.spec_from_file_location('existing_dm_oracle',ROOT/'test/t556/independent_oracle.py');oracle=importlib.util.module_from_spec(spec);spec.loader.exec_module(oracle)
 for i in range(event_count):
  row=stream[64*i:64*(i+1)];require(row[0]==843400263 and row[1]==2 and u64(row,4)==nonce and row[6]==i+1 and row[63]==3235776802,'EVENT_IDENTITY')
  if row[2]==15:
   require(row[7]==22 and not any(row[51:63]),'DM_EVENT_LAYOUT');s,f,p,gn,chi=row[15],row[16],signed(row,17),row[20],row[21];j=1<<row[3]
   q,upper,rounds,h0,omega0,branch=oracle.solve_physical(f,p,gn,chi,s+f,s,j);hat,omega,feasible,*_=oracle.eval_q(q,f,p,gn,chi,s+f,j)
   require(feasible and row[25]==h0 and row[26]==omega0 and row[27]==q and row[30]==hat and row[31]==omega and row[46]==upper and row[47]==q and row[48]==q+1 and row[35]==rounds,'DM_MAXIMAL_Q')
   require(row[33]==q+1 and row[34]==0 and (q+1>s or q+1>upper or not oracle.eval_q(q+1,f,p,gn,chi,s+f,j)[2]),'DM_Q_NEXT')
   require(row[29]==min(q,row[28]) and row[41]==s-row[29] and row[42]==f+row[29] and signed(row,43)==p,'DM_ACTUAL_PATCH');dm+=1
  elif row[2]==16:
   require(row[7]==27 and not any(row[30:63]) and row[27:29]==[1,1] and row[29] in [1,2,4,8,16,32],'PREDICTOR_LAYOUT');entry,trial,end,mid=[signed(row,at) for at in [17,19,21,23]];require(mid==oracle.rne_div(entry+end,2),'PREDICTOR_MIDPOINT');require(row[25] in [0,1] and row[26] in [0,1] and (row[15]>0 or mid==0),'PREDICTOR_SUPPORT');predict+=1
 for i in range(path_count):
  at=profile['pathByteOffset']//4+16*i;row=stream[at:at+16];require(0<row[0]<=event_count and row[2]<profile['K'] and row[4]<=row[5]<=65536 and row[15]==3235776802,'PATH_PREFIX')
 return {'dmEvents':dm,'predictorEvents':predict}

class MappedWords:
 def __init__(self,raw,at,count):self.raw,self.at,self.count=raw,at,count
 def __len__(self):return self.count
 def __getitem__(self,key):
  if isinstance(key,slice):return [self[i] for i in range(*key.indices(self.count))]
  if key<0:key+=self.count
  if key<0 or key>=self.count:raise IndexError(key)
  return struct.unpack_from('<I',self.raw,4*(self.at+key))[0]
def file_sha(path):
 h=hashlib.sha256()
 with path.open('rb') as f:
  while chunk:=f.read(65536):h.update(chunk)
 return h.hexdigest()
def check_source(role_path, archive, header, tail, inp):
 raw=(ROOT/'docs/architecture-T736-source-contract-v1.json').read_bytes()
 contract=json.loads(raw); approved=contract['families']['RX']; roles=json.loads(role_path.read_bytes())
 role_set=roles.pop('roleSetSHA')
 require(role_set==digest(json.dumps(roles,sort_keys=True,separators=(',',':')).encode()),'SOURCE_MANIFEST_SHA')
 require((roles['schema'],roles['backend'],roles['variant'],roles['family'],roles['supportsRoots'],
          roles['t736ContractSHA'],roles['source'],roles['reflection'],roles['passTable'])==
         ('G9_BOOT_SOURCE_V1','GLES','BOOT_SOURCE','RX',['SOURCE'],digest(raw),
          {**approved['sourceRole'],'opcodes':list(range(12))},contract['reflection'],contract['passTable']),
         'UNTRUSTED_SOURCE_MANIFEST')
 initial=json.loads(archive['initialStatusRaw']); final=json.loads(archive['finalStatusRaw'])
 root=initial['source']; other=final['source']
 loaded=json.loads(archive['loadedIdentityRaw'])
 require(root=={**other,'pendingNow':root['pendingNow']} and initial['configure']['ticket']==final['configure']['ticket'] and
         initial['pid']==final['pid'] and initial['processStartTick']==final['processStartTick'] and bool(initial['processStartTick']) and
         loaded['memoryBuildIdMatched'] and initial['configure']['mappedBuildId']==final['configure']['mappedBuildId']==loaded['BuildId'],
         'SOURCE_ARCHIVE_SCOPE_CHANGED')
 require(root['variant']=='BOOT_SOURCE' and root['roleSetSHA']==role_set==hexwords(header[120:128]) and
         root['sourceSHA']==approved['sourceRole']['sha256']==hexwords(tail[80:88]) and
         root['inputSHA']==sha_words(inp) and root['seedSHA']==hexwords(header[152:160]) and
         root['parameterSHA']==hexwords(header[144:152]) and header[7]==192 and tail[5]==1 and inp[40]==1,
         'SOURCE_ROOT_IDENTITY')
 require(inp[64:77]==approved['inputBody64To76'] and inp[24]==0 and tail[6]==1 and
         tail[62]==root['admittedQ20']==contract['firstRootExpected']['admittedQ'] and
         int(initial['commit'])==int(final['commit'])==int(root['baseCommit'])+1==u64(tail,30) and
         int(initial['quanta'])==int(final['quanta'])==int(root['baseQuanta'])==u64(tail,54),
         'T736_POSITIVE_SOURCE')
 require(initial['configure']['run']==root['run'] and initial['configure']['context']==root['context'] and
         all(int(root[name])==u64(tail,at) for name,at in [('nonce',8),('run',10),('owner',12),('context',14),('geometry',18),('plan',26),('baseCommit',28)]),
         'SOURCE_TAIL_SCOPE')
 require(root['typedAccepted'] and root['committed'] and root['planRetired'] and not root['fault'] and
         root['pendingAtClaim']==root['activeRemaining']==root['queueAfter']==0 and root['queueBefore']==1 and
         root['candidateObject']==root['committedObject']!=0 and bool(int(root['expectedSHA'],16)) and
         root['fenceCreated'] and root['fenceWaitRaw'] in (0x911a,0x911c) and root['mapReturned'] and root['unmapped'] and
         root['workByteOffset']==1171072 and root['workReadBytes']==1024 and root['gl']==0 and root['egl']==12288 and
         0<int(root['beginNs'])<=int(root['fenceNs'])<=int(root['commitNs'])<=int(root['deadlineNs']),
         'SOURCE_NATIVE_PROOF')
 require((root['dispatches'],root['dispatchBarriers'],root['copyBarriers'])==(12,12,3) and
         [r[:5] for r in root['passes']]==[[i,i,0,0,0xffffffff] for i in range(12)] and
         root['boundProgram']==root['sourceProgram']!=0 and root['bootProgram']!=root['sourceProgram'],
         'SOURCE_DISPATCH_RECEIPT')
 return role_set

def decode(path,source_manifest=None):
 table=json.loads((ROOT/'docs/architecture-T641-storage-cold-v1.json').read_text(encoding='utf8'));pages_hash=hashlib.sha256();payload_hash=hashlib.sha256();prefix_hash=hashlib.sha256();manifest=None;trace=None;page_index=0
 with tempfile.TemporaryFile() as raw:
  with path.open('rb') as archive:
   for line in archive:
    require(len(line)<=16384 and line.endswith(b'\n'),'JSONL_LINE_LIMIT');item=json.loads(line)
    if item.get('schema')=='G9_RX_ARCHIVE_V1':require(manifest is None,'DUPLICATE_MANIFEST');manifest=item;continue
    require(manifest is None and item.get('page')=='G9_DM_RX_SEGMENT_PAGE_V1' and item['index']==page_index,'PAGE_ORDER');trace=item['trace'] if trace is None else trace;require(item['trace']==trace,'TRACE_IDENTITY');w=item['words'];require(0<len(w)<=1024 and all(type(x) is int and 0<=x<2**32 for x in w),'PAGE_WORDS');data=packed(w);raw.write(data);pages_hash.update(line);payload_hash.update(data)
    if page_index<8:prefix_hash.update(data)
    page_index+=1
  require(manifest is not None and manifest['trace']==trace and manifest['pages']==page_index and manifest['rawBytes']==raw.tell() and manifest['rawPrefixSHA']==prefix_hash.hexdigest() and manifest['payloadRawSHA']==payload_hash.hexdigest() and manifest['pagesFileSHA']==pages_hash.hexdigest(),'ARCHIVE_MANIFEST')
  raw.flush()
  with mmap.mmap(raw.fileno(),0,access=mmap.ACCESS_READ) as mapped:
   prefix=words(mapped[:32768]);h=prefix[:256];require(h[:5]==[1129861703,1,8192,3235799921,827477575] and not any(h[248:]),'RX_HEADER');profile=next((p for p in table['profiles'] if p['K']==h[7]),None);require(profile is not None and h[5] in [0,1],'PROFILE_K_MODE');layout=profile['modes']['FULL' if h[5] else 'ROOT'];require(len(mapped)==layout['bytes']==u64(h,8) and h[6]==len(layout['segments']),'RAW_LAYOUT');require(not any(prefix[1408:2048]) and not any(prefix[3712:8192]),'PREFIX_ZERO_GAPS')
   config=prefix[3584:3648];require(config==next(p['config64'] for p in json.loads((ROOT/'docs/architecture-T641-seed-config-v1.json').read_text(encoding='utf8'))['profiles'] if p['K']==h[7]),'CONFIG64');eqhash(h,144,packed(config),'PARAMETER_SHA');require(prefix[704:768]==config,'ROOT_CONFIG64')
   inp=prefix[512:640];tail=prefix[256:512];kind=tail[5];require(kind in [1,2] and tail[:5]==[843269191,2,256,3235776802,827477575] and tail[39:42]==[12 if kind==1 else 2404]*3 and tail[42]==(65535 if kind==1 else 262143) and not any(tail[156:]),'DM_COMPLETION');require(tail[64:72]==h[144:152] and tail[72:80]==h[128:136] and sha_words(inp)==hexwords(tail[88:96]),'TAIL_HASH_LINK')
   current=record_check(prefix[3072:3328],1,inp,h,True);base=record_check(prefix[3328:3584],2,None,h,False);require(h[224]==int(current)+2*int(base) and current==(inp[40]==2),'CURRENT_BASE_PRESENCE')
   seed=prefix[3648:3712];require(seed[:4]==[1297306183,1,64,3235799922] and seed[12:20]==h[152:160] and seed[20:28]==h[144:152] and not any(seed[37:]),'SEED_META')
   segments=[]
   for i,seg in enumerate(layout['segments']):
    d=prefix[2048+32*i:2080+32*i];require(d[0]==i+1 and u64(d,8)==seg['byteStart']//4 and u64(d,10)==seg['bytes']//4 and d[31]==3235776802,'DESCRIPTOR');require(hexwords(d[23:31])==digest(memoryview(mapped)[seg['byteStart']:seg['byteEnd']]),'SEGMENT_SHA');segments.append(MappedWords(mapped,seg['byteStart']//4,seg['bytes']//4))
   if current:require(hexwords(h[104:112])==sha_words(geometry(prefix[3072:3328])) and prefix[3080:3098]==h[50:68] and u64(prefix[3072:3328],140)==u64(h,26),'CURRENT_SCOPE_GEOMETRY_NONCE')
   else:require(not any(h[32:36]+h[104:112]),'ABSENT_CURRENT_SIDECAR')
   if base:
    record=prefix[3328:3584];capsule=segments[0][384:512];require(segments[0][105]==1 and capsule[:3]==[1481788999,3,827477575] and capsule[40]==2 and u64(capsule,41)==u64(record,26) and u64(segments[0],30)==u64(record,26),'BASE_CAPSULE_IDENTITY');eqhash(record,176,packed(capsule),'BASE_FROZEN_INPUT_LINK');require(hexwords(h[112:120])==sha_words(geometry(record)),'BASE_GEOMETRY_KEY')
   else:require(not any(h[36:40]+h[112:120]),'ABSENT_BASE_SIDECAR')
   before,after=[state_check(state,h[7],config) for state in segments[:2]]
   for axis in range(2):
    ledger=sum(signed(segments[1],50+8*c+4*axis,4)-signed(segments[0],50+8*c+4*axis,4) for c in range(4));require(after[axis]-before[axis]==ledger,'MOMENTUM_LEDGER')
   if tail[6] in [1,3]:require(u64(tail,30)==u64(tail,28)+1 and u64(tail,54)==u64(segments[0],12)+(kind==2),'ACCEPTED_TIME')
   else:require(tail[6] in [2,4] and u64(tail,30)==u64(tail,28) and u64(tail,54)==u64(segments[0],12),'UNCOMMITTED_TIME')
   results=[]
   for i,stream in enumerate(segments[2:],2):
    d=prefix[2048+32*i:2080+32*i];results.append(stream_check(stream,d[5],d[6],profile,u64(h,26)))
   if tail[6]!=4:
    selected,actual=segments[2:4];events,paths=tail[44],tail[46]
    for row in range(events):
     a=selected[64*row:64*(row+1)];b=actual[64*row:64*(row+1)];a[14]&=~3;b[14]&=~3;require(a==b,'AUTHORITATIVE_EVENT_REPLAY')
    p=profile['pathByteOffset']//4;require(selected[p:p+paths*16]==actual[p:p+paths*16],'AUTHORITATIVE_PATH_REPLAY')
   journal=manifest.get('journal',[]);require(len(journal)==256,'TERMINAL_JOURNAL_ORIGINALS');loaded=manifest.get('loadedIdentityRaw','');eqhash(h,176,loaded.encode(),'LOADED_RECEIPT_SHA');require(json.loads(loaded)['sha256']==hexwords(h[216:224]),'LOADED_MODULE_LINK')
   result={'schema':'GNR1_COLD_READER_V1','archiveSHA':file_sha(path),'inputReceipt':'VALID_FROZEN_PROTOCOL' if current else 'NO_CURRENT_IMPULSE','baseHistory':'REFERENCE_ONLY_NO_PRIOR_ROOT_ARCHIVE' if base else 'ABSENT','physicalMassMomentum':'CLOSED','dmEventChecks':results,'acceptedStreamReplay':tail[6]!=4,'nativeCommitAndVisibleFrame':'REQUIRE_EXTERNAL_LIFECYCLE_DEVICE_EVIDENCE','deviceUsed':False}
   source_status=json.loads(manifest.get('initialStatusRaw','{}')).get('source')
   if source_manifest or source_status:
    require(source_manifest is not None,'SOURCE_MANIFEST_REQUIRED')
    result['sourceRoleSetSHA']=check_source(source_manifest,manifest,h,tail,inp)
   return result
if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('archive',type=Path);parser.add_argument('--source-manifest',type=Path);args=parser.parse_args();print(json.dumps(decode(args.archive,args.source_manifest),indent=2))
