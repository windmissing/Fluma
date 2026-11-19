"""Generate only T619 architecture tables and update their explicit parent references."""
from pathlib import Path
import hashlib,json
R=Path(__file__).resolve().parents[1];D=R/'docs'
def get(n):return json.loads((D/n).read_text(encoding='utf-8-sig'))
def put(n,x):(D/n).write_text(json.dumps(x,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def receipt(n):
 b=(D/n).read_bytes();return dict(path='docs/'+n,bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
def field(i,name,producer,unit='U32',constant=None):
 x=dict(word=i,name=name,producer=producer,unit=unit)
 if constant is not None:x['constant']=constant
 return x
c=[]
for name,value in [('magic',0x31433552),('revision',1),('words',64),('seal',0xC0DE0511)]:
 c.append(field(len(c),name,'CONTRACT_CONSTANT',constant=value))
for name in ['kind','ordinal','baseBank','candidateBank','spareBank','eventArena','eventStart','eventCapacity','publicationSlot','uploadedBytes','carry']:
 c.append(field(len(c),name,'frozen RootPermitV5.'+name))
c.append(field(15,'captureFlags','owner actual authorization and first dispatch submission; bit0 AUTHORIZED, bit1 ROOT_ENTERED, all other bits0'))
for name in ['nonce','plan','frame','control','geometry','sourceCommit','physicsCommit','publicationGeneration','deadline']:
 for part in ['Lo','Hi']:c.append(field(len(c),name+part,'frozen RootPermitV5.'+name,'U64_'+part+'; deadline ns MONOTONIC' if name=='deadline' else 'U64_'+part))
for name in ['run','owner','context','inputEpoch','pageSession','permission','window','config','surface']:
 for part in ['Lo','Hi']:c.append(field(len(c),name+part,'frozen RootPermitV5.identity.'+name,'U64_'+part))
for name in ['sourcePrefix','dedupPrefix','eventEnd','actualBank']:
 c.append(field(len(c),name,'frozen RootOwnerV5 before root.'+name))
for name in ['eventFrame']:
 for part in ['Lo','Hi']:c.append(field(len(c),name+part,'frozen RootOwnerV5 before root.'+name,'U64_'+part))
for name in ['lastGood','stale','currentBank','publicationSlot']:c.append(field(len(c),name,'frozen RootOwnerV5 before root.'+name,'boolean U32' if name in ['lastGood','stale'] else 'U32'))
assert len(c)==62
for i in [62,63]:c.append(field(i,'reserved_'+str(i),'CONTRACT_ZERO',constant=0))
caps=[]
def add(name,producer,unit='U32',constant=None,valid=None):
 x=field(len(caps),name,producer,unit,constant)
 if valid is not None:x['valid']=valid
 caps.append(x)
for name,value in [('magic',0x46433556),('revision',1),('words',1024),('seal',0xC0DE0511)]:add(name,'CONTRACT_CONSTANT',constant=value)
add('K','frozen Profile.k');add('Kw','frozen Profile.kw')
add('physicalWire','CONTRACT_CONSTANT',constant=5);add('features','CONTRACT_CONSTANT',constant=31)
add('parameterRevision','frozen config.lift.revision',valid='1..UINT32_MAX')
add('liftMode','frozen config.lift.mode',valid='OFF0/SENSOR_RESEARCH1/SYNTHETIC_TEST2')
add('omegaProvided','frozen config.omegaProvided','boolean U32',valid='must be1')
add('fullBranchTrace','frozen config.fullBranchTrace','boolean U32',valid='0 or1')
for name,unit,valid in [
 ('omega','angle-table index/s','0..1966080'),('waterRate','Q20/s','0..1006632960'),
 ('gravity','Q20/s^2','120..1073741824'),('settle','Q20/s','1..1073741824'),
 ('emit','Q20/s','0..1073741824'),('kAir','integer s^-1','0..960'),('kFlight','integer s^-1','0..960'),
 ('kWet','integer s^-1','1..960'),('soft','Q20 column height-difference threshold','0..605393'),
 ('flux','Q20/s per-edge solid transfer cap','0..4294967295')]:add(name,'frozen PhysicalConfigV5.'+name,unit,valid=valid)
for name,unit,valid in [
 ('liftRateQ16','Q16/s','0..62914560'),('bandQ','Q20 model height','1..1048576'),
 ('rampQ16','Q16/s','1..7864320'),('decayQ16','Q16/s','1..62914560'),
 ('liftSpeedQ','Q20/s model velocity','1..67108864'),('liftCapQ','Q20/s volume channel cap','0..314572800')]:add(name,'frozen config.lift.'+name,unit,valid=valid)
add('quantumHz','CONTRACT_CONSTANT','Hz',120);add('timeTicksPerQuantum','CONTRACT_CONSTANT','ticks',65536)
for name,value in [('gapNs',33333333),('rootDeadlineNs',250000000)]:
 add(name+'Lo','CONTRACT_CONSTANT','ns U64_Lo',value);add(name+'Hi','CONTRACT_CONSTANT','ns U64_Hi',0)
for name in ['layoutSha','shaderSha','angleSha','parameterSha']:
 for i in range(8):add(name+str(i),'frozen '+('config.lift.sha' if name=='parameterSha' else 'root permit '+name)+f'[{i}]','digest original bytes packed LE32')
for name,value in [('rootCursorRevision',1),('rootTokenRevision',1),('inputRevision',5)]:add(name,'CONTRACT_CONSTANT',constant=value)
assert len(caps)==69
add('effectiveWaterKappa','existing owned-program uniform policy: uKappa remains link-initial0; explicitly track0, reject if policy differs','integer s^-1',0)
for i in range(70,1024):add('reserved_'+str(i),'CONTRACT_ZERO',constant=0)
pre=[dict(name='domainMagic',words=1,constant=0x31545235),dict(name='tokenRevision',words=1,constant=1),
 dict(name='headerIdentity24to41',words=18,producer='original capture scope header words24..41, frozen at root; never current export scope')]
for name,n,producer in [('rootCursor64',64,'same root frozen entry'),('input64',64,'same root original INPUT64'),
 ('quota256',256,'same root original quota'),('completion256',256,'same root sealed completion'),
 ('rootRecord0to29',30,'same root actual finalized RootRecord128 words0..29'),
 ('parameterSha',8,'record42..49/config.lift.sha'),('shaderSha',8,'frozen RootPermitV5.shaderSha'),
 ('layoutSha',8,'frozen RootPermitV5.layoutSha'),('angleSha',8,'frozen RootPermitV5.angleSha'),
 ('configCapsuleRawSha',8,'SHA256 entire4096B capsule, digest bytes packed LE32')]:pre.append(dict(name=name,words=n,producer=producer))
x=dict(schema='T619_V5_PUBLIC_IDENTITY_V1',date='2026-11-19',status='ENGINEERING_READY_NOT_VALIDATED',
 encoding='LE32 U32; U64 low then high; no C++ object representation, padding or JSON',
 rootCursor64=dict(prefixByteStart=20736,slots=5,words=64,revision=1,usedMaskLo=0xffffffff,usedMaskHi=0x3fffffff,
 fields=c,capture='authorized owner before first root dispatch; publish entered flag only after actual submission',
 completeFlags=3,zeroWords=[62,63],notPrivateScopeCursor=True),
 rootTokenSha=dict(recordWords=[58,65],algorithm='SHA256',bytes=2920,words=730,preimage=pre,
 digestPacking='original digest bytes per4 LE32; lowercase hex in original byte order',
 missing='FOR FORENSIC: no invented commitment; COMPLETE requires all frozen inputs; NOT_EXECUTED token zero'),
 fullColdConfigCapsule=dict(prefixByteStart=26752,bytes=4096,words=1024,revision=1,fields=caps,
 parameterShaMeaning='existing named lift parameter SHA, not full-config digest',
 numericSemantics='model units, not SI material calibration; record actual frozen values, never defaults'),
 writerReader=dict(order=['frozen same-root originals','capsule','root record0..57','rootTokenSha58..65','nonprefix descriptor SHA',
 'T535 canonical prefix SHA mask[1116,1148)','raw prefix SHA','immutable publish'],
 reject=['IDENTITY_CONTRACT_MISSING','UNSUPPORTED_IDENTITY_REVISION','RESERVED_NONZERO','ROOT_TOKEN_MISMATCH','SCOPE_MISMATCH'],
 complete='all required actual producers, exact flags/versions/seals, same root identity and recomputed hashes',
 noNewEndpoint=True,prefixRevision=2,payload='GRS_M3B_LINEAGE_COLD_V5_2'),
 budget=dict(extraGPUBytes=0,extraDispatches=0,extraHotMapBytes=0,prefixBytes=32768,
 extraFrozenHostMetadataMaxBytes=2352,metadataCredit='7 CaptureRoot instances (5 retained + pending + accepted), each64 cursor words +18 original header identity words =2296 raw bytes; allow struct rounding up to2352, charge actual sizeof delta before allocation; never payload/diagnostic credit',
 hashWorkspace='streaming SHA context only; no duplicate preimage allocation',scopeCopiesPerPlan=40,scopeCopyBytesPerPlan=5120),
 G13Reuse='hash/credit/immutable publishing helpers only; not V5 public layout/token preimage',
 testsRun=False,deviceUsed=False)
put('architecture-T619-v5-public-identity-v1.json',x)
ref=receipt('architecture-T619-v5-public-identity-v1.json')
p=get('architecture-T527-v5-cold-manifest-v1.json');p['publicIdentityContract']=ref
history=p.setdefault('revisionHistory',[])
item=dict(task='T619',date='2026-11-19',change='first defined rootCursor64/token/capsule; no offsets/credit/prefix policy change')
if item not in history:history.append(item)
put('architecture-T527-v5-cold-manifest-v1.json',p)
w=get('architecture-T552-v5-segment-wire-v2.json');w['parent']=receipt('architecture-T527-v5-cold-manifest-v1.json');w['publicIdentityContract']=ref
put('architecture-T552-v5-segment-wire-v2.json',w)
t=get('architecture-T614-v5-entry-cursor-v1.json');t['publicRootIdentityContract']=ref
put('architecture-T614-v5-entry-cursor-v1.json',t)
print('T619 engineering tables generated; no product execution')
