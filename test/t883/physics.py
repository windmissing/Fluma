"""PC offline long-window, layer, and explicit G9 archive provenance gates."""
import json
import struct

from transactions import need

FAMILIES = {
    'DM2': ('G9_SPF_DM_COLD_V2','U32LE',843924551),
    'R1': ('G9_SPF_COLD_V1','JSONL',826691667),
    'RX': ('G9_DM_RX_COLD_V1','JSONL',827477575),
}
HEADERS = {
    'DM2': (844383303,2,8192,3235776802,843924551),
    'R1': (0x31433947,1,8192,3235776769,826691667),
    'RX': (1129861703,1,8192,3235799921,827477575),
}
TAIL_HEADERS = {
    'DM2': (843269191,2,256,3235776802,843924551),
    'R1': (826489159,1,256,3235776769,826691667),
    'RX': (843269191,2,256,3235776802,827477575),
}


def motion_plan(pointer):
    return [dict(ms=row['ms'],kind=row['kind'],generation=row['generation'],args=row['args']) for row in pointer] + [
        dict(ms=1200,kind='GRAVITY',generation=2,args=[-1,0]),
        dict(ms=1550,kind='GRAVITY',generation=2,args=[0,-1]),
        dict(ms=1900,kind='OBSERVE_EMPTY',generation=2,args=[])]


def gravity(plan, observed):
    need(len(plan)==len(observed),'GRAVITY_MISSING_OBSERVATION')
    ticks=[r['observedQpc'] for r in observed]
    need(ticks==sorted(set(ticks)),'GRAVITY_ORDER')
    first=observed[0]['geometry']
    second=next(r['geometry'] for r in observed if r['action']['kind']=='RESIZE')
    need(first!=second,'GRAVITY_GENERATION')
    for p,r in zip(plan,observed,strict=True):
        need(r['action']==p and r['observedQpc']>=r['inputQpc'],'GRAVITY_ACTION')
        gen=first if p['generation']==1 else second
        if p['kind']=='STALE_DOWN':
            need(r['geometry']==first and r['activeGeometry']==second and r['accepted'] is False,'OLD_GEOMETRY_INPUT')
        elif p['kind']=='GRAVITY':
            need(r['geometry']==gen and r['accepted'] is True and r['effectiveGravity']==p['args']
                 and r['root'] and r['acceptedInputQpc']==r['inputQpc'],'GRAVITY_ROOT')
        elif p['kind'] in ('EMPTY','OBSERVE_EMPTY'):
            need(r['geometry']==gen and r['root'] and r['sandQ20']==0 and r['particleQ20']==0
                 and r['shaderFrame'] is True,'EMPTY_WINDOW')
    return 'EMPTY_AND_GRAVITY_JOINED'


