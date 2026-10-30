"""Independent integer checks of actual GLES candidates; no device/T280 replay."""
from pathlib import Path
import json,struct,subprocess,os,re,sys,bisect
r=Path(__file__).resolve().parents[2];out=r/'.codex/local-test/t279';report=r/'test/reports/t279';report.mkdir(exist_ok=True,parents=True)
Q=1<<20;CQ=256*Q;T=65536;D=120*T
def rne(n,d):
    sign=-1 if n<0 else 1;q,z=divmod(abs(n),d)
    return sign*(q+(2*z>d or (2*z==d and q%2)))
def signed(x):return x-(1<<32) if x>=1<<31 else x
def pair(a,i,v):a[i]=v&0xffffffff;a[i+1]=v>>32
def u64(a,i):return a[i]+(a[i+1]<<32)
manifest=json.loads((r/'docs/analysis-T238-gpu-water-M1-engineering-manifest-v1.json').read_text())
masks={e['id']:set(e['usedWordIndices']) for e in manifest['eventKinds']}
angles=[int(x,16) for x in re.findall(r'0x([0-9a-f]{8})u',(r/'entry/src/main/cpp/gpu_resident/angles_generated.h').read_text())]
def fixtures(K,Kw):
    water=16*K;col=water+16*Kw;h=col+2048;n=h+256;cases=[]
    def make(name,particles=(),W=None,S=None,params=None,error=0,cap=0):
        a=[0]*n;l=[0]*65536;p=[512*Q,8*Q,0,0,24,0,angles[(7680+7168)*4],0,0,cap]
        for i,v in (params or {}).items():p[i]=v
        a[h:h+3]=[1213420103,2,256];a[h+31]=3;a[h+39]=a[h+40]=1;a[h+43]=K;a[h+44]=Kw;a[h+122]=1;a[h+123]=T;a[h+124]=a[h+125]=2
        for c in range(128):
            s=S[c] if S else 0;w=W[c] if W else 0
            a[col+2*128+c]=a[col+10*128+c]=s;a[col+6*128+c]=w
        counts=[0,0]
        for source,(ns,mass,x,y,vx,vy) in enumerate(particles,1):
            slot=counts[ns];counts[ns]+=1;base=0 if ns==0 else water;stride=K if ns==0 else Kw
            vals=[mass,x,y,vx&0xffffffff,vy&0xffffffff,slot+1]
            vals+= [1,1,source,0,1,0,0,0,0,0] if ns==0 else [source,0,source,0,1,1,1,0,0,0]
            for f,v in enumerate(vals):a[base+f*stride+slot]=v
            l[(source-1)*16:source*16]=[source,0,source,0,ns,x//Q,mass,slot+1,1,1,x,y,slot+1,slot+2,0,0]
            a[col+(8 if ns==0 else 7)*128+x//Q]+=mass
        a[h+34]=sum(counts);a[h+41]=counts[0];a[h+42]=counts[1];a[h+43]=K-counts[0];a[h+44]=Kw-counts[1];a[h+39]=counts[0]+1;a[h+40]=counts[1]+1
        ms=sum(a[col+2*128:col+3*128])+sum(z[1] for z in particles if z[0]==0);mw=sum(a[col+6*128:col+7*128])+sum(z[1] for z in particles if z[0]==1)
        for i,v in [(56,ms),(58,mw),(64,ms),(66,mw),(112,sum(a[col+6*128:col+7*128])),(114,sum(a[col+7*128:col+8*128]))]:pair(a,h+i,v)
        for c in range(128):a[col+12*128+c]=a[col+10*128+c]+a[col+6*128+c];a[col+13*128+c]=sum(a[col+f*128+c] for f in [10,6,7,8])
        case={'name':name,'a':a,'ledger':l,'p':p,'error':error,'particles':list(particles)};cases.append(case);return case
    make('empty')
    for ns in [0,1]:
        for vx,vy in [(0,-16*Q),(3*Q,-Q),(-3*Q,-Q),(0,0)]:make(f'air-{ns}-{vx}-{vy}',[(ns,Q//8,64*Q+Q//2,100*Q,vx,vy)])
    make('water-arrive',[(1,Q//8,64*Q+Q//2,100,0,-16*Q)])
    make('sand-land',[(0,Q//8,64*Q+Q//2,0,0,0)])
    make('land-local-partial',[(0,Q//8,64*Q+Q//2,0,0,0)]*3)
    for ns in [0,1]:
        make(f'cross-{ns}',[(ns,Q//8,65*Q-Q//32,100*Q,16*Q,-Q)])
        make(f'cross-left-{ns}',[(ns,Q//8,64*Q+Q//32,100*Q,-16*Q,-Q)])
        make(f'wall-{ns}',[(ns,Q//8,128*Q-Q//32,100*Q,16*Q,-Q)])
    make('wet-sand',[(0,Q//8,64*Q+Q//2,25*Q,3*Q,-16*Q)],W=[50*Q]*128)
    make('surface-entry',[(0,Q//8,64*Q+Q//2,50*Q+100,0,-16*Q)],W=[50*Q]*128)
    make('turn-sand',[(0,Q//8,64*Q+Q//2,100*Q,0,Q)])
    W=[0]*128;W[64]=Q;make('water-spread',W=W,params={5:24})
    W=[0]*128;W[65]=CQ;make('water-capacity-wait',[(1,Q//8,65*Q-Q//32,100*Q,16*Q,-Q)],W=W)
    S=[0]*128;S[65]=605394;make('sand-highbed-wait',[(0,Q//8,65*Q-Q//32,100000,16*Q,0)],S=S,params={0:120})
    S=[0]*128;S[64]=605394;make('guard-zero-land',[(0,Q//8,64*Q+Q//2,605394,0,0)],S=S)
    S=[0]*128;S[64]=600000;make('bed-flow',S=S,params={7:24*Q})
    S=[0]*128;S[64]=S[70]=600000;make('bed-parallel-prefix',S=S,params={7:24*Q})
    make('top-wait',[(0,Q//8,64*Q+Q//2,CQ-100,0,16*Q)])
    for ns in [0,1]:make(f'multicross-{ns}',[(ns,Q//8,64*Q+Q//2,100*Q,1024*Q,-Q)])
    make('air-drag',[(0,Q//8,64*Q+Q//2,100*Q,3*Q,-Q)],params={2:960})
    make('flight-drag',[(1,Q//8,64*Q+Q//2,100*Q,3*Q,-Q)],params={3:960})
    make('wet-extreme',[(0,Q//8,64*Q+Q//2,25*Q,3*Q,-16*Q)],W=[50*Q]*128,params={4:960})
    make('arrival-y-order',[(1,Q//8,64*Q+Q//2,200000,0,-16*Q),(1,Q//8,64*Q+Q//2,100,0,-16*Q)])
    W=[0]*128;W[65]=200*Q;make('cross-highwater-arrival',[(1,Q//8,65*Q-Q//32,100*Q,16*Q,-Q)],W=W)
    make('fatal-event-cap',[(0,Q//8,64*Q+Q//2,0,0,0)]*2,error=12,cap=1)
    bad=make('fatal-feature',error=1);bad['a'][h+31]=7
    bad=make('fatal-stock',error=15);bad['a'][col+7*128+3]=1
    return cases,water,col,h,n
def verify(case,status,bank,events,info,K,Kw,water,col,h):
    a=case['a'];name=case['name'];assert status[0]==case['error'],(name,'error',status[:4])
    if case['error']:assert bank==a,(name,'rollback');return {'name':name,'result':'EXPECTED_REJECT','reason':status[0]}
    assert status[12]==1,(name,'candidate not ready');assert u64(bank,h+56)==u64(a,h+56) and u64(bank,h+58)==u64(a,h+58)
    # Independent whole head2 stage, integer proposals from a simultaneous
    # color snapshot, then deterministic edge order. No designer model import.
    S=a[col+10*128:col+11*128];W=a[col+6*128:col+7*128];J=a[col+7*128:col+8*128];P=a[col+8*128:col+9*128]
    low=max([-605394]+[S[i+1]-S[i]-605394 for i in range(127)])
    high=min([605394]+[S[i+1]-S[i]+605394 for i in range(127)])
    table=[signed(angles[i*4]) for i in range(15361)]
    accepted=max(bisect.bisect_left(table,low),min(case['p'][8]+7680,bisect.bisect_right(table,high)-1))-7680
    assert signed(bank[h+45])==accepted,(name,'angle projection')
    tangent=table[accepted+7680];waterRows=[]
    for color in [1,0]:
        proposals=[]
        for edge in range(color,127,2):
            head=[2*(S[c]+W[c])-(2*c+1)*tangent for c in [edge,edge+1]]
            d,rr=(edge,edge+1) if head[0]>=head[1] else (edge+1,edge)
            delta=abs(head[1]-head[0]);hd=max(head);bd=2*S[d]-(2*d+1)*tangent;br=2*S[rr]-(2*rr+1)*tangent
            q=min(case['p'][5]*delta//240,delta//4,W[d],max(0,hd-max(bd,br))//2,CQ-S[rr]-W[rr]-P[rr]-J[rr])
            if q:proposals.append((d,rr,q,W[d],W[rr]))
        for d,rr,q,da,rb in proposals:
            waterRows.append((d,rr,da,da-q,rb,rb+q));W[d]-=q;W[rr]+=q
    actualWater=[tuple(e[i] for i in [20,21,32,33,34,35]) for e in events if e[6]==10]
    assert actualWater==waterRows,(name,'complete head2 oracle')
    assert u64(bank,h+118)==1 and u64(bank,h+16)==1
    assert bank[h+14:h+16]==a[h+14:h+16] and bank[h+33:h+35]==a[h+33:h+35]
    spent=2621440-bank[h+54];local=sum(bank[col+14*128:col+15*128]);assert spent<=local<=2*spent
    for c in range(128):
        assert sum(bank[col+f*128+c] for f in [10,6,7,8])<=CQ,(name,'capacity',c)
        for f,base,stride in [(8,0,K),(7,water,Kw)]:assert sum(bank[base+i] for i in range(stride) if bank[base+stride+i]//Q==c)==bank[col+f*128+c],(name,'fold',f,c)
        assert bank[col+14*128+c]+bank[col+15*128+c]==244667
        if c<127:assert abs(bank[col+10*128+c+1]-bank[col+10*128+c]-signed(angles[(7680+signed(bank[h+45]))*4]))<=605394
    assert sum(bank[:K])+sum(bank[col+2*128:col+3*128])==u64(a,h+56)
    assert sum(bank[water:water+Kw])+sum(bank[col+6*128:col+7*128])==u64(a,h+58)
    kinds=[];arrivals=[]
    for ordinal,e in enumerate(events):
        kind=e[6];kinds.append(kind);assert e[0:2]==[1163088455,2] and e[7]==ordinal+1
        assert all(v==0 for i,v in enumerate(e) if i not in masks[kind]),(name,'unused event word',kind)
        assert e[56]<=e[57]<=T,(name,'time')
        # Independent per-segment signed rational velocity oracle. A WAIT
        # consumes residual time but applies force only for ceil(rSeg*lambda).
        if e[63]:
            ticks=(e[63]*u64(e,44)+u64(e,46)-1)//u64(e,46)
            ns=(e[52]>>18)&3;wet=e[58]==1;drag=case['p'][3] if ns==1 else case['p'][4] if wet else case['p'][2]
            accepted=signed(bank[h+45]);angle=(accepted+7680)*4
            acc=[rne(signed(angles[angle+1])*case['p'][0],Q),-rne(angles[angle+2]*case['p'][0],Q)]
            target=[rne(signed(angles[angle+1])*case['p'][1],Q),-rne(angles[angle+2]*case['p'][1],Q)]
            velocity=[rne(D*signed(e[28+i])+drag*ticks*target[i],D+drag*ticks) if wet else rne(D*signed(e[28+i]),D+drag*ticks)+rne(acc[i]*ticks,D) for i in range(2)]
            if kind==7:
                reason=(e[52]>>14)&15
                if reason in [1,6]:velocity[1]=0
                elif reason in [2,3,4]:velocity[0]=0
                elif reason==5:velocity[1 if e[27]==CQ else 0]=0
            assert [signed(e[30]),signed(e[31])]==velocity,(name,'T16 segment oracle',kind,e[56:64])
            if kind==7:assert e[57]==T
        if kind in [8,9]:
            mass=e[33]-e[32] if kind==8 else e[32]-e[33]
            assert mass>0 and e[36]-e[38]==mass and e[40]-e[41]==mass,(name,'solid budget',e)
            if kind==9:assert e[42]-e[43]==mass
            assert abs(e[33]-e[32])==abs(e[35]-e[34])
        if kind==5:
            mass=e[22];assert e[32]-e[33]==mass and e[35]-e[34]==mass
            ns=(e[52]>>18)&3;expected=mass if ns==0 else 0
            assert e[36]-e[38]==expected and e[40]-e[41]==expected and e[42]-e[43]==expected,(name,'cross budget')
        if kind==11:assert e[33]-e[32]==e[34]-e[35]==e[22];arrivals.append(e[18])
    if name.startswith('air-'):
        ns,m,x,y,vx,vy=case['particles'][0];stride=K if ns==0 else Kw;base=0 if ns==0 else water;drag=case['p'][2] if ns==0 else case['p'][3];vx1=rne(D*vx,D+drag*T);vy1=rne(D*vy,D+drag*T)+rne(-case['p'][0]*T,D)
        assert [bank[base+f*stride] for f in [1,2,3,4]]==[(x+rne((vx+vx1)*T,2*D))&0xffffffff,(y+rne((vy+vy1)*T,2*D))&0xffffffff,vx1&0xffffffff,vy1&0xffffffff],(name,'AIR integer oracle')
    if name=='wet-sand':
        vx=rne(D*3*Q,D+24*T);vy=rne(D*(-16*Q)+24*T*(-8*Q),D+24*T)
        assert bank[3*K]==vx&0xffffffff and bank[4*K]==vy&0xffffffff,(name,'WET velocity')
        assert bank[K]==64*Q+Q//2+rne((3*Q+vx)*T,2*D) and bank[2*K]==25*Q+rne((-16*Q+vy)*T,2*D)
    if name=='sand-land':assert bank[0]==0 and bank[col+2*128+64]==Q//8 and 8 in kinds
    if name=='land-local-partial':assert bank[col+2*128+64]==244667 and sum(bank[:K])==3*Q//8-244667
    if name.startswith('cross-') and name!='cross-highwater-arrival':assert 5 in kinds
    if name=='turn-sand':assert 14 in kinds
    if name=='surface-entry':assert 6 in kinds and bank[14*K]==1
    if name=='guard-zero-land':assert 8 not in kinds and bank[0]==Q//8
    if name=='water-spread':assert bank[col+6*128+63]>0 and bank[col+6*128+65]>0 and 10 in kinds
    if name=='bed-flow':assert 9 in kinds
    if name=='arrival-y-order':assert arrivals==[2,1],(name,'arrival order',arrivals)
    if name in ['water-arrive','cross-highwater-arrival']:assert 11 in kinds and bank[water]==0
    if name in ['water-capacity-wait','sand-highbed-wait']:assert 7 in kinds and 5 not in kinds
    return {'name':name,'result':'PASS','eventKinds':sorted(set(kinds)),'eventRows':len(events)}
def run(K,Kw):
    cases,water,col,h,n=fixtures(K,Kw);ip=out/f'cases-{K}-{Kw}.bin';op=out/f'output-{K}-{Kw}.bin'
    with ip.open('wb') as f:
        f.write(struct.pack('<4I',279,K,Kw,len(cases)))
        for c in cases:
            for v in [c['p'],c['a'],c['ledger']]:f.write(struct.pack('<'+'I'*len(v),*[x&0xffffffff for x in v]))
    env=os.environ.copy();env['PATH']='C:/Users/admin/AppData/Local/Programs/CokePLUS;'+env['PATH']
    if not os.environ.get('T279_VERIFY_ONLY'):
        p=subprocess.run([str(out/'quantum.exe'),str(ip),str(op)],cwd=r,env=env,text=True,capture_output=True,timeout=240)
        (report/f'gpu-{K}-{Kw}.txt').write_text(p.stdout+p.stderr);print(p.stdout+p.stderr);assert p.returncode==0,p.returncode
    raw=op.read_bytes();offset=16;results=[]
    def take(count):
        nonlocal offset
        v=list(struct.unpack_from('<'+'I'*count,raw,offset));offset+=count*4;return v
    for c in cases:
        status=take(16);info=take(3);bank=take(n);ev=take(info[2]);events=[ev[i:i+64] for i in range(0,len(ev),64)]
        results.append(verify(c,status,bank,events,info,K,Kw,water,col,h))
    assert offset==len(raw)
    return {'K':K,'Kw':Kw,'passed':len(results),'failed':0,'dispatchesPerQuantum':info[0],'candidateAllocatedBytes':info[1],'cases':results}
if __name__=='__main__':
    profiles=[(int(sys.argv[1]),int(sys.argv[2]))] if len(sys.argv)>1 else [(192,64),(192,128),(512,64),(512,128)]
    results=[run(*x) for x in profiles]
    result={'revision':'T279_CANDIDATE_V1','execution':'PC_SWIFTSHADER','device':'NOT_RUN','nativeAcceptance':'T280_NOT_IMPLEMENTED','scene':'T281_NOT_IMPLEMENTED','profiles':results}
    (report/('quantum-'+('-'.join(sys.argv[1:]) if len(sys.argv)>1 else 'all')+'.json')).write_text(json.dumps(result,indent=2));print(json.dumps({'passed':sum(x['passed'] for x in results),'failed':0}))
