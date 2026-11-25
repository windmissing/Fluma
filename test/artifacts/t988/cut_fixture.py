"""Check 16×16 wet-graph feasibility using T981's archived real MAC words."""

import hashlib
import importlib.util
import json
import math
import struct
from fractions import Fraction
from pathlib import Path


HERE = Path(__file__).resolve().parent
REPORT = HERE.parents[2] / 'test/reports/t981/runs'


def geometry():
    module = importlib.util.spec_from_file_location('t982_oracle', HERE.parent/'t982/oracle.py')
    reference = importlib.util.module_from_spec(module)
    module.loader.exec_module(reference)
    return reference


def mac_state():
    folder = REPORT/'g13-16-circulation'
    boot, = (r for line in (folder/'preview.jsonl').read_bytes().splitlines()
             if (r := json.loads(line)).get('state') == 'T978_COARSE_PREVIEW_ROOT_RETIRED')
    archive = boot['t833QuantumArchive']
    cold = (folder/'preview.quantum').read_bytes()
    at = archive['fieldOffset']+archive['segmentsBytes'][0]
    state = cold[at:at+archive['segmentsBytes'][1]]
    state_sha = hashlib.sha256(state).hexdigest()
    assert state_sha == boot['g13CoarsePreview']['stateSHA']
    return (struct.unpack_from('<272f',state,256*4), struct.unpack_from('<272f',state,528*4), state_sha)


def correct(ref, arm, u, v):
    n, h = ref.N, float(ref.DX)
    lo,hi = Fraction(1,8),Fraction(2,15)
    dt = float(hi-lo)
    swept = [float(ref.area(i,j,lo,arm)-ref.area(i,j,hi,arm)) for j in range(n) for i in range(n)]
    wet = {i+n*j for j in range(n) for i in range(n)
           if ref.area(i,j,lo,arm)>0 or ref.area(i,j,hi,arm)>0}
    faces = []
    def face(geometry_fn, i, j, a, b, speed, midline):
        length = ref.integral(lambda t: geometry_fn(i,j,t,arm)[0],lo,hi)/dt
        if length > 0:
            assert a in wet and b in wet
            faces.append((a,b,length*dt/(1000*h),length*speed*.1,midline))
    for j in range(n):
        for i in range(1,n):
            slot = j*(n+1)+i
            face(ref.uface,i,j,i-1+j*n,i+j*n,u[slot],i==n//2)
    for j in range(1,n):
        for i in range(n):
            slot = j*n+i
            face(ref.vface,i,j,i+(j-1)*n,i+j*n,v[slot],False)

    flux_before = [0.]*(n*n)
    adjacency = {i: [] for i in wet}
    for a,b,weight,speed,_ in faces:
        flux_before[a] += speed
        flux_before[b] -= speed
        adjacency[a].append(b)
        adjacency[b].append(a)
    anchor = min(wet)
    seen, frontier = set(), {anchor}
    while frontier:
        idx = frontier.pop()
        seen.add(idx)
        frontier.update(set(adjacency[idx])-seen)
    assert seen == wet

    def matvec(x):
        result = [0.]*(n*n)
        for a,b,weight,_,_ in faces:
            dv = weight*(x[a]-x[b])
            result[a] += dv
            result[b] -= dv
        result[anchor] = 0.
        return result

    residual = [s/dt-f for s,f in zip(swept,flux_before,strict=True)]
    assert abs(math.fsum(residual)) < 1e-16
    residual[anchor] = 0.
    direction = residual.copy()
    p = [0.]*(n*n)
    dot = lambda a,b: math.fsum(x*y for x,y in zip(a,b,strict=True))
    norm = dot(residual,residual)
    count = 0
    while norm > 1e-29 and count < len(wet):
        step = matvec(direction)
        alpha = norm/dot(direction,step)
        p = [x+alpha*y for x,y in zip(p,direction,strict=True)]
        residual = [x-alpha*y for x,y in zip(residual,step,strict=True)]
        next_norm = dot(residual,residual)
        direction = [x+next_norm/norm*y for x,y in zip(residual,direction,strict=True)]
        norm = next_norm
        count += 1
    assert norm <= 1e-29
    flux_after = [0.]*(n*n)
    midline = 0.
    for a,b,weight,q,mid in faces:
        q += weight*(p[a]-p[b])
        flux_after[a] += q
        flux_after[b] -= q
        if mid:
            midline += q*dt
    err = max(abs(s-q*dt) for s,q in zip(swept,flux_after,strict=True))
    assert err < 1e-16
    return dict(interval=[str(lo),str(hi)],wetCells=len(wet),openFaces=len(faces),iterations=count,
                preCorrectMaxM2=max(abs(s-q*dt) for s,q in zip(swept,flux_before,strict=True)),
                correctedMaxM2=err,midlineM2=midline)


def compute():
    ref = geometry()
    u,v,state_sha = mac_state()
    n = ref.N
    reflected_u = [-u[y*(n+1)+n-x] for y in range(n) for x in range(n+1)]
    reflected_v = [v[y*n+n-1-x] for y in range(n+1) for x in range(n)]
    fixed = correct(ref,0,u,v)
    moving = correct(ref,1,u,v)
    mirror = correct(ref,-1,reflected_u,reflected_v)
    zero_source = correct(ref,1,[0.]*len(u),[0.]*len(v))
    assert abs(mirror['midlineM2']+moving['midlineM2']) < 1e-16
    assert abs(zero_source['midlineM2']-moving['midlineM2']) < 1e-16
    assert abs(fixed['midlineM2']) < 1e-16
    return dict(schema='G14_T988_FROZEN_MAC_OFFLINE',sourceStateSHA=state_sha, moving=moving,
                mirror=mirror,fixed=fixed,zeroSourceMoving=zero_source)


if __name__ == '__main__':
    print(json.dumps(compute(), indent=2))
