"""Offline, family-isolated rechecks of G9 reference inputs and host guards."""
import argparse
from copy import deepcopy
from fractions import Fraction
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory

ROOT=Path(__file__).resolve().parents[2]
Q=1<<20

def load(relative):
    return json.loads((ROOT/relative).read_bytes())

def module(name,path):
    s=importlib.util.spec_from_file_location(name,ROOT/path)
    m=importlib.util.module_from_spec(s)
    s.loader.exec_module(m)
    return m

def displace(data):
    try:
        x,y=(Fraction(data[field]) for field in ('dx','dy'))
    except ValueError:
        return {'status':'REJECT'}
    if abs(x)>128 or abs(y)>256:
        return {'status':'REJECT'}
    dx,dy=int(x*Q),int(y*Q)
    if dx*dx+dy*dy<(Q//4)**2:
        return {'status':'NO_IMPULSE'}
    vx,vy=2*dx,2*max(dy,0)
    sat=int(abs(vx)>24*Q)+2*int(vy>32*Q)
    vx=max(-24*Q,min(24*Q,vx));vy=min(32*Q,vy)
    if not vx and not vy:
        return {'status':'NO_IMPULSE'}
    return {'status':'COMMAND','vxQ':vx,'vyQ':vy,'sat':sat}

def rx():
    expected=load('test/t642/independent-exec.json')
    expected={row['id']:row['got'] for row in expected['results']}
    checked={}
    for case in load('docs/design-T637-fixtures-v1.json')['cases']:
        kind=case['kind'];data=case['input']
        if kind=='recipe':
            got=displace(data)
        elif kind in ('duration','age','validation'):
            low,high={'duration':(8333334,10**9),'age':(0,250000000),'validation':(0,8000000)}[kind]
            got={'eligible':low<=data['ns']<=high}
        else:
            continue
        assert got==case['expected'],(case['id'],got,case['expected'])
        if 'vxQ' in got:
            assert all(str(got[k])==str(expected[case['id']][k]) for k in ('vxQ','vyQ'))
        checked[case['id']]=got
    assert checked['P01']['vxQ']==Q//2 and checked['P02']['status']=='NO_IMPULSE'
    assert displace(dict(dx='1',dy='1',receiptNs=20000000))==displace(dict(dx='1',dy='1',receiptNs=200000000))
    return checked

def fixtures():
    make=module('t670_fixtures','test/t670/fixtures.py').make
    results={}
    with TemporaryDirectory(prefix='t779-fixtures-') as d:
        for label,k,angle,full,roots,foreign,short in (
            ('DM20',192,7680,False,1,False,False),('DM22',192,7680,False,1,True,False),
            ('DM29',192,7680,False,2,False,False),('DM31',512,7680,True,1,False,True)):
            path=Path(d)/label
            files=make('dm',k,angle,full,path,roots,foreign,short)
            for name,identity in files.items():
                blob=(path/name).read_bytes()
                assert len(blob)==identity['bytes'] and hashlib.sha256(blob).hexdigest()==identity['sha256']
            command=(path/'commands.u32').read_bytes()
            assert len(command)==roots*192*4
            assert struct.unpack_from('<I',command,8)[0]==(826691667 if foreign else 843924551)
            if short:
                resource=next(r for r in load('docs/architecture-T554-g9-dm-manifest-v2.json')['resources'] if r['K']==k)
                assert struct.unpack('<Q',(path/'cold-credit.u32').read_bytes())[0]*4==resource['FULL']['bytes']-8*1024**2
            results[label]={'files':files,'inputFixtureOnly':True,'device':'NOT_RUN'}
    return results

def totals(state):
    result={}
    for bucket in state.values():
        for origin,quantity in bucket.items():
            if type(quantity) is not int or quantity<0:
                raise ValueError('INVALID_ORIGIN_QUANTITY')
            result[origin]=result.get(origin,0)+quantity
    return result

def transaction(before,actual,replay,accepted):
    if not accepted and actual!=before:
        raise ValueError('REJECTED_MUTATION')
    if totals(before)!=totals(actual):
        raise ValueError('ORIGIN_CONSERVATION')
    if actual!=replay:
        raise ValueError('REPLAY_MISMATCH')

def ledger():
    # Prescribed snapshots isolate the invariants from GPU dynamics.
    bed={'S':{'sandA':8,'sandB':2},'F':{},'P':{}}
    states=[bed,{'S':{'sandA':5,'sandB':2},'F':{'sandA':3},'P':{}},
            {'S':{'sandA':5,'sandB':2},'F':{'sandA':1},'P':{'sandA':2}},
            {'S':{'sandA':5,'sandB':2},'F':{'sandA':3},'P':{}},bed]
    for before,after in zip(states,states[1:]):
        transaction(before,after,deepcopy(after),True)
    for label,args in (
        ('ORIGIN_CONSERVATION',(bed,{'S':{'sandA':7,'sandB':3}},bed,True)),
        ('REJECTED_MUTATION',(bed,states[1],states[1],False)),
        ('REPLAY_MISMATCH',(bed,states[1],states[2],True))):
        try:
            transaction(*args)
        except ValueError as error:
            assert str(error)==label
        else:
            raise AssertionError(label)
    return {'fixtureOnly':True,'transitions':['S_TO_F','F_TO_P','P_TO_F','F_TO_S'],
            'negative':['ORIGIN_CONSERVATION','REJECTED_MUTATION','REPLAY_MISMATCH']}

def cold_rejections():
    dm=module('t779_dm_reader','tools/g9dm/read_cold.py')
    rx_reader=module('t779_rx_reader','tools/g9rx/read_cold.py')
    results={}
    with TemporaryDirectory(prefix='t779-cold-negative-') as d:
        path=Path(d)/'misframed.u32'
        for kind in (0,1):
            words=[0]*8192
            words[:10]=[844383303,2,8192,3235776802,843924551,kind,10 if kind else 4,192,32772,0]
            path.write_bytes(struct.pack('<8192I',*words))
            try:
                dm.read(path)
            except ValueError as error:
                assert str(error)=='COLD_LAYOUT'
                results['DM_FULL' if kind else 'DM_ROOT']=str(error)
            else:
                raise AssertionError('TRUNCATED_DM_ACCEPTED')
        page=Path(d)/'misframed.jsonl'
        page.write_bytes(b'{"page":"G9_DM_RX_SEGMENT_PAGE_V1","index":1}\n')
        try:
            rx_reader.decode(page)
        except ValueError as error:
            assert str(error)=='PAGE_ORDER'
            results['RX_PAGE']=str(error)
        else:
            raise AssertionError('REORDERED_RX_ACCEPTED')
    return results

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--rx-host-output',type=Path)
    args=parser.parse_args()
    if args.rx_host_output:
        sys.path.insert(0,str(ROOT/'test/t765'))
        host=module('t765_host','test/t765/run.py')
        with TemporaryDirectory(prefix='t779-host-') as build:
            host.BUILD=Path(build)
            result=dict(native=host.host_cases(),trace=host.synthetic(),static=host.source_wiring())
        args.rx_host_output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf8')
        return
    dm=module('t556_oracle','test/t556/independent_oracle.py').run_all()
    print(json.dumps({'dmOracle':dm,'rxDisplacement':rx(),'ledgerChecks':ledger(),
                      'inputs':fixtures(),'coldNegativeControls':cold_rejections(),
                      'acceptedRootFullReplay':'NOT_OBSERVED_OFFLINE',
                      'device':'NOT_RUN','productPASS':False},indent=2))

if __name__=='__main__':
    main()
