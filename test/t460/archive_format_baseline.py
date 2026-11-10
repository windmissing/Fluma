#!/usr/bin/env python3
"""T460 · GPU M2 V3 独立归档格式反例基线（不使用真机，不改产品）。

依据 docs/analysis-T445-gpu-M2-wire-v3.json（T445 机器 manifest，唯一合同来源）独立
合成冷档 fixture 并冻结 schema 判读预期。独立实现 = 本文件从 wire JSON 重新推导全部
预期，不 import 生产 cold_reader.py，不做 T453 生产差分。复用 T446 语义层结论。

覆盖（卡面逐条）：
  A. SOURCE128/QUANTUM64 错 stride（rows×stride×4 与 eventWords 不一致）
  B. rows×stride×4 事件区长度边界
  C. 逐文件哈希/字节数不符
  D. V2 冷档（revision=T445_M2_TRACE_V2 类前代）被 V3 reader 拒
  E. reader 拒未知 revision
  F. scope 错配（page 头字段与 manifest meta 不一致）
  G. arena/generation/rootKind 错配
  H. pub/scene token 不可互替（rootToken 逐字重算，改动任一成分即拒）
  I. SOURCE 行 rowSeal/word 保留位/END 尾行 (18) 判读
  J. QUANTUM 行 kind 必须在 allowedForQuantum 集合
  K. 当前未接入字段登记 NOT_IMPLEMENTED

fixture 合成路径：从 .codex/local-test/t452-host/raw-N-export（T452 真实 GPU 冷档）
取正例，按上述 A–J 各做一类最小扰动生成负例；正例预期 PASS 作对照。
每个负例同时记录：扰动字段、位置、独立预期拒绝码。

用法: python test/t460/archive_format_baseline.py [--json out]
退出码: 0=全部预期一致；1=有意外分歧
"""
import hashlib
import json
import shutil
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
WIRE = json.loads((REPO / 'docs/analysis-T445-gpu-M2-wire-v3.json').read_text(encoding='utf-8'))
OUT = REPO / '.codex/local-test/t460'
REPORTS = REPO / 'test/reports/t460'

SOURCE_MAGIC = WIRE['source128']['magic']          # 860172877
SOURCE_ROW_SEAL = WIRE['source128']['rowSeal']     # 3235774467
QUANTUM_MAGIC = 1163088455                          # 与生产 reader 同值；来源：wire quantum64 行头
SOURCE_KIND_CODES = {v['code']: set(v['usedWords']) for v in WIRE['source128']['kinds'].values()}
QUANTUM_ALLOWED = {v['kind']: set(v['usedWords']) for v in WIRE['quantum64'] if v['allowedForQuantum']}
V3_REVISION = 'T445_M2_TRACE_V3'
END_ROW_WORD = 18                                   # wire: 最后一行 kind==18

