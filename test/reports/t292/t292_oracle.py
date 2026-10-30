"""T292 independent oracle v2 (256B row stride, T238 manifest masks, D=120*T).
Reproduces B/C/D/E/F/A/G results for K192/Kw64 T279 candidate output."""
import struct,json,re,bisect,copy,sys
raw=open(r'D:/hmprojects/HelloWorld/.codex/local-test/t279/output-192-64.bin','rb').read()
_,K,Kw,ncases=struct.unpack_from('<4I',raw,0)
water=16*K;col=water+16*Kw;h=col+2048;n=h+256
Q=1<<20;T=65536;D=120*T
ip=open(r'D:/hmprojects/HelloWorld/.codex/local-test/t279/cases-192-64.bin','rb').read()
o=16;fixtures=[]
for _ in range(ncases):
    p=list(struct.unpack_from('<10I',ip,o));o+=40
    a=list(struct.unpack_from('<%dI'%n,ip,o));o+=n*4
    l=list(struct.unpack_from('<65536I',ip,o));o+=262144
    fixtures.append(dict(p=p,a=a,ledger=l))
off=16;outs=[]
NAMES=['empty','air-0-0--16777216','air-0-3145728--1048576','air-0--3145728--1048576','air-0-0-0','air-1-0--16777216','air-1-3145728--1048576','air-1--3145728--1048576','air-1-0-0','water-arrive','sand-land','land-local-partial','cross-0','cross-left-0','wall-0','cross-1','cross-left-1','wall-1','wet-sand','surface-entry','turn-sand','water-spread','water-capacity-wait','sand-highbed-wait','guard-zero-land','bed-flow','bed-parallel-prefix','top-wait','multicross-0','multicross-1','air-drag','flight-drag','wet-extreme','arrival-y-order','cross-highwater-arrival','fatal-event-cap','fatal-feature','fatal-stock']
for i in range(ncases):
    status=struct.unpack_from('<16I',raw,off);off+=64
    info=struct.unpack_from('<3I',raw,off);off+=12
    bank=list(struct.unpack_from('<%dI'%n,raw,off));off+=n*4
    ew=info[2]
    assert ew%64==0 and status[1]==ew//64,(NAMES[i],ew,status[1])
    rows=[list(struct.unpack_from('<64I',raw,off+j*256)) for j in range(ew//64)];off+=ew*4
    outs.append(dict(name=NAMES[i],status=status,info=info,bank=bank,events=rows,fx=fixtures[i]))
assert off==len(raw)
def signed(v):return v-(1<<32) if v>=1<<31 else v
def rne(num,den):
    sign=-1 if num<0 else 1;q,z=divmod(abs(num),den)
    if z*2>den or (z*2==den and q%2==1):q+=1
    return sign*q
man=json.load(open(r'D:/hmprojects/HelloWorld/docs/analysis-T238-gpu-water-M1-engineering-manifest-v1.json',encoding='utf-8'))
M={e['id']:(int(e['usedMaskHex'],16)&0xffffffff,(int(e['usedMaskHex'],16)>>32)&0xffffffff) for e in man['eventKinds']}
res={}
# F
bad=0;total=0
for o_ in outs:
    for j,row in enumerate(o_['events']):
        total+=1;k=row[6];lo,hi=M[k]
        for i,w in enumerate(row):
            if w and not (lo if i<32 else hi) & (1<<(i%32)):bad+=(o_['name'],j,i);break
res['F_mask']=dict(rows=total,violations=bad)
# B
ok=0;fails=[]
for o_ in outs:
    mm=re.fullmatch(r'air-(\d+)-(-?\d+)-(-?\d+)',o_['name'])
    if not mm:continue
    ns=int(mm.group(1));vx=int(mm.group(2));vy=int(mm.group(3))
    x=64*Q+Q//2;y=100*Q
    drag=o_['fx']['p'][2] if ns==0 else o_['fx']['p'][3]
    base=0 if ns==0 else water;stride=K if ns==0 else Kw
    vx1=rne(D*vx,D+drag*T);vy1=rne(D*vy,D+drag*T)+rne(-o_['fx']['p'][0]*T,D)
    exp=[(x+rne((vx+vx1)*T,2*D))&0xffffffff,(y+rne((vy+vy1)*T,2*D))&0xffffffff,vx1&0xffffffff,vy1&0xffffffff]
    got=[o_['bank'][base+f*stride] for f in [1,2,3,4]]
    if exp==got:ok+=1
    else:fails.append((o_['name'],exp,got))
res['B_t16']=dict(passed=ok,failed=len(fails),detail=fails)
# C
rec=[o_ for o_ in outs if o_['name']=='arrival-y-order'][0]
ids=[e[18] for e in rec['events'] if e[6]==11]
res['C_arrive']=dict(order=ids,passed=ids==[2,1])
# E
rec=[o_ for o_ in outs if o_['name']=='water-spread'][0]
prod=[(e[20],e[21],e[32],e[33],e[34],e[35]) for e in rec['events'] if e[6]==10]
S=rec['fx']['a'][col+10*128:col+11*128];Wc=list(rec['fx']['a'][col+6*128:col+7*128])
angles=[int(x,16) for x in re.findall(r'0x([0-9a-f]{8})u',open(r'D:/hmprojects/HelloWorld/entry/src/main/cpp/gpu_resident/angles_generated.h',encoding='utf-8',errors='replace').read())]
atable=[signed(angles[i*4]) for i in range(15361)]
low=max([-605394]+[S[i+1]-S[i]-605394 for i in range(127)])
high=min([605394]+[S[i+1]-S[i]+605394 for i in range(127)])
accepted=max(bisect.bisect_left(atable,low),min(rec['fx']['p'][8]+7680,bisect.bisect_right(atable,high)-1))-7680
tangent=atable[accepted+7680]
exp_rows=[]
for color in [1,0]:
    props=[]
    for edge in range(color,127,2):
        hd=[2*(S[c]+Wc[c])-(2*c+1)*tangent for c in [edge,edge+1]]
        d,rr=(edge,edge+1) if hd[0]>=hd[1] else (edge+1,edge)
        delta=abs(hd[1]-hd[0]);hh=max(hd);bd=2*S[d]-(2*d+1)*tangent;br=2*S[rr]-(2*rr+1)*tangent
        q=min(24*delta//240,delta//4,Wc[d],max(0,hh-max(bd,br))//2,256*Q-S[rr]-Wc[rr]-rec['fx']['a'][col+8*128+rr]-rec['fx']['a'][col+7*128+rr])
        if q:props.append((d,rr,q))
    for d,rr,q in props:
        exp_rows.append((d,rr,Wc[d],Wc[d]-q,Wc[rr],Wc[rr]+q));Wc[d]-=q;Wc[rr]+=q
res['E_wateredge']=dict(rows=prod,match=prod==exp_rows)
# D
dc=0;df=[]
for o_ in outs:
    if o_['name'].startswith('fatal'):continue
    for e in o_['events']:
        k=e[6]
        if k==5:
            mass=e[22];ns=(e[52]>>18)&3;exp=mass if ns==0 else 0
            if abs(e[36]-e[38])!=exp or abs(e[40]-e[41])!=exp:df.append((o_['name'],'CROSS'))
            dc+=1
        elif k==8:
            mass=e[33]-e[32]
            if mass<=0 or e[36]-e[38]!=mass or e[40]-e[41]!=mass:df.append((o_['name'],'LAND'))
            dc+=1
        elif k==9:
            mass=e[32]-e[33]
            if mass<=0 or e[36]-e[38]!=mass or e[40]-e[41]!=mass or e[42]-e[43]!=mass:df.append((o_['name'],'BED'))
            dc+=1
res['D_budgets']=dict(checked=dc,failed=df)
# A/G
res['A_rollback']=all(o_['status'][0]!=0 and o_['bank']==o_['fx']['a'] for o_ in outs if o_['name'].startswith('fatal'))
res['G_reasons']={o_['name']:o_['status'][0] for o_ in outs if o_['name'].startswith('fatal')}
print(json.dumps(res,indent=1,default=str))
