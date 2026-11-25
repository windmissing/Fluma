"""Generate diagnostic-only small MAC fixtures. No GPU or product is launched."""

import hashlib
import json
import math
from pathlib import Path
import struct

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
LAYOUT = ROOT / 'docs/architecture-T607-g13-layout-v1.json'
BASE = ROOT / 'test/reports/t868/fixtures/F02-config128.bin'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def fp32(x):
    return struct.unpack('<f', struct.pack('<f', x))[0]


def fwords(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def digest_words(data):
    return struct.unpack('<8I', hashlib.sha256(data).digest())


def divergence(u, v, n):
    return [n * math.fsum((u[y*(n+1)+x+1], -u[y*(n+1)+x],
                           v[(y+1)*n+x], -v[y*n+x]))
            for y in range(n) for x in range(n)]


def mac_components(n):
    wave = math.pi / n
    amplitude = .25 * wave / (2 * math.sin(wave/2))
    axis = [math.cos(wave*(i+.5)) for i in range(n)]
    p = [amplitude * x * y for y in axis for x in axis]
    # Start from one scalar at each vertex. The discrete curl then cancels
    # exactly in cell divergence before the diagnostic gradient is added.
    psi = [fp32(round(2**24 * math.sin(wave*x)**2 * math.sin(wave*y)**2 / 128) / 2**24)
           if 0 < x < n and 0 < y < n else 0.0 for y in range(n+1) for x in range(n+1)]
    g_u, g_v, s_u, s_v = [], [], [], []
    for y in range(n):
        for x in range(n+1):
            at = y*(n+1)+x
            s_u.append(n*(psi[at+n+1]-psi[at]))
            gr = (p[y*n+x]-p[y*n+x-1]) * n if 0 < x < n else 0.0
            g_u.append(gr / 120)
    for y in range(n+1):
        for x in range(n):
            at = y*(n+1)+x
            s_v.append(-n*(psi[at+1]-psi[at]))
            gr = (p[y*n+x]-p[(y-1)*n+x]) * n if 0 < y < n else 0.0
            g_v.append(gr / 120)
    assert not any(divergence(s_u,s_v,n))
    return (g_u,g_v), (s_u,s_v), p


def build(profile, arm):
    n = profile['Nx']
    state = [0] * (profile['stateBytes']//4)
    state[:7] = [1395863879,1,1177760071,len(state),n,n,1]
    gradient, swirl, pressure = mac_components(n)
    faces = [[fp32(g + s) if arm == 'circulation' else fp32(g) if arm == 'projection' else 0.0
              for g,s in zip(g_face,s_face,strict=True)]
             for g_face,s_face in zip(gradient,swirl,strict=True)]
    layout = {entry['name']:entry for entry in profile['state']}
    for name, values in zip(('u','v'),faces,strict=True):
        region = layout[name]
        assert len(values) == region['words']
        at = region['wordStart']
        state[at:at+len(values)] = [fwords(x) for x in values]
    u,v = faces
    assert all(u[y*(n+1)] == u[y*(n+1)+n] == 0 for y in range(n))
    assert all(v[x] == v[n*n+x] == 0 for x in range(n))
    payload = struct.pack(f'<{len(state)}I',*state)
    config = list(struct.unpack('<128I',BASE.read_bytes()))
    config[4:6] = [n,n]
    config[10] = fwords(1/n)
    config[60:68] = digest_words(payload)
    config[76:84] = [0]*8
    config[76:84] = digest_words(struct.pack('<128I',*config))
    config_bytes = struct.pack('<128I',*config)
    label = f'n{n}-{arm}'
    (HERE / f'{label}-config128.bin').write_bytes(config_bytes)
    (HERE / f'{label}-state.bin').write_bytes(payload)
    expected = swirl if arm == 'circulation' else ([0.0]*len(u),[0.0]*len(v))
    expected_pressure = pressure if arm != 'zero' else [0.0]*n*n
    return dict(fixture=label, n=n, scope='F02 private approximation', configSHA=sha(config_bytes),
                embeddedConfigSHA=struct.pack('<8I',*config[76:84]).hex(), stateSHA=sha(payload),
                stateBytes=len(payload), workBytes=profile['workBytes'], packBytes=profile['renderPackBytes'],
                mg=[(x['Nx'],x['Ny']) for x in profile['MG']], inputNonzeroFaces=sum(x!=0 for x in u+v),
                inputDivMax=max(map(abs,divergence(u,v,n))), oracleVelocityMax=max(map(abs,expected[0]+expected[1])),
                oraclePressureRange=[min(expected_pressure),max(expected_pressure)],
                oracleProbe=dict(bottomU=expected[0][(n//4)*(n+1)+n//2],
                                 topU=expected[0][(3*n//4)*(n+1)+n//2],
                                 leftV=expected[1][(n//2)*n+n//4],
                                 rightV=expected[1][(n//2)*n+3*n//4]))


if __name__ == '__main__':
    layout = json.loads(LAYOUT.read_bytes())
    entries = [build(profile,arm) for profile in layout['profiles'] if profile['Nx'] in (16,32)
               for arm in ('circulation','projection','zero')]
    result = dict(layoutSHA=sha(LAYOUT.read_bytes()), baseConfigSHA=sha(BASE.read_bytes()), fixtures=entries)
    (HERE/'fixtures.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
