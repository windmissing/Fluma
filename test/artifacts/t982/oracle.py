"""Independent 16×16 prescribed-bed fixture and conservation/traction oracle."""

from fractions import Fraction as F
import argparse
from functools import lru_cache
import hashlib
import json
import math
from pathlib import Path

N = 16
L = H = F(1,10)
DX = L/N
T = F(1,4)
RAMP = F(1,40)
MU = .01
WATER_AREA = F(17,3200)
LF, HF, DF, TF, RF = map(float, (L, H, DX, T, RAMP))
GAUSS = (
    (.09501250983763744,.18945061045506850), (.28160355077925891,.18260341504492359),
    (.45801677765722739,.16915651939500254), (.61787624440264375,.14959598881657673),
    (.75540440835500303,.12462897125553387), (.86563120238783174,.09515851168249278),
    (.94457502307323258,.06225352393864789), (.98940093499164993,.02715245941175410),
)


def points(lo,hi):
    mid = float((lo+hi)/2)
    radius = float((hi-lo)/2)
    for xi,weight in GAUSS:
        yield mid-radius*xi, radius*weight
        yield mid+radius*xi, radius*weight


def integral(fn,lo,hi):
    return 0.0 if hi == lo else math.fsum(w*fn(x) for x,w in points(lo,hi))


def a(t):
    if t < RAMP:
        return t*t/(2*RAMP*(T-RAMP))
    if t <= T-RAMP:
        return (t-RAMP/2)/(T-RAMP)
    return 1-(T-t)**2/(2*RAMP*(T-RAMP))


def adot(t):
    if t < RAMP:
        return t/(RAMP*(T-RAMP))
    if t <= T-RAMP:
        return 1/(T-RAMP)
    return (T-t)/(RAMP*(T-RAMP))


def bed(x,t,m):
    s = 2*x/L-1
    return DX*(F(31,4)-abs(s)/2+m*a(t)*s)


def a_numeric(t):
    if t < RF:
        return t*t/(2*RF*(TF-RF))
    if t <= TF-RF:
        return (t-RF/2)/(TF-RF)
    return 1-(TF-t)**2/(2*RF*(TF-RF))


def adot_numeric(t):
    if t < RF:
        return t/(RF*(TF-RF))
    if t <= TF-RF:
        return 1/(TF-RF)
    return (TF-t)/(RF*(TF-RF))


@lru_cache(maxsize=32768)
def bed_numeric(x,t,m):
    s = 2*x/LF-1
    return DF*(7.75-abs(s)/2+m*a_numeric(t)*s)


def crossing(x0,x1,b0,b1,y):
    return x0+(x1-x0)*(y-b0)/(b1-b0)


def area(i,j,t,m):
    x0,x1 = i*DX,(i+1)*DX
    y0,y1 = j*DX,(j+1)*DX
    b0,b1 = bed(x0,t,m),bed(x1,t,m)
    cuts = [x0,x1]
    for y in (y0,y1):
        if min(b0,b1)<y<max(b0,b1):
            cuts.append(crossing(x0,x1,b0,b1,y))
    cuts.sort()
    height = lambda x: min(DX,max(F(0),y1-bed(x,t,m)))
    return sum((hi-lo)*(height(lo)+height(hi))/2 for lo,hi in zip(cuts,cuts[1:]))


def segment_in_cell(i,j,t,m):
    x0,x1 = i*DF,(i+1)*DF
    y0,y1 = j*DF,(j+1)*DF
    b0,b1 = bed_numeric(x0,t,m),bed_numeric(x1,t,m)
    if b0 == b1:
        return (x0,x1) if y0 <= b0 < y1 else (x0,x0)
    intersections = sorted(crossing(x0,x1,b0,b1,y) for y in (y0,y1))
    lo,hi = max(x0,intersections[0]),min(x1,intersections[1])
    return (lo,hi) if hi > lo else (x0,x0)


def wall_sweep(i,j,t,m):
    lo,hi = segment_in_cell(i,j,t,m)
    return m*DF*adot_numeric(t)*(hi-lo)*((lo+hi)/LF-1)


def psi(x,y,t,m):
    s = x/LF
    b = bed_numeric(x,t,m)
    eta = (y-b)/(HF-b)
    return -LF*m*DF*adot_numeric(t)*(s*s-s)*(1-3*eta*eta+2*eta**3)


