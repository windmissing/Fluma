"""T889 empty-frame, gravity, bounded ROI and original LAND word gates."""
import hashlib
import bisect
from functools import lru_cache
import json
import math
from pathlib import Path
import struct
import re

ROOT=Path(__file__).resolve().parents[2]
SEAL=3235823838
Q=1<<20
K=192
SCOPE=('pid','processStartTick','ownerTid','device','window','surface','geometry','swapchain')
BIND=SCOPE+('root','generation','imageIndex','lease','graphicsFence')
RESULTS=('acquire','submit','fence','present','imagePresent')
LAND_MASK=next(set(e['usedWordIndices']) for e in json.loads((ROOT/
    'docs/analysis-T238-gpu-water-M1-engineering-manifest-v1.json').read_bytes())['eventKinds'] if e['id']==8)


def need(ok,reason):
    if not ok:
        raise ValueError(reason)


def match(a,b,fields):
    return all(k in a and k in b and a[k]==b[k] for k in fields)


def u64(w,at):
    return w[at]|w[at+1]<<32


def u32sha(w):
    return hashlib.sha256(struct.pack(f'<{len(w)}I',*w)).hexdigest()


@lru_cache
def angle_tangents():
    source=(ROOT/'entry/src/main/cpp/gpu_resident/angles_generated.h').read_text()
    words=[int(h,16) for h in re.findall(r'0x([0-9a-f]{8})u',source)]
    need(len(words)==61444,'ANGLE_TABLE')
    return [w if w<2**31 else w-2**32 for w in words[::4]]


def angle_range(bed):
    need(len(bed)==128 and all(type(x) is int and 0<=x<=256*Q for x in bed),'GRAVITY_BED_RANGE')
    slopes=[b-a for a,b in zip(bed,bed[1:])]
    lo=max([-605394]+[s-605394 for s in slopes])
    hi=min([605394]+[s+605394 for s in slopes])
    table=angle_tangents()
    return bisect.bisect_left(table,lo)-7680,bisect.bisect_right(table,hi)-1-7680


def frame_ok(r):
    need(r['renderer']=='SHADER' and all(r[k]==0 for k in RESULTS)
         and 0<r['presentEnterQpc']<=r['presentExitQpc'],'SCENE_PRESENT')


def empty(e):
    init,frame,ready=e['init'],e['frame'],e['ready']
    need(match(init,frame,SCOPE) and match(frame,ready,SCOPE),'EMPTY_EPOCH')
    need(init['fence']==0 and init['zeroInventory'] is True and init['zeroResourceGeneration']==frame['generation']
         and init['fenceQpc']<=frame['presentEnterQpc']<=frame['presentExitQpc']<=ready['qpc'],'EMPTY_FENCE_READY')
    need(frame['kind']=='EMPTY_READY' and frame['root'] is None and frame['sourceCommit']==0
         and frame['acceptedSteps']==0 and frame['permit'] is None and frame['inventoryQ20']==[0,0],
         'EMPTY_NULL_ROOT')
    frame_ok(frame)
    for event in e['invalidatedInput']:
        need(event['geometry']!=ready['geometry'] and event['accepted'] is False
             and event['reason']=='STALE_GEOMETRY','EMPTY_STALE_INPUT')
    return 'EMPTY_PREVIEW_JOINED'


