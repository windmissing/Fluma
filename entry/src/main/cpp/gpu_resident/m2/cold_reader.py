"""Read a complete V3 cold export. Historical V2 exports need their V2 reader."""
from pathlib import Path
import argparse
import hashlib
import json
import struct

SPEC = Path(__file__).resolve().parents[6] / 'docs/analysis-T445-gpu-M2-wire-v3.json'


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def read_export(manifest):
    spec = json.loads(SPEC.read_text(encoding='utf-8'))
    report = json.loads(manifest.read_text(encoding='utf-8'))
    meta = report['header']
    require(meta['revision'] == 'T445_M2_TRACE_V3' and meta['state'] == 'TRACE_READY', 'TRACE_REVISION')
    kind = meta['rootKind']
    require(kind in (1, 2) and meta['stride'] == (128 if kind == 1 else 64), 'TRACE_STRIDE')
    require(meta['rows'] * meta['stride'] == meta['eventWords'], 'TRACE_EVENT_SIZE')
    count = 256 + meta['stateWords'] + meta['eventWords']
    require(meta['pages'] == (count + 1023) // 1024 and len(report['files']) == meta['pages'], 'TRACE_PAGES')
    words = []
    for i, receipt in enumerate(report['files'], 1):
        name = receipt['name']
        require(Path(name).name == name and ':' not in name and '\\' not in name, 'TRACE_FILENAME')
        raw = (manifest.parent / name).read_bytes()
        require(len(raw) == receipt['bytes'] and hashlib.sha256(raw).hexdigest() == receipt['sha256'], 'TRACE_FILE_HASH')
        page = json.loads(raw)
        require(all(page[k] == v for k, v in meta.items() if k not in ('page', 'words')), 'TRACE_SCOPE')
        require(page['page'] == i and len(page['words']) == min(1024, count - len(words)), 'TRACE_PAGE_SIZE')
        require(all(type(w) is int and 0 <= w <= 0xffffffff for w in page['words']), 'TRACE_WORD')
        words.extend(page['words'])
    c = words[:256]
    state = words[256:256 + meta['stateWords']]
    events = words[256 + meta['stateWords']:]
    require(c[:4] == [1381192263, 3, 256, 3235823838] and c[27] == kind and c[28] in (1, 2, 3), 'TRACE_COMPLETION')
    require(c[35] < 2 and c[36:38] == [meta['generationLo'], meta['generationHi']], 'TRACE_GENERATION')
    require(c[172 if kind == 1 else 51] == meta['arena'] and meta['arena'] < 2, 'TRACE_ARENA')
    require(c[169 if kind == 1 else 53] == meta['rows'], 'TRACE_ROW_COUNT')
    require(kind != 1 or c[168] == 128, 'TRACE_SOURCE_STRIDE')
    h = state[-256:]
    k, kw = h[41] + h[43], h[42] + h[44]
    require(k in (192, 512) and kw in (64, 128) and len(state) == 16 * (k + kw) + 2048 + 256, 'TRACE_PROFILE')
    require(h[1] == 3 and h[31] == 7 and h[132:137] == [1, 122333, 122333, 65536, k], 'TRACE_POLICY')
    require(h[132:145] == c[160:173] and not any(h[145:]), 'TRACE_SUMMARY')
    require(c[130:138] == list(struct.unpack('<8I', hashlib.sha256(SPEC.read_bytes()).digest())), 'TRACE_LAYOUT_HASH')
    token = 'GRS_GPU_ROOT_Q20_M2_V3' + ''.join('|' + str(c[i]) for i in list(range(4, 28)) + list(range(32, 42)) + list(range(114, 138)) + list(range(160, 173)))
    require(meta['rootToken'] == token, 'TRACE_ROOT_TOKEN')
    if kind == 1:
        kinds = {v['code']: set(v['usedWords']) for v in spec['source128']['kinds'].values()}
        for i in range(meta['rows']):
            row = events[128*i:128*(i+1)]
            require(row[0] == spec['source128']['magic'] and row[1] == 3 and row[3] == 1 and row[6:8] == [i+1, 0] and row[31] == spec['source128']['rowSeal'], 'TRACE_SOURCE_ROW')
            require(row[4:6] == c[4:6] and (row[2] == 18) == (i == meta['rows']-1), 'TRACE_SOURCE_ORDER')
            require(row[2] in kinds and all(v == 0 for w, v in enumerate(row) if w not in kinds[row[2]]), 'TRACE_SOURCE_RESERVED')
    else:
        kinds = {v['kind']: set(v['usedWords']) for v in spec['quantum64'] if v['allowedForQuantum']}
        for i in range(meta['rows']):
            row = events[64*i:64*(i+1)]
            require(row[0] == 1163088455 and row[1] == 3 and row[4] == 2 and row[7:9] == [i+1, 0], 'TRACE_QUANTUM_ROW')
            require(row[6] in kinds and all(v == 0 for w, v in enumerate(row) if w not in kinds[row[6]]), 'TRACE_QUANTUM_RESERVED')
    return {'revision': meta['revision'], 'rootKind': kind, 'K': k, 'Kw': kw, 'rows': meta['rows'],
            'stride': meta['stride'], 'words': count, 'rootToken': token, 'deviceVerdict': 'NOT_EVALUATED'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('manifest', type=Path)
    args = parser.parse_args()
    print(json.dumps(read_export(args.manifest), ensure_ascii=False))