def uface(i,j,t,m):
    if i in (0,N):
        return 0.,0.
    t = float(t)
    x,hi = i*DF,(j+1)*DF
    lo = max(j*DF,bed_numeric(x,t,m))
    return (hi-lo,psi(x,hi,t,m)-psi(x,lo,t,m)) if hi > lo else (0.,0.)


def vface(i,j,t,m):
    if j in (0,N):
        return 0.,0.
    t = float(t)
    x0,x1,y = i*DF,(i+1)*DF,j*DF
    b0,b1 = bed_numeric(x0,t,m),bed_numeric(x1,t,m)
    if min(b0,b1)>=y:
        return 0.,0.
    if b0>y:
        x0 = crossing(x0,x1,b0,b1,y)
    elif b1>y:
        x1 = crossing(x0,x1,b0,b1,y)
    return x1-x0,psi(x0,y,t,m)-psi(x1,y,t,m)


def traction(x,t,i,m):
    y = bed_numeric(x,t,m)
    slope = (bed_numeric((i+1)*DF,t,m)-bed_numeric(i*DF,t,m))/DF
    stretch = math.sqrt(1+slope*slope)
    normal = (-slope/stretch,1/stretch)
    tangent = (1/stretch,slope/stretch)
    signed_rate = m*DF*adot_numeric(t)
    xi = x/LF
    visc_a = 6*LF*signed_rate*(xi*xi-xi)/(HF-y)**2
    visc_c = 2*signed_rate/LF
    p = 4*m*(xi-.5)+4*(y/HF-.5)
    mu_n = -2*MU*slope*visc_c/(1+slope*slope)
    mu_t = MU*((1+slope*slope)*visc_a+(1-slope*slope)*visc_c/(1+slope*slope))
    wall_v = signed_rate*(2*xi-1)
    result = []
    for f,d in ((-p,normal),(mu_n,normal),(mu_t,tangent)):
        fx,fy = f*d[0]*stretch,f*d[1]*stretch
        result.extend((fx,fy,(x-LF/2)*fy-y*fx,wall_v*fy))
    return result


def load_at(t,m):
    values = [0.0]*12
    for i in range(N):
        for x,w in points(i*DX,(i+1)*DX):
            values = [value+w*df for value,df in zip(values,traction(x,t,i,m),strict=True)]
    return values


def schedule(m):
    events = {}
    for i in range(N+1):
        x = i*DX
        direction = m*(2*x/L-1)
        if direction == 0:
            continue
        for j in range(1,N):
            ratio = (j-bed(x,0,m)/DX)/direction
            if 0<ratio<1:
                assert a(RAMP)<=ratio<=a(T-RAMP), (i,j,ratio)
                t = ratio*(T-RAMP)+RAMP/2
                assert bed(x,t,m)==j*DX
                events.setdefault(t,[]).append([i,j])
    roots = []
    for i in range(30):
        lo,hi = F(i,120),F(i+1,120)
        marks = [lo]+sorted(t for t in events if lo<t<hi)+[hi]
        roots.append(list(zip(marks,marks[1:])))
    assert max(map(len,roots)) <= 4
    return events,roots


