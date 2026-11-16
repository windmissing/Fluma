"""Generate V5 ABI without changing the V4 producer or its readers."""
from pathlib import Path
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[2]
DEST = ROOT / 'entry/src/main/cpp/gpu_resident/m3b'


def identifier(name):
    return re.sub(r'[^A-Za-z0-9_]', '_', name)


def generate():
    paths = [ROOT / 'docs/architecture-T512-v5-engineering-manifest-v1.json',
             ROOT / 'docs/architecture-T527-v5-cold-manifest-v1.json']
    engineering, cold = [json.loads(p.read_text(encoding='utf-8')) for p in paths]
    design = engineering['designTables']
    DEST.mkdir(parents=True, exist_ok=True)
    lines = ['#pragma once', '#include <array>', '#include <cstddef>', '#include <cstdint>',
             'namespace grs::m3b {',
             'inline constexpr uint32_t kWireRevision=5,kFeatures=31,kChannels=15,kAuditMask=0x3ffff;',
             'inline constexpr uint32_t kColumns=128,kLayers=64,kOrigins=4096;',
             'inline constexpr uint64_t kColdCapacity=524375552ULL;']
    for path, name in zip(paths, ['Engineering', 'Cold']):
        lines.append(f'inline constexpr const char* k{name}Sha="{hashlib.sha256(path.read_bytes()).hexdigest()}";')
    for name, fields in [('header256', design['header256']),
                         ('completion256', design['completion256']),
                         ('cohort4', design['cohort4']),
                         ('certificate16', design['certificate16']),
                         ('aux4', design['aux4']),
                         ('input64', design['sensorInput64']['words']),
                         ('provenance32', design['provenance32']['words']),
                         ('coldHeader256', cold['header256']),
                         ('descriptor32', cold['descriptor32']),
                         ('rootRecord128', cold['rootRecord128'])]:
        lines.append(f'namespace {name} {{')
        for i, field in enumerate(fields):
            lines.append(f'inline constexpr size_t {identifier(field)}={i};')
        lines.append('}')
    lines += ['struct Segment {size_t begin,bytes,stride;constexpr size_t End()const{return begin+bytes;}};',
              'struct LineageProfile {uint32_t k,kw,e,ea,sourceRows;size_t oldStateBytes,stateBytes,payloadBytes,rootBytes,fullBytes;std::array<LineageSegment,4> extension;};',
              'inline constexpr std::array<LineageProfile,4> kLineageProfiles={{']
    for p, c in zip(engineering['profiles'], cold['profiles']):
        assert (p['K'], p['Kw']) == (c['K'], c['Kw'])
        ext = p['stateExtensionSegments']
        end = p['stateBytesV4']
        for s in ext:
            assert s['byteStart'] == end and s['byteEnd'] == end + s['bytes']
            end = s['byteEnd']
        assert end == p['stateBytesV5'] == c['stateBytes']
        scalar = [p['K'], p['Kw'], p['physicalRowsPerScope'], p['provenanceRowsPerScope'],
                  c['sourceRows'], p['stateBytesV4'], end, p['payloadBytes'],
                  c['rootSegments'][-1]['byteEnd'], c['fullSegments'][-1]['byteEnd']]
        segments = '{{' + ','.join('{' + ','.join(map(str, [s['byteStart'], s['bytes'], 4])) + '}' for s in ext) + '}}'
        lines.append('{' + ','.join(map(str, scalar)) + ',' + segments + '},')
    lines = [line.replace('struct Segment {','struct LineageSegment {') for line in lines]
    lines += ['}};', 'inline const LineageProfile* SelectLineageProfile(uint32_t k,uint32_t kw){for(const auto& p:kLineageProfiles)if(p.k==k&&p.kw==kw)return &p;return nullptr;}', '}']
    (DEST / 'lineage_schema.h').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    dictionaries = {'provenance': design['provenance32'], 'input': design['sensorInput64'],
                    'physicalLift': design['physicalLiftEvent64'], 'profiles': cold['profiles']}
    (DEST / 'reader_schema.json').write_text(json.dumps(dictionaries, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    generate()
