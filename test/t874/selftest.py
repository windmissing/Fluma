"""Check native candidate ABI against preserved T868 SOURCE and synthetic archives."""
import importlib.util
import json
from pathlib import Path
import struct
from tempfile import TemporaryDirectory

ROOT=Path(__file__).resolve().parents[2]


def load(name,path):
    spec=importlib.util.spec_from_file_location(name,ROOT/path)
    mod=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


F=load('t866','test/t866/check.py').F
E=load('candidate_export','pc/vulkan/export_g13_candidate.py')
need=F.enforce
IDS=(('nonce',8),('plan',10),('owner',14),('context',16),('surface',18),('geometry',20),
     ('configGeneration',22),('commit',26),('acceptedQuantum',34))


def original():
    base=ROOT/'test/reports/t868'
    rows=list(map(json.loads,(base/'rest.jsonl').read_bytes().splitlines()))
    row=next(r for r in rows if 't833SourceArchive' in r)
    meta=row['t833SourceArchive']
    raw=(base/'rest-capture.bin.source').read_bytes()
    need(meta['schema']=='PC_G13_T833_RAW_V1' and len(raw)==meta['fileBytes']
         and F.sha(raw)==meta['sha256'],'original SOURCE receipt')
    sizes=meta['segmentsBytes']
    need(meta['segments']==['BASE','CANDIDATE','REPLAY','WORK','CTRL','PACK']
         and meta['fieldOffset']==1024 and sum(sizes)==len(raw)-1024,'original segment layout')
    at=1024
    pieces={}
    for name,length in zip(meta['segments'],sizes,strict=True):
        pieces[name]=raw[at:at+length]
        at+=length
    tail=E.words(raw[:1024])
    ctrl=E.words(pieces['CTRL'])
    permit=ctrl[128:256]
    need(tail==tuple(meta['completion'])==ctrl[18432:] and tail[4:6]==(50,1) and tail[39]==16
         and F.identity(tail,32)==0 and row['t833SourceFence']==0
         and meta['acceptedRoot']==str(F.identity(tail,8))==row['t833SourcePlan']['nonce']
         and (F.identity(tail,28),F.identity(tail,30))==(F.identity(permit,36),F.identity(permit,38)),
         'original SOURCE tail')
    return pieces,dict(archiveSHA256=F.sha(raw),tailSHA256=F.sha(raw[:1024]),opcode=tail[4],
                       count=tail[39],gpuAcceptedNs=0,nativeAcceptedNs='UNAVAILABLE_V1',
                       acceptedRoot=meta['acceptedRoot'])


def fixture(pieces, mode, opcode=0):
    # Original files remain untouched. This deliberately constructed archive
    # exercises the synthetic clock, ROOT/FULL directory and raw PACK joins.
    pieces=pieces.copy()
    c=list(E.words(pieces['CTRL']))
    permit=c[128:256]
    tail=c[18432:]
    token='test/t874 synthetic root'
    permit[80:88]=E.words(bytes.fromhex(F.sha(token.encode())))
    permit[1]=opcode
    tail[4]=opcode
    if opcode==0:
        tail[34]=1
        tail[36]=1
        tail[39:42]=[820,820,0]
        tail[104:110]=[0]*6
    c[128:256]=permit
    c[18432:]=tail
    pieces['CTRL']=E.pack(c)
    pack=[0]*(len(pieces.pop('PACK'))//4)
    pack[:2]=[1446195527,1]
    pack[4:6]=c[4:6]
    pack[7]=1
    pack[63]=0xc0de1301
    pack[14:16]=tail[20:22]
    pack[18:22]=tail[8:10]+tail[26:28]
    pack[24:32]=c[76:84]
    pack[67]=0x3d000000
    pieces['PACK0']=E.pack(pack)
    pieces['PACK1']=bytes(len(pieces['PACK0']))
    nonce=str(E.pair(tail,8))
    accepted=str(E.pair(tail,28)+1)
    meta=dict(baseToken=token,acceptedNs=accepted,acceptedRoot=nonce,privateProjectComplete=False,
              nativeError=0,baseBank=0,candidateBank=1,descriptorBuffers=['1','2','3','4','5','6','7'],
              sha256='0'*64,publicationSlot=0)
    parts,profile,config,tail,native=E.archive_parts(pieces,meta,mode)
    stream=E.page_archive(parts,nonce)
    manifest=dict(synthetic=True,case='F03',archiveSHA=F.sha(stream),completionSHA=F.sha(E.pack(tail)),
                  nativeAcceptedNs=accepted,native=native,acceptedRoot=nonce,roles={'test':'fixture'},
                  op66=[dict(role='test',moduleSHA='fixture',probeWords=F.T721.EXPECTED*2,
                             mismatchMasks=[0,0],queueSubmit=0,wait=0)])
    manifest.update((key,str(E.pair(tail,at))) for key,at in IDS)
    if mode=='FULL' and opcode==0:
        manifest['view']=E.grid_view(pieces,meta,profile,config,tail)
    return stream, manifest


def main():
    pieces,source=original()
    results={}
    negatives=[]
    with TemporaryDirectory() as tmp:
        path=Path(tmp)/'archive.jsonl'
        receipt=Path(tmp)/'archive.receipt.json'
        for mode,opcode,want in (('ROOT',50,'SOURCE_ABI_ONLY'),('ROOT',0,'ROOT_ABI_ONLY'),
                                  ('FULL',0,'SYNTHETIC_PASS')):
            stream, manifest=fixture(pieces,mode,opcode)
            path.write_bytes(stream)
            receipt.write_bytes(E.encoded(dict(schema='G13_FILE_RECEIPT_V1',sha256=F.sha(stream))))
            a=F.candidate_archive(path,receipt)
            result=F.inspect_archive(manifest,a,synthetic=True)
            need(result['state']==want,str(result))
            results[want]=True
            if mode=='FULL':
                for code,bad in (
                    ('native accepted epoch or permit',dict(manifest,nativeAcceptedNs=manifest['native']['deadlineNs'])),
                    ('native accepted epoch or permit',dict(manifest,acceptedRoot='1')),
                    ('GridView GPU PACK provenance',dict(manifest,view=dict(manifest['view'],phi=[0.]*4096))),
                    ('GridView raw field mismatch',dict(manifest,view=dict(manifest['view'],u32=[1]*4160))),
                    ('manifest archive SHA',dict(manifest,archiveSHA='00')),
                ):
                    reject(lambda:F.inspect_archive(bad,a,synthetic=True),code)
                    negatives.append(code)
                modified=bytearray(a['tail'])
                struct.pack_into('<I',modified,4*4,2)
                reject(lambda:F.inspect_archive(dict(manifest,completionSHA=F.sha(modified)),dict(a,tail=modified),
                                                synthetic=True),
                       'accepted opcode/counts/audit')
                negatives.append('logical opcode substitution')
    print(json.dumps(dict(historicalSOURCE=source,synthetic=results,negativeControls=negatives,
                          actualCompleteField='PENDING'),indent=2))


def reject(fn,expected):
    try:
        fn()
    except ValueError as error:
        need(str(error)==expected,f'{expected}: {error}')
    else:
        raise AssertionError(expected)


if __name__=='__main__':
    main()