def measure(m,roots):
    U = [(i,j) for j in range(N) for i in range(N+1)]
    V = [(i,j) for j in range(N+1) for i in range(N)]
    cells = [(i,j) for j in range(N) for i in range(N)]
    flux_totals, loads = [], [0.0]*12
    balance_max, area_diff_max, swept_cell_max, close_bad, open_close_count = 0.,0.,0.,0.,0
    interval_results = []
    for root_id,children in enumerate(roots,1):
        for child,(lo,hi) in enumerate(children):
            u = [integral(lambda t:uface(i,j,t,m)[1],lo,hi) for i,j in U]
            v = [integral(lambda t:vface(i,j,t,m)[1],lo,hi) for i,j in V]
            lengths = [integral(lambda t:fn(i,j,t,m)[0],lo,hi) for fn,ids in ((uface,U),(vface,V)) for i,j in ids]
            a0,a1 = ([area(i,j,t,m) for i,j in cells] for t in (lo,hi))
            assert sum(a0)==sum(a1)==WATER_AREA
            sweeps = [integral(lambda t:wall_sweep(i,j,t,m),lo,hi) for i,j in cells]
            for (i,j),before,after,sweep in zip(cells,a0,a1,sweeps,strict=True):
                u0,v0 = j*(N+1)+i,j*N+i
                div = u[u0+1]-u[u0]+v[v0+N]-v[v0]
                balance_max = max(balance_max,abs(div-sweep))
                area_diff_max = max(area_diff_max,abs(sweep-float(before-after)))
                swept_cell_max = max(swept_cell_max,abs(sweep))
            for amount,length in zip(u+v,lengths,strict=True):
                if length==0:
                    close_bad = max(close_bad,abs(amount))
            open_close_count += sum((fn(i,j,lo,m)[0]>0)!=(fn(i,j,hi,m)[0]>0)
                                    for fn,ids in ((uface,U),(vface,V)) for i,j in ids)
            midflux = math.fsum(u[j*(N+1)+N//2] for j in range(N))
            flux_totals.append(midflux)
            load = [0.0]*12
            for t,w in points(lo,hi):
                load = [x+w*y for x,y in zip(load,load_at(t,m),strict=True)]
            loads = [x+y for x,y in zip(loads,load,strict=True)]
            interval_results.append(dict(root=root_id,child=child,begin=str(lo),end=str(hi),
                                         midFaceFluxM2=midflux,wetAreaM2=str(sum(a1)),
                                         leftWaterAreaM2=str(sum(x for (i,_),x in zip(cells,a1,strict=True) if i<N//2))))
    assert balance_max<1e-10 and area_diff_max<1e-12 and close_bad==0
    gap = min((H-bed(i*DX,t,m))/DX for i in range(N+1) for t in (0,T))
    assert gap >= 4
    return dict(roots=len(roots),intervals=sum(map(len,roots)),maxChildren=max(map(len,roots)),
                wetAreaM2=str(WATER_AREA),minimumWaterGapCells=str(gap),
                midlineFluxM2=math.fsum(flux_totals),expectedMidlineFluxM2=str(-m*L*DX/4),
                maxCellDivMinusIndependentSweepM2=balance_max,maxCellSweepMinusEndpointDifferenceM2=area_diff_max,
                maskOnlyZeroFluxMaxImbalanceM2=swept_cell_max,closedFaceLeakM2=close_bad,
                faceEndpointOpenChanges=open_close_count,sandImpulseTorqueWork=loads,
                waterImpulseTorqueWork=[-x for x in loads],childResults=interval_results)


def compute():
    assert a(0) == 0 and a(T) == 1
    assert adot(0) == adot(T) == 0
    fixture = dict(schema='G14_PREVIEW16_FIXTURE_V1',Nx=N,Ny=N,Lm=str(L),Hm=str(H),hM=str(DX),
                   durationS=str(T),rampS=str(RAMP),nominalStepS='1/120',rhoKgM3=1000,muPaS=MU,
                   pressureReference='relative_gauge',arm=[])
    results = {}
    for m,name in ((0,'fixed'),(1,'moving'),(-1,'mirror')):
        events,roots = schedule(m)
        fixture['arm'].append(dict(name=name,m=m,vertices=[
            dict(key=i,xM=str(i*DX),bedAtStartM=str(bed(i*DX,0,m)),bedAtEndM=str(bed(i*DX,T,m))) for i in range(N+1)],
            events=[dict(t=str(t),vertexY=events[t]) for t in sorted(events)],
            intervals=[[[str(lo),str(hi)] for lo,hi in children] for children in roots]))
        results[name] = measure(m,roots)
    for m,name in ((0,'fixed'),(1,'moving'),(-1,'mirror')):
        assert abs(results[name]['midlineFluxM2']+float(m*L*DX/4))<1e-13
    for i,(move,mirror) in enumerate(zip(results['moving']['sandImpulseTorqueWork'],
                                         results['mirror']['sandImpulseTorqueWork'],strict=True)):
        assert abs(move*(-1 if i%4 in (0,2) else 1)-mirror)<1e-12
    return fixture,results


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--out',type=Path,default=Path(__file__).resolve().parent)
    args = parser.parse_args()
    args.out.mkdir(parents=True,exist_ok=True)
    fixture, results = compute()
    design = json.dumps(fixture,indent=2)+'\n'
    (args.out/'fixtures.json').write_text(design,encoding='utf-8',newline='\n')
    results = dict(schema='G14_PREVIEW16_ORACLE_V1',fixtureSHA=hashlib.sha256(design.encode()).hexdigest(),arms=results)
    (args.out/'oracle.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8',newline='\n')
