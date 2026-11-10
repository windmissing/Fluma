"""Generate independent M2 V3 constants from the frozen engineering manifest."""
from pathlib import Path
import argparse
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[6]
MANIFEST = ROOT / 'docs/analysis-T445-gpu-M2-wire-v3.json'


def generate():
    raw = MANIFEST.read_bytes()
    spec = json.loads(raw)
    if spec['schema'] != 'T445_M2_ENGINEERING_WIRE_V3' or spec['scalarBytes'] != 4 or spec['endianness'] != 'little':
        raise ValueError('unsupported M2 manifest')
    for file, digest in spec['references'].items():
        if hashlib.sha256((ROOT / file).read_bytes()).hexdigest() != digest:
            raise ValueError(f'changed M2 reference: {file}')
    cpp = ['#pragma once', '#include <cstdint>', 'namespace grs::m2 {',
           f'inline constexpr char kLayoutSha[] = "{hashlib.sha256(raw).hexdigest()}";']
    glsl = []

    def constant(name, value, prefix=''):
        cpp.append(f'inline constexpr uint32_t {name} = {value}u;')
        glsl.append(f'const uint M2_{prefix}{name} = {value}u;')

    def fields(name, layout):
        words = layout['words']
        if len(words) != layout['wordCount']:
            raise ValueError(f'{name} length mismatch')
        cpp.append(f'namespace {name} {{')
        constant('wordCount', len(words), name + '_')
        for i, field in enumerate(words):
            if re.match(r'^reserved', field, re.I):
                continue
            # namespace is a C++ keyword and GLSL has a macro for it.
            identifier = re.sub(r'[^A-Za-z0-9_]+', '_', field)
            constant('namespaceWord' if identifier == 'namespace' else identifier, i, name + '_')
        cpp.append('}')

    constant('kPolicyId', spec['policyId'])
    for name, number in spec['versions'].items():
        constant('rev_' + name, number)
    for name, number in spec['cold'].items():
        if isinstance(number, int):
            constant('cfg_' + name, number)
    for name, layout in spec['layouts'].items():
        fields(name, layout)
    for domain in ('stopCodes', 'fatalReason'):
        for name, number in spec[domain].items():
            constant(name, number)
    e = spec['source128']
    constant('sourceMagic', e['magic'])
    constant('sourceSeal', e['rowSeal'])
    for name, row in e['kinds'].items():
        constant(name, row['code'])
        words = row['usedWords']
        if len(words) != len(set(words)) or any(not 0 <= w < 96 for w in words):
            raise ValueError('invalid SOURCE128 used words')
        masks = [sum(1 << (w % 32) for w in words if w // 32 == part) for part in range(4)]
        if masks != row['usedMaskU32']:
            raise ValueError(f'wrong SOURCE128 mask: {name}')
        for i, mask in enumerate(masks):
            constant(f'{name}_mask{i}', mask)

    cpp += ['struct WordRange { uint32_t begin, end; };',
            'struct Profile {',
            ' uint32_t k, kw, pad, stateWords, pubWords, sourceRows;',
            ' uint32_t sourceBytes, quantumBytes, arenaBytes, payloadBytes, coldTransientBytes;',
            ' uint32_t sourceDispatches, quantumDispatches;',
            ' WordRange sourceLedger, index0, index1, control, sourceOrigins, config, quantumOrigins;',
            '};', 'inline constexpr Profile kProfiles[] = {']
    for row in spec['resources']:
        s, q = row['sourceWorkRanges'], row['quantumWorkRanges']
        for ranges, byte_count in [(s, row['sourceWorkBytesV3']), (q, row['quantumWorkBytesV3'])]:
            offset = 0
            for a, b in ranges.values():
                if a != offset or b <= a or b > 2**29:
                    raise ValueError(f'non-contiguous WORK range {a}..{b}')
                offset = b
            if offset * 4 != byte_count:
                raise ValueError('WORK byte accounting')
        vals = [row[k] for k in ['K', 'Kw', 'pad', 'stateWords', 'publicationWords', 'sourceRows', 'sourceWorkBytesV3',
            'quantumWorkBytesV3', 'sourceArenaBytes', 'totalV3PayloadBytes', 'maxColdTransientEventBytes', 'sourceDispatch', 'quantumDispatch']]
        ranges = [s[k] for k in ['ledger', 'index0', 'index1', 'control', 'originU64', 'config']] + [q['originU64']]
        cpp.append(' {' + ', '.join(map(str, vals)) + ', ' + ', '.join('{' + ', '.join(map(str, v)) + '}' for v in ranges) + '},')
    cpp += ['};', 'constexpr bool ValidProfiles(){', ' for(const auto& p:kProfiles){',
            '  if(p.sourceRows!=2*p.k+1 || p.arenaBytes!=p.sourceRows*512 || p.sourceDispatches!=13 ||',
            '     p.sourceLedger.end-p.sourceLedger.begin!=4096*16 || p.index0.end-p.index0.begin!=p.pad ||',
            '     p.index1.end-p.index1.begin!=p.pad || p.control.end-p.control.begin!=64 ||',
            '     p.sourceOrigins.end-p.sourceOrigins.begin!=8192 || p.quantumOrigins.end-p.quantumOrigins.begin!=8192 ||',
            '     p.config.end*4!=p.sourceBytes || p.quantumOrigins.end*4!=p.quantumBytes) return false;',
            ' }', ' return true;', '}', 'static_assert(ValidProfiles(), "Invalid T445 profiles");', '}']
    return '\n'.join(cpp) + '\n', '\n'.join(glsl) + '\n'


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--check', action='store_true')
    args = p.parse_args()
    for name, content in zip(('layout_generated.h', 'layout_generated.glsl'), generate()):
        path = Path(__file__).with_name(name)
        if args.check:
            if path.read_text(encoding='utf-8') != content:
                raise SystemExit(f'stale generated file: {path}')
        else:
            path.write_text(content, encoding='utf-8', newline='\n')