def long_window(e, pixel_sha, present_results):
    cfg=e['window']
    rows=e['roots']
    needs=('S','P','landed','admitted','qpc','tick','nonce','scope','kind','accepted','fence')
    if not rows or any(any(k not in r for k in needs) for r in rows):
        return {'state':'MISSING_EVIDENCE','missing':['accepted inventory/event timeline']}
    need(cfg['frequency']>0 and cfg['minAfterStopMs']>0 and cfg['start']<cfg['stop']<cfg['end'],
         'LONG_WINDOW_PLAN')
    need(rows[0]['qpc']<=cfg['start'] and rows[-1]['qpc']>=cfg['end'],'LONG_WINDOW_COVERAGE')
    need(len({r['nonce'] for r in rows})==len(rows) and all(r['scope']==cfg['scope'] for r in rows),
         'LONG_WINDOW_SCOPE')
    need(all(r['accepted'] is True and r['fence']==0 for r in rows),'LONG_WINDOW_ACCEPTANCE')
    need(all(type(rows[0][k]) is int and rows[0][k]>=0 for k in ('S','P','tick')),'INITIAL_STOCK')
    current=set()
    by={}
    post=0
    for a,b in zip(rows,rows[1:]):
        need(a['qpc']<b['qpc'] and 0<=b['tick']-a['tick']<=1,'ACCEPTED_TICK_GAP')
        need(all(type(b[k]) is int and b[k]>=0 for k in ('S','P','landed','admitted')),'INVENTORY_WORDS')
        if b['qpc']>=cfg['stop']:
            need(b['admitted']==0,'POST_STOP_SOURCE')
        need((b['kind']=='SOURCE' and b['admitted']>0 and b['landed']==0 and b['tick']==a['tick'])
             or (b['kind']=='QUANTUM' and b['tick']==a['tick']+1 and b['admitted']==0),'ACCEPTED_PHASE')
        need(b['S']-a['S']==b['landed'] and b['P']-a['P']==b['admitted']-b['landed'],
             'LAND_MASS_BALANCE')
        if b['qpc']>=cfg['stop']:
            if b['qpc']<=cfg['end']:
                post+=b['landed']
        if b['S']>0 and b['P']>0:
            current.add(b['nonce'])
        by[b['nonce']]=b
    need((cfg['end']-cfg['stop'])*1000>=cfg['minAfterStopMs']*cfg['frequency']
         and post>0,'POST_STOP_WINDOW_OR_LAND')
    matched=[]
    for frame in e.get('frames',[]):
        r=by.get(frame['nonce'])
        need(r is not None and frame['scope']==r['scope'] and frame['lease']==r['lease']
             and frame['geometry']==r['geometry'] and frame['presentQpc']>=r['qpc']
             and all(frame[k]==0 for k in present_results) and frame['renderer']=='SHADER'
             and frame['status']=='CURRENT',
             'PHYSICS_PRESENT_JOIN')
        raw=bytes.fromhex(frame['rgba'])
        need(frame['width']>0 and frame['height']>0 and
             frame['width']*frame['height']<=3840*2160 and len(raw)==4*frame['width']*frame['height']
             and pixel_sha(raw)==frame['imageSHA'],'PIXEL_PAYLOAD')
        indices=set()
        for name in ('bed','particle'):
            p=frame['probes'][name]
            need(p['lease']==r['lease'] and p['nonce']==r['nonce'] and p['geometry']==r['geometry']
                 and p['stockQ20']==r['S' if name=='bed' else 'P']>0 and p['index'] not in indices
                 and 0<=p['index']<len(raw)//4,'PIXEL_PHYSICS_IDENTITY')
            off=4*p['index']
            need(list(raw[off:off+4])==p['expectedRGBA'] and p['expectedRGBA']!=p['backgroundRGBA'],
                 'PIXEL_LAYER')
            indices.add(p['index'])
        if frame['nonce'] in current:
            matched.append(frame['nonce'])
    return dict(state='PHYSICS_AND_PIXELS' if matched else 'MISSING_EVIDENCE',
                missing=[] if matched else ['same lease bed+particle pixel probes'],
                postStopLandedQ20=post,sameRootFrames=matched)


def choose(family,schema,backend,encoding,profile):
    need(family in FAMILIES and (schema,encoding,profile)==FAMILIES[family],'ARCHIVE_ROUTE')
    need(backend in ('HISTORICAL','VULKAN_PC'),'ARCHIVE_BACKEND')
    return family


def prefix(path,encoding):
    with path.open('rb') as stream:
        if encoding=='U32LE':
            raw=stream.read(32768)
            need(len(raw)==32768,'G9_PREFIX')
            return list(struct.unpack('<8192I',raw))
        words=[]
        while len(words)<8192:
            line=stream.readline(16385)
            need(0<len(line)<=16384,'G9_PAGE_PREFIX')
            chunk=json.loads(line)['words']
            need(len(chunk)<=1024 and all(type(x) is int and 0<=x<2**32 for x in chunk),'G9_PAGE_WORD')
            words.extend(chunk)
        return words[:8192]


def u64(words, at):
    return words[at] | words[at+1]<<32


def interval(tail, expected):
    need(tail[5] in (1,2) and tail[6] in (1,3) and tail[43]==tail[44] and tail[45]==tail[46],
         'G9_UNACCEPTED_OR_UNSEALED')
    observed={key:str(u64(tail,idx)) for key,idx in
              (('nonce',8),('run',10),('geometry',18),('baseCommit',28),('commit',30),('tick',54))}
    need(observed==expected and all(int(observed[k])>0 for k in ('nonce','run','geometry'))
         and int(observed['commit'])==int(observed['baseCommit'])+1,'G9_INTERVAL_MISMATCH')
    return observed


def route(path, envelope, readers, sha_file):
    family=envelope['family']
    encoding=envelope['encoding']
    raw=prefix(path,encoding)
    choose(family,envelope['schema'],envelope['backend'],encoding,raw[4])
    need(tuple(raw[:5])==HEADERS[family],'G9_CONTAINER_HEADER')
    need(sha_file(path)==envelope['sha256'],'G9_ARCHIVE_SHA')
    tail=raw[256:512]
    need(tuple(tail[:5])==TAIL_HEADERS[family],'G9_TAIL_FAMILY')
    if envelope['backend']=='VULKAN_PC':
        # The historical family readers do not attest Vulkan role/PE/permit identity.
        return {'state':'MISSING_PC_ROLE_EVIDENCE','family':family,'eligiblePairs':0}
    report=readers[family](path)
    observed=interval(tail,envelope['acceptedInterval'])
    need(sha_file(path)==envelope['sha256'],'G9_ARCHIVE_CHANGED')
    return dict(state='HISTORICAL_ARCHIVE_ONLY',family=family,acceptedInterval=observed,
                structure=report,eligiblePairs=0)