def gravity(e):
    last_seq=0
    generation=e['initialGeometry']
    inputs={}
    grants={}
    previous={}
    proof=[]
    for row in e['records']:
        phase=row['phase']
        if phase=='RESIZE':
            need(row['geometry']!=generation,'GRAVITY_RESIZE')
            generation=row['geometry']
            inputs.clear()
            grants.clear()
            continue
        if phase=='INPUT':
            valid=row['geometry']==generation and row['seq']>last_seq and -7680<=row['angle']<=7680
            if not valid:
                expected=('STALE_GEOMETRY' if row['geometry']!=generation else 'INPUT_SEQUENCE'
                          if row['seq']<=last_seq else 'ANGLE_OUT_OF_RANGE')
                need(row['result']==expected,'GRAVITY_INPUT_REJECT')
                continue
            need(row['result']=='ACCEPTED' and row['qpc']>0,'GRAVITY_INPUT')
            inputs[row['seq']]=row
            last_seq=row['seq']
            proof.append(row['angle'])
        elif phase=='GRANT':
            key=row['permit']
            sample=inputs.get(row['seq'])
            need(sample is not None and row['seq']==last_seq and row['geometry']==generation
                 and row['angle']==sample['angle'] and row['activeBefore']==0
                 and row['qpc']>=sample['qpc'] and key not in grants,'GRAVITY_GRANT')
            grants[key]=row
        elif phase=='QUANTUM':
            g=grants.get(row['permit'])
            tw=row['completion']
            angle=tw[107] if tw[107]<2**31 else tw[107]-2**32
            lo,hi=angle_range(row['bedBeforeQ20'])
            need(g and row['geometry']==generation and row['accepted'] is True and row['fence']==0
                 and 0<row['qpc'] and row['qpc']>=g['qpc']
                 and len(tw)==256 and tw[:4]==[1381192263,2,256,SEAL] and tw[27:29]==[2,1]
                 and str(u64(tw,4))==row['root'] and str(u64(tw,38))==str(generation)
                 and u64(tw,22)==row['permit'] and row['grantedAngle']==g['angle']
                 and -7680<=lo<=hi<=7680 and angle==min(hi,max(lo,g['angle']))
                 and row['constraintApplied']==(angle!=g['angle']),'GRAVITY_QUANTUM')
            need(row['ordinal']==tw[26] and row['ordinal']==previous.get(row['permit'],0)+1,
                 'GRAVITY_ORDINAL')
            previous[row['permit']]=row['ordinal']
        else:
            raise ValueError('GRAVITY_PHASE')
    need([0,3840,-3840,0]==proof and grants and previous,'GRAVITY_COVERAGE')
    return 'M1_ANGLE_GENERATION_JOINED'


def rgba_piece(row,image):
    x,y,w,h=row['rect']
    width,height=image['extent']
    need(0<=x<x+w<=width and 0<=y<y+h<=height and 1<=w<=16 and 1<=h<=16,'ROI_RECT')
    data=bytes.fromhex(row['bytesHex'])
    need(len(data)==4*w*h and hashlib.sha256(data).hexdigest()==row['sha256'],'ROI_BYTES')
    if image['format']=='BGRA8':
        data=bytes(value for i in range(0,len(data),4) for value in (data[i+2],data[i+1],data[i],data[i+3]))
    return x,y,w,h,data