# ---------- 独立判读器（不 import 生产 cold_reader.py） ----------
def indep_read(pages_words, meta):
    """pages_words: list[list[int]] 每页 words；meta: manifest header dict。
    返回 (ok, reason)。按 T445 wire 独立推导全部判据。"""
    def req(ok, reason):
        if not ok:
            raise ValueError(reason)
    req(meta.get('revision') == V3_REVISION, 'TRACE_REVISION')
    req(meta.get('state') == 'TRACE_READY', 'TRACE_STATE')
    kind = meta.get('rootKind')
    req(kind in (1, 2), 'TRACE_KIND')
    stride = 128 if kind == 1 else 64
    req(meta.get('stride') == stride, 'TRACE_STRIDE')
    req(meta['rows'] * stride == meta['eventWords'], 'TRACE_EVENT_SIZE')
    count = 256 + meta['stateWords'] + meta['eventWords']
    req(meta['pages'] == (count + 1023) // 1024, 'TRACE_PAGES_CALC')
    req(len(pages_words) == meta['pages'], 'TRACE_PAGE_COUNT')
    words = []
    for i, page in enumerate(pages_words, 1):
        req(len(page) == min(1024, count - len(words)), 'TRACE_PAGE_SIZE')
        req(all(type(w) is int and 0 <= w <= 0xffffffff for w in page), 'TRACE_WORD_TYPE')
        words.extend(page)
    req(len(words) == count, 'TRACE_TOTAL_WORDS')
    c = words[:256]
    state = words[256:256 + meta['stateWords']]
    events = words[256 + meta['stateWords']:]
    req(c[:4] == [1381192263, 3, 256, 3235823838], 'TRACE_COMPLETION')
    req(c[27] == kind, 'TRACE_COMPLETION_KIND')
    req(c[28] in (1, 2, 3), 'TRACE_COMPLETION_OUTCOME')
    req(c[35] < 2, 'TRACE_EPOCH')
    req(c[36:38] == [meta['generationLo'], meta['generationHi']], 'TRACE_GENERATION')
    req(c[172 if kind == 1 else 51] == meta['arena'] and meta['arena'] < 2, 'TRACE_ARENA')
    req(c[169 if kind == 1 else 53] == meta['rows'], 'TRACE_ROW_COUNT')
    if kind == 1:
        req(c[168] == 128, 'TRACE_SOURCE_STRIDE')
    h = state[-256:]
    k, kw = h[41] + h[43], h[42] + h[44]
    req(k in (192, 512) and kw in (64, 128), 'TRACE_PROFILE')
    req(len(state) == 16 * (k + kw) + 2048 + 256, 'TRACE_STATE_LEN')
    req(h[1] == 3 and h[31] == 7, 'TRACE_POLICY_HEADER')
    req(h[132:137] == [1, 122333, 122333, 65536, k], 'TRACE_POLICY_CONSTANTS')
    req(h[132:145] == c[160:173] and not any(h[145:]), 'TRACE_SUMMARY')
    layout_hash = list(struct.unpack('<8I', hashlib.sha256(
        (REPO / 'docs/analysis-T445-gpu-M2-wire-v3.json').read_bytes()).digest()))
    req(c[130:138] == layout_hash, 'TRACE_LAYOUT_HASH')
    token = 'GRS_GPU_ROOT_Q20_M2_V3' + ''.join('|' + str(c[i]) for i in
        list(range(4, 28)) + list(range(32, 42)) + list(range(114, 138)) + list(range(160, 173)))
    req(meta['rootToken'] == token, 'TRACE_ROOT_TOKEN')
    if kind == 1:
        for i in range(meta['rows']):
            row = events[128 * i:128 * (i + 1)]
            req(row[0] == SOURCE_MAGIC and row[1] == 3 and row[3] == 1, 'TRACE_SOURCE_ROW_HEAD')
            req(row[6:8] == [i + 1, 0], 'TRACE_SOURCE_ROW_ORDINAL')
            req(row[31] == SOURCE_ROW_SEAL, 'TRACE_SOURCE_ROW_SEAL')
            req(row[4:6] == c[4:6], 'TRACE_SOURCE_ROW_NONCE')
            req((row[2] == END_ROW_WORD) == (i == meta['rows'] - 1), 'TRACE_SOURCE_END_TAIL')
            req(row[2] in SOURCE_KIND_CODES, 'TRACE_SOURCE_KIND')
            req(all(v == 0 for w, v in enumerate(row) if w not in SOURCE_KIND_CODES[row[2]]),
                'TRACE_SOURCE_RESERVED')
    else:
        for i in range(meta['rows']):
            row = events[64 * i:64 * (i + 1)]
            req(row[0] == QUANTUM_MAGIC and row[1] == 3 and row[4] == 2, 'TRACE_QUANTUM_ROW_HEAD')
            req(row[7:9] == [i + 1, 0], 'TRACE_QUANTUM_ROW_ORDINAL')
            req(row[6] in QUANTUM_ALLOWED, 'TRACE_QUANTUM_KIND')
            req(all(v == 0 for w, v in enumerate(row) if w not in QUANTUM_ALLOWED[row[6]]),
                'TRACE_QUANTUM_RESERVED')
    return {'rootKind': kind, 'rows': meta['rows'], 'stride': stride}

# ---------- fixture 装载 ----------
def load_export(src_dir):
    m = json.loads((src_dir / 'manifest.json').read_text(encoding='utf-8'))
    meta = m['header']
    pages = []
    for i in range(1, meta['pages'] + 1):
        p = json.loads((src_dir / f'page-{i}.json').read_text(encoding='utf-8'))
        pages.append(p['words'])
    return pages, meta

def write_fixture(name, pages, meta, page_headers=None):
    """page_headers: 可选 list[dict]，第 i 项为第 i 页的 scope 头覆盖（None 用 meta）。"""
    d = OUT / name
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True)
    out_meta = dict(meta)
    out_meta['pages'] = len(pages)
    out_files = []
    for i, page in enumerate(pages, 1):
        hdr = dict(meta)
        if page_headers and page_headers[i - 1]:
            hdr.update(page_headers[i - 1])
        page_obj = dict(hdr)
        page_obj['page'] = i
        page_obj['pages'] = len(pages)
        page_obj['words'] = page
        raw = json.dumps(page_obj, ensure_ascii=False).encode('utf-8')
        fname = f'page-{i}.json'
        (d / fname).write_bytes(raw)
        out_files.append({'name': fname, 'bytes': len(raw),
                          'sha256': hashlib.sha256(raw).hexdigest()})
    manifest = {'header': out_meta, 'files': out_files}
    (d / 'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False))
    return d

def run_case(cases, name, src_pages, meta, expect_ok, expect_reason, mutate=None, page_headers=None):
    pages = [list(p) for p in src_pages]
    m = json.loads(json.dumps(meta))
    detail = ''
    if mutate:
        try:
            mutate(pages, m)
        except Exception as e:  # mutate 内直接构造畸形
            detail = str(e)
    try:
        indep_read(pages, m)
        # scope 比对：page 头与 meta 一致性（生产 reader 的 TRACE_SCOPE 独立对应）
        ph = page_headers
        for i in range(len(pages)):
            if ph and ph[i]:
                for k, v in ph[i].items():
                    if m.get(k) != v:
                        raise ValueError('TRACE_SCOPE')
        got_ok, got_reason = True, ''
    except ValueError as e:
        got_ok, got_reason = False, str(e)
    agree = (got_ok == expect_ok) and (expect_ok or got_reason == expect_reason)
    cases.append({'case': name, 'expect': 'PASS' if expect_ok else expect_reason,
                  'got': 'PASS' if got_ok else got_reason, 'agree': agree, 'detail': detail})
    return agree

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    REPORTS.mkdir(parents=True, exist_ok=True)
    # 正例源：T452 真实 GPU 冷档第一档第一个 SOURCE 与一个 QUANTUM
    src_source = OUT / 'pos-source'
    src_quantum = OUT / 'pos-quantum'
    found_s = found_q = False
    for i in range(1, 100):
        d = Path(f'.codex/local-test/t452-host/raw-{i}-export')
        if not d.exists():
            break
        m = json.loads((d / 'manifest.json').read_text(encoding='utf-8'))['header']
        if not found_s and m['rootKind'] == 1:
            shutil.copytree(d, src_source, dirs_exist_ok=True)
            found_s = True
        if not found_q and m['rootKind'] == 2:
            shutil.copytree(d, src_quantum, dirs_exist_ok=True)
            found_q = True
        if found_s and found_q:
            break
    if not found_s or not found_q:
        print('T460 BLOCKED: t452-host raw-*export 冷档缺失（需先跑 test/t452/build_source.cmd）')
        return 2
    s_pages, s_meta = load_export(src_source)
    q_pages, q_meta = load_export(src_quantum)
    write_fixture('pos-source-copy', s_pages, s_meta)
    write_fixture('pos-quantum-copy', q_pages, q_meta)

    cases = []
    ok = True

    def add(name, pages, meta, expect_ok, expect_reason, mutate, page_headers=None):
        nonlocal ok
        if not run_case(cases, name, pages, meta, expect_ok, expect_reason, mutate, page_headers):
            ok = False

    # 0. 正例对照（两份 fixture 重读 PASS）
    add('pos-source', s_pages, s_meta, True, '', None)
    add('pos-quantum', q_pages, q_meta, True, '', None)

    # A. 错 stride
    add('neg-source-stride', s_pages, s_meta, False, 'TRACE_STRIDE',
        lambda ps, m: m.__setitem__('stride', 64))
    add('neg-quantum-stride', q_pages, q_meta, False, 'TRACE_STRIDE',
        lambda ps, m: m.__setitem__('stride', 128))
    # A2. stride 字段正确但 eventWords 与 rows×stride 不一致
    add('neg-source-eventsize', s_pages, s_meta, False, 'TRACE_EVENT_SIZE',
        lambda ps, m: m.__setitem__('eventWords', m['eventWords'] + 128))

    # B. rows×stride×4 边界：rows 加一但事件区不变
    add('neg-source-rows-inflate', s_pages, s_meta, False, 'TRACE_EVENT_SIZE',
        lambda ps, m: m.__setitem__('rows', m['rows'] + 1))

    # C. 逐文件哈希/字节不符（manifest 收据层，独立核对函数模拟）
    def hash_mutate(ps, m):
        pass
    # 哈希层在本独立判读器外（fixture 写盘层），单独用文件层验证：
    fx = OUT / 'neg-file-hash'
    if fx.exists():
        shutil.rmtree(fx)
    shutil.copytree(OUT / 'pos-source-copy', fx)
    m = json.loads((fx / 'manifest.json').read_text(encoding='utf-8'))
    raw = (fx / 'page-2.json').read_bytes()
    tampered = raw.replace(b'"page": 2', b'"page": 2 ', 1)  # 字节变、内容 JSON 等价
    (fx / 'page-2.json').write_bytes(tampered)
    file_layer_ok = False
    for f in m['files']:
        if f['name'] == 'page-2.json':
            got = (len(tampered) == f['bytes'] and
                   hashlib.sha256(tampered).hexdigest() == f['sha256'])
            file_layer_ok = not got  # 期望：字节或哈希至少一项不符 → 拒
    cases.append({'case': 'neg-file-hash', 'expect': 'REJECT(bytes/sha mismatch)',
                  'got': 'REJECT' if file_layer_ok else 'ACCEPT', 'agree': file_layer_ok})
    ok = ok and file_layer_ok

    # D/E. 未知与前代 revision 拒绝
    add('neg-revision-v2', s_pages, s_meta, False, 'TRACE_REVISION',
        lambda ps, m: m.__setitem__('revision', 'T445_M2_TRACE_V2'))
    add('neg-revision-unknown', s_pages, s_meta, False, 'TRACE_REVISION',
        lambda ps, m: m.__setitem__('revision', 'T445_M2_TRACE_V9'))

    # F. scope 错配：manifest 头 epoch 与全部 page 头不一致（生产 TRACE_SCOPE 对应）
    # epoch 本身在独立读器中经 c[35]<2/页面头一致性判；此处 page 头保持旧值、meta 漂移
    ph = [ {'epoch': s_meta['epoch']} for _ in s_pages ]
    add('neg-scope-epoch', s_pages, dict(s_meta, epoch=str(int(s_meta['epoch']) + 1)),
        False, 'TRACE_SCOPE', None, page_headers=ph)

    # G. arena / generation / rootKind 错配
    add('neg-arena-oob', s_pages, s_meta, False, 'TRACE_ARENA',
        lambda ps, m: m.__setitem__('arena', 5))
    add('neg-generation-mismatch', s_pages, s_meta, False, 'TRACE_GENERATION',
        lambda ps, m: m.__setitem__('generationLo', m['generationLo'] + 1))
    add('neg-rootkind-flip', s_pages, s_meta, False, 'TRACE_STRIDE',
        lambda ps, m: m.__setitem__('rootKind', 2))

    # H. rootToken 篡改任一成分
    add('neg-roottoken', s_pages, s_meta, False, 'TRACE_ROOT_TOKEN',
        lambda ps, m: m.__setitem__('rootToken', m['rootToken'].replace('M2_V3', 'M2_V4')))
    # H2. pub/scene token 不可互替：把 scene token 形态塞进 rootToken 位
    add('neg-token-scene-shape', s_pages, s_meta, False, 'TRACE_ROOT_TOKEN',
        lambda ps, m: m.__setitem__('rootToken', 'GRS_GPU_SCENE_Q20_M1_V2|1|0'))

    # I. SOURCE 行：rowSeal 篡改 / 保留位非零 / END 尾行位提前
    def seal_mutate(ps, m):
        # completion/state 之后第一页含事件首行；事件起点 = 256+stateWords
        event_start = 256 + m['stateWords']
        page_idx = event_start // 1024
        off = event_start % 1024
        ps[page_idx][off + 31] ^= 1  # rowSeal 翻位
    add('neg-source-rowseal', s_pages, s_meta, False, 'TRACE_SOURCE_ROW_SEAL', seal_mutate)

    def reserved_mutate(ps, m):
        event_start = 256 + m['stateWords']
        page_idx = event_start // 1024
        off = event_start % 1024
        row0 = ps[page_idx][off:off + 128]
        kind = row0[2]
        used = SOURCE_KIND_CODES[kind]
        # 找第一个未用 word 写毒
        for w in range(120, 128):
            if w not in used:
                ps[page_idx][off + w] = 0xdeadbeef
                break
    add('neg-source-reserved', s_pages, s_meta, False, 'TRACE_SOURCE_RESERVED', reserved_mutate)

    def end_early_mutate(ps, m):
        # 把首行 kind 改成 END(18)：非尾行出现 END → 拒
        event_start = 256 + m['stateWords']
        page_idx = event_start // 1024
        off = event_start % 1024
        ps[page_idx][off + 2] = END_ROW_WORD
    add('neg-source-end-early', s_pages, s_meta, False, 'TRACE_SOURCE_END_TAIL', end_early_mutate)

    # J. QUANTUM 行 kind 不在 allowedForQuantum（SOURCE_BIRTH kind=1 禁入 QUANTUM）
    def q_kind_mutate(ps, m):
        event_start = 256 + m['stateWords']
        page_idx = event_start // 1024
        off = event_start % 1024
        ps[page_idx][off + 6] = 1  # SOURCE_BIRTH 禁止出现在 QUANTUM
    add('neg-quantum-kind', q_pages, q_meta, False, 'TRACE_QUANTUM_KIND', q_kind_mutate)

    def q_reserved_mutate(ps, m):
        event_start = 256 + m['stateWords']
        page_idx = event_start // 1024
        off = event_start % 1024
        kind = ps[page_idx][off + 6]
        used = QUANTUM_ALLOWED[kind]
        for w in range(60, 64):
            if w not in used:
                ps[page_idx][off + w] = 7
                break
    add('neg-quantum-reserved', q_pages, q_meta, False, 'TRACE_QUANTUM_RESERVED', q_reserved_mutate)

    # K. 未接入字段（本卡不覆盖生产路径，登记 NOT_IMPLEMENTED）
    not_impl = [
        '真机冷档（GPU 页写出 bytes/SHA 收据链路）由 T461 真机档覆盖',
        'T453 生产差分中的 reader 与产品 GL 链路一致性',
        'pub/scene token 真实互替攻击（需产品 scene token 通道，当前离线无该通道）',
    ]

    passed = sum(1 for c in cases if c['agree'])
    report = {'schema': 'T460_M2_V3_ARCHIVE_FORMAT_BASELINE_V1', 'verdict': 'PASS' if ok else 'FAIL',
              'passed': passed, 'total': len(cases), 'cases': cases, 'notImplemented': not_impl,
              'wireContract': 'docs/analysis-T445-gpu-M2-wire-v3.json',
              'wireSha256': hashlib.sha256((REPO / 'docs/analysis-T445-gpu-M2-wire-v3.json').read_bytes()).hexdigest(),
              'independentReader': 'test/t460/archive_format_baseline.py::indep_read (不 import 生产 cold_reader.py)',
              'device': 'NOT_RUN'}
    (REPORTS / 't460-baseline.json').write_text(json.dumps(report, ensure_ascii=False, indent=1),
                                                encoding='utf-8')
    print(json.dumps({'revision': report['schema'], 'verdict': report['verdict'],
                      'passed': passed, 'entries': len(cases), 'notImplemented': len(not_impl)},
                     ensure_ascii=False))
    return 0 if ok else 1

if __name__ == '__main__':
    sys.exit(main())
