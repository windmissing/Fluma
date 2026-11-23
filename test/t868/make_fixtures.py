"""T868 fixture generator: F02/F03/F05 Config128 + State originals for the T833 candidate PE.

Matches the C++ validation in g13vulkan_boot/boot.cpp exactly:
- Config 128 words based on kRestConfig (REST scene, 64x64, sw=49536)
- F03: boundary=2 (periodic), viscosity 1/16, motion/gravity zeroed
- F02: boundary=1, config[7]=3 (private project)
- F05: boundary=1, viscosity 1/64
- config[60..68] = SHA-256 of state words (little-endian word digest)
- config[76..84] = SelfHash (SHA-256 of config with those 8 words zeroed)
- State header words 0..255 + velocity/pi slots at state[1]/state[2]/state[3] offsets
"""
import hashlib
import json
import math
import os
import re
import struct
import sys

ROOT = 'D:/hmprojects/HelloWorld'

VK_BOOT = os.path.join(ROOT, '.codex/local-test/t833-candidate-ninja/g13_assets/vk_boot_spirv.h')

kMagic_state = 1395863879
kMagic_profile = 1177760071

SW = 49536   # 64x64 profile state words
NX = NY = 64
U_AT, U_WORDS = 256, 4160        # state[1]
V_AT, V_WORDS = 4416, 4160       # state[2]
PI_AT, PI_WORDS = 8576, 8192     # state[3]


def sha_le_words(blob):
    digest = hashlib.sha256(blob).digest()
    return [int.from_bytes(digest[4 * i:4 * i + 4], 'little') for i in range(8)]


def f32(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def rest_config():
    with open(VK_BOOT, encoding='utf-8', errors='replace') as handle:
        data = handle.read()
    i = data.find('kRestConfig')
    j = data.find('};', i)
    words = [int(x) for x in re.findall(r'(\d+)u', data[i:j])]
    assert len(words) == 128
    return words


def make_config(case):
    c = list(rest_config())
    # Tracer count zero: fieldAudit requires z<c[28] tracer id patterns; our seed
    # carries no tracers, so c[28]=0 keeps the whole tracer region legitimately empty.
    c[28] = 0
    if case == 'F03':
        c[6] = 2                       # periodic boundary
        c[7] = 1                       # diagnostic scope required by boot.cpp gate
        c[20] = 0                      # motion off
        c[24] = c[25] = 0              # gravity off
        c[18] = 0x3d800000             # viscosity 1/16 (validated: .0625 or .125)
        c[17] = 0x404f4000             # exactNuBits: m=62.5 over l=1,r=1000 -> 1/16
    elif case == 'F02':
        c[6] = 1
        c[7] = 3                       # private project scope
        c[18] = 0x3c800000             # REST nu 1/64 (15.625/1000)
    elif case == 'F05':
        c[6] = 1                       # closed box
        c[7] = 1
        c[18] = 0x3c800000             # nu 1/64 matches REST l/r/m exactly
    else:
        raise ValueError(case)
    c[76:84] = [0] * 8
    c[76:84] = sha_le_words(struct.pack('<128I', *c))
    return c


def make_state(boundary, ufield, vfield=None, pifield=None):
    assert len(ufield) == U_WORDS
    state = [0] * SW
    state[0] = kMagic_state
    state[1] = 1
    state[2] = kMagic_profile
    state[3] = SW
    state[4] = NX
    state[5] = NY
    state[6] = boundary
    state[U_AT:U_AT + U_WORDS] = [w & 0xFFFFFFFF for w in ufield]
    if vfield:
        assert len(vfield) == V_WORDS
        state[V_AT:V_AT + V_WORDS] = [w & 0xFFFFFFFF for w in vfield]
    if pifield:
        # pi cells are Q32.32 fixed-point wide pairs (lo, hi) — coldAudit gauge sums them
        # as signed 64-bit integers; ledger reads them via sfloat(pi,-32).
        assert len(pifield) * 2 <= PI_WORDS
        for i, value in enumerate(pifield):
            word = int(round(float(value) * 2.0 ** 32)) & 0xFFFFFFFFFFFFFFFF
            state[PI_AT + 2 * i] = word & 0xFFFFFFFF
            state[PI_AT + 2 * i + 1] = (word >> 32) & 0xFFFFFFFF
    return state


def bind(config, state):
    config[60:68] = sha_le_words(struct.pack('<%dI' % len(state), *state))
    config[76:84] = [0] * 8
    config[76:84] = sha_le_words(struct.pack('<128I', *config))
    return config


def write_fixture(out_dir, case, ufield, vfield=None, pifield=None, tag=''):
    config = make_config(case)
    state = make_state(config[6], ufield, vfield, pifield)
    config = bind(config, state)
    os.makedirs(out_dir, exist_ok=True)
    cfg_path = os.path.join(out_dir, f'{case}{tag}-config128.bin')
    st_path = os.path.join(out_dir, f'{case}{tag}-state.bin')
    with open(cfg_path, 'wb') as handle:
        handle.write(struct.pack('<128I', *config))
    with open(st_path, 'wb') as handle:
        handle.write(struct.pack('<%dI' % SW, *state))
    return cfg_path, st_path, config, state


def build_all(out_dir):
    n = 64
    pi = math.pi
    info = {}
    # F03: periodic shear eigenmode, u depends on y only, J=1 initial amplitude
    eig = 4 * (n * math.sin(pi / n)) ** 2
    nu = 1 / 16
    amp = 0.25 * (1 + nu * eig / 120) ** (-30)
    u_f03 = [f32(amp * math.sin(2 * pi * (j + 0.5) / n)) for j in range(n) for _ in range(n + 1)]
    info['F03'] = write_fixture(out_dir, 'F03', u_f03, tag='')
    # F02: private projection from rest (zero field)
    u_f02 = [f32(0.0)] * (n * (n + 1))
    info['F02'] = write_fixture(out_dir, 'F02', u_f02, tag='')
    # F05: closed-box manufacturing solution (T631)
    a = 0.25 * math.exp(-0.25)
    u_f05 = [f32(0.0 if i in (0, n) else
                  a * math.sin(pi * i / n) ** 2 * math.sin(2 * pi * (j + 0.5) / n))
             for j in range(n) for i in range(n + 1)]
    v_f05 = [f32(0.0 if j in (0, n) else
                  -a * math.sin(2 * pi * (i + 0.5) / n) * math.sin(pi * j / n) ** 2)
             for j in range(n + 1) for i in range(n)]
    pi_f05 = [0.125 * math.exp(-0.5) * math.cos(pi * (i + 0.5) / n) * math.cos(pi * (j + 0.5) / n)
              for j in range(n) for i in range(n)]
    info['F05'] = write_fixture(out_dir, 'F05', u_f05, v_f05, pi_f05, tag='')
    manifest = {}
    for case, (cfg_path, st_path, config, state) in info.items():
        manifest[case] = {
            'config': cfg_path.replace('\\', '/'),
            'configSHA256': hashlib.sha256(open(cfg_path, 'rb').read()).hexdigest(),
            'state': st_path.replace('\\', '/'),
            'stateSHA256': hashlib.sha256(open(st_path, 'rb').read()).hexdigest(),
            'stateWords': len(state),
            'boundary': config[6],
            'viscosityBits': config[18],
        }
    with open(os.path.join(out_dir, 'fixtures.json'), 'w', encoding='utf-8') as handle:
        json.dump(manifest, handle, indent=2)
    return manifest


if __name__ == '__main__':
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'test/reports/t868/fixtures')
    result = build_all(out)
    print(json.dumps(result, indent=2))
