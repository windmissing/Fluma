# T287 tester independent oracle (key excerpts). Layout derived independently
# from test/gpu_resident/t279_quantum_host.cpp: params 10w, state n=h+256,
# ledger 65536w, output per case: status16+info3+bank n+events info[2].
import struct
raw=open(r'D:/hmprojects/HelloWorld/.codex/local-test/t279/output-192-64.bin','rb').read()
K,Kw=192,64
water=16*K;col=water+16*Kw;h=col+2048;n=h+256
ip=open(r'D:/hmprojects/HelloWorld/.codex/local-test/t279/cases-192-64.bin','rb').read()
o=16;fixtures=[]
for _ in range(38):
    p=list(struct.unpack_from('<10I',ip,o));o+=40
    a=list(struct.unpack_from('<%dI'%n,ip,o));o+=n*4
    l=list(struct.unpack_from('<65536I',ip,o));o+=262144
    fixtures.append(dict(p=p,a=a,ledger=l))
off=16;outs=[]
for i in range(38):
    status=struct.unpack_from('<16I',raw,off);off+=64
    info=struct.unpack_from('<3I',raw,off);off+=12
    bank=list(struct.unpack_from('<%dI'%n,raw,off));off+=n*4
    ne=info[2]
    ev=[list(struct.unpack_from('<64I',raw,off+j*64)) for j in range(ne//64)];off+=ne*4
    outs.append(dict(status=status,info=info,bank=bank,events=ev,fx=fixtures[i]))
assert off==len(raw)
fails=[]
for o_ in outs:
    if o_['name'] if 'name' in o_ else False: pass
# A rollback
for idx,o_ in enumerate(outs):
    if idx in (35,36,37):
        if o_['status'][0]==0:fails.append((idx,'no-reject'))
        if o_['bank']!=o_['fx']['a']:fails.append((idx,'rollback!=input'))
print('A rollback failures:',fails)
Q=1<<20;T=128;D=2*Q
def signed(v):return v-(1<<32) if v>=1<<31 else v
def rne(num,den):
    sign=-1 if num<0 else 1;q,z=divmod(abs(num),den)
    if z*2>den or (z*2==den and q%2==1):q+=1
    return sign*q
# D cross budgets
for idx,nm in [(12,'cross-0'),(15,'cross-1')]:
    e=[x for x in outs[idx]['events'] if x[6]==5][0]
    ns=(e[52]>>18)&3
    print('D',nm,'ns',ns,'mass',e[22],'solidDelta',e[36]-e[38],'localDelta',e[40]-e[41])
# E spread
e=[x for x in outs[21]['events'] if x[6]==10][0]
print('E spread donor,recv,dW,rW:',e[20],e[21],e[32]-e[33],e[35]-e[34])
# event row stride probe (NOT_INDEPENDENT evidence)
flat=[]
for row in outs[33]['events']: flat+=row
mag=[j for j,w in enumerate(flat) if w==1163088455]
print('F event magic offsets:',mag,'gaps',[mag[k+1]-mag[k] for k in range(len(mag)-1)])