def roi(e):
    frame=e['frame']; gpu=e['gpu']
    frame_ok(frame)
    need(match(frame,gpu,BIND) and gpu['copyFenceVkResult']==0 and gpu['invalidateVkResult']==0,
         'ROI_GPU_JOIN')
    w=gpu['publicationWords']; bank=gpu['bankWords']; tail=gpu['completion']
    need(len(w)==7936 and len(bank)==7424 and len(tail)==256
         and all(u32sha(data)==gpu[k] for data,k in ((w,'publicationSHA'),(bank,'bankSHA'),(tail,'completionSHA'))),
         'ROI_GPU_BYTES')
    col=5632
    header=w[col+2048:]
    need(header[:4]==[1347637831,2,256,SEAL] and tail[:4]==[1381192263,2,256,SEAL]
         and tail[27:29]==[2,1] and u64(tail,4)==int(frame['root'])
         and u64(header,18)==u64(tail,38)==int(frame['geometry'])
         and u64(header,32)==u64(tail,36)==int(frame['generation'])
         and all(u64(header,a)==u64(tail,b) for a,b in ((4,6),(12,18),(14,20))),'ROI_PACK_HEADER')
    s=sum(bank[5120+256:5120+384])
    p=sum(bank[:K])
    need(s>0 and p>0 and s+p==u64(tail,77)==u64(tail,71) and 0<header[34]<=K
         and u64(tail,79)==0,'ROI_DUAL_STOCK')
    f=lambda word:struct.unpack('<f',struct.pack('<I',word))[0]
    packets=[(f(w[8*K+i]),f(w[9*K+i]),f(w[10*K+i])) for i in range(header[34]) if w[3*K+i]]
    need(bool(packets) and all(math.isfinite(c) for a in packets for c in a),'ROI_PACKET_FIELDS')
    width,height=frame['extent']
    need(frame['format'] in ('RGBA8','BGRA8') and frame['rowOrder']=='TOP_DOWN'
         and frame['srgbEncodings']==1 and width>0 and height>0,'ROI_FORMAT')
    expected=e['palette']
    need(set(expected)=={'sand','background'} and expected['sand']!=expected['background'],'ROI_PALETTE')
    used=set()
    need([r['label'] for r in e['probes']]==['bed','particle','background'],'ROI_PROBE_SET')
    for row in e['probes']:
        need(match(frame,row,BIND),'ROI_FRAME_IDENTITY')
        x,y,_,ww,data=rgba_piece(row,frame)
        need(len(data)<=1024,'ROI_CREDIT')
        # Classify the declared pixel within each bounded raw ROI.
        i,j=row['sample']
        need(0<=i<row['rect'][2] and 0<=j<ww and (x+i,y+j) not in used,'ROI_SAMPLE')
        used.add((x+i,y+j))
        px,py=x+i+.5,y+j+.5
        need(2<px<width-2 and 2<py<height-2,'ROI_BORDER')
        model_x,model_y=128*px/width,256*(1-py/height)
        bed=f(w[col+8*128+int(model_x)])
        inside_bed=model_y<bed
        inside_packet=any(rad>0 and math.hypot(px-cx/128*width,py-(1-cy/256)*height)<rad
                          for cx,cy,rad in packets)
        label=('mixed' if inside_bed and inside_packet else 'bed' if inside_bed else
               'particle' if inside_packet else 'background')
        need(label==row['label'],'ROI_SPATIAL_PAIR')
        at=4*(j*row['rect'][2]+i)
        need(list(data[at:at+4])==expected['background' if label=='background' else 'sand'],'ROI_COLOR')
    return {'state':'DUAL_ROI_JOINED','S':s,'P':p}


def land(e):
    before=e['before']; after=e['after']; tail=e['completion']
    need(match(before,after,SCOPE) and after['qpc']>before['qpc']>=e['upQpc'] and before['P']>0
         and len(tail)==256 and tail[:4]==[1381192263,2,256,SEAL]
         and tail[27:29]==[2,1] and after['accepted'] is True and after['fence']==0
         and u64(tail,4)==int(after['root']) and u64(tail,38)==int(after['geometry'])
         and u64(tail,18)==u64(tail,14)==after['sourceCommit']==before['sourceCommit'],
         'LAND_AFTER_UP_ROOT')
    stream=e['eventWords']
    need(len(stream)==64*tail[53] and u32sha(stream)==e['eventSHA'] and tail[52]<=tail[53],
         'LAND_EVENT_RANGE')
    mass=0
    for i in range(tail[52],tail[53]):
        r=stream[64*i:64*(i+1)]
        if r[6]!=8:
            continue
        moved=r[33]-r[32]
        need(r[:2]==[1163088455,2] and u64(r,2)==u64(tail,4) and r[4]==2
             and u64(r,7)==i+1 and u64(r,9)==u64(tail,24) and r[11]==tail[26]
             and 0<=r[16]<K and r[17]>0 and r[18]>0 and moved>0 and r[34]-r[35]==moved
             and r[36]-r[38]==r[40]-r[41]==moved
             and not any(word for j,word in enumerate(r) if j not in LAND_MASK),'LAND_EVENT')
        mass+=moved
    need(mass>0 and after['S']-before['S']==before['P']-after['P']==mass and
         after['S']+after['P']==u64(tail,77)==u64(tail,71),'LAND_CONSERVATION')
    return {'state':'LAND_AFTER_UP_JOINED','movedQ20':mass}
