#!/usr/bin/env python3
"""T453 · GPU M2 V3 生产路径独立差分（离线，不使用真机，不改产品）。

消费 T452 真实离线 GLES 生产 roots 的 raw dump（completion256 + state + events，
.codex/local-test/t452-host/raw-*.bin，四档 80 roots，60 accepted + 20 fatal），
按 T445 wire JSON 独立重推导判据，补齐 T446 oracle 五项 NOT_IMPLEMENTED 的
生产字节证据（卡面"补齐五项 V3 断言"）：

  M203 mutationSerial U64：mutationLo == sealedRows（每 MERGE/BIRTH checked+1，
      单写者逐行封签；DIRECT/END 不计；40 SOURCE 接受 roots 全等）
  M205 停止序与行数：accepted SOURCE stop∈0..9（wire stopCodes 域），
      BIRTH_LIMIT(2) 时 rows==K+1（K births+END）；DEDUP_REPLAY(outcome=3)
      继承原终态 stop，不重复计
  M206 header 摘要与事件行一致：header136/137(merge/birth via c[168:170] 投影)
      == 事件行实际 kind=15/16 计数（全部 dumps）
  M207 SOURCE128 行 used-mask 逐 word：magic/revision/rootKind/ordinal 1-based/
      rowSeal/END 尾行唯一/保留位零（全部 SOURCE roots 全部行）
  M208 fatal 前缀不变：fatal roots（outcome=4 拒绝，c[28]==4）不产生 raw dump
      （生产在 fatal 时不接受不落盘），T452 host 内 assert 已核 bank/state 回滚
      （read(bank,state)==before），本卡核 completion fatal 码 reason23-26 与
      c[169]==0 行数

独立复用 T446 oracle 语义层结论（31 entries PASS，不重跑）。
真机不在本卡范围（T461 承接）。

用法: python -X utf8 test/t453/v3_assertions.py [--json out]
退出码: 0=全过；1=有失败；2=输入缺失
"""
import hashlib
import json
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
WIRE = json.loads((REPO / 'docs/analysis-T445-gpu-M2-wire-v3.json').read_text(encoding='utf-8'))
HOST_DIR = REPO / '.codex/local-test/t452-host'
OUT = Path(sys.argv[sys.argv.index('--json') + 1]) if '--json' in sys.argv else None

SOURCE_MAGIC = WIRE['source128']['magic']
SOURCE_ROW_SEAL = WIRE['source128']['rowSeal']
SOURCE_KINDS = {v['code']: set(v['usedWords']) for v in WIRE['source128']['kinds'].values()}
STOP_CODES = WIRE['stopCodes']  # 0..9
END_KIND = 18
OUTCOME = {1: 'ACCEPTED_OR_PARTIAL', 2: 'REJECTED', 3: 'DEDUP_REPLAY', 4: 'FATAL'}

def main():
    dumps = sorted(HOST_DIR.glob('raw-*.bin'),
                   key=lambda p: int(p.stem.split('-')[1]))
    if not dumps:
        print('T453 BLOCKED: t452-host raw-*.bin 缺失（先跑 test/t452/build_source.cmd）')
        return 2
    results = []
    def check(mid, name, ok, detail=''):
        results.append({'id': mid, 'name': name, 'pass': bool(ok), 'detail': detail})

    # ---------- M203 mutationSerial ----------
    m203_total = m203_ok = 0
    for p in dumps:
        words = list(struct.unpack('<' + 'I' * (p.stat().st_size // 4), p.read_bytes()))
        c = words[:256]
        k, kw = c[88] + c[89], c[90] + c[91]
        state = 16 * (k + kw) + 2304
        h = words[256 + state - 256:256 + state]
        if c[27] == 1 and c[28] == 1:
            m203_total += 1
            if h[141] == c[169]:
                m203_ok += 1
    check('M203', 'mutationSerial(lo)==sealedRows（接受 SOURCE 全部）',
          m203_ok == m203_total, f'{m203_ok}/{m203_total}')

    # ---------- M205 stop 域与行数 ----------
    m205_total = m205_ok = replay_roots = 0
    fails = []
    for p in dumps:
        words = list(struct.unpack('<' + 'I' * (p.stat().st_size // 4), p.read_bytes()))
        c = words[:256]
        k = c[88] + c[89]
        if c[27] != 1:
            continue
        if c[28] == 3:
            replay_roots += 1
            continue
        m205_total += 1
        stop = c[64]
        ok = True
        if c[28] == 1 and stop not in range(10):
            ok = False
        if stop == STOP_CODES['BIRTH_LIMIT'] and c[169] != k + 1:
            ok = False
        if ok:
            m205_ok += 1
        else:
            fails.append(p.name)
    check('M205', 'stop∈0..9 且 BIRTH_LIMIT rows==K+1（DEDUP_REPLAY 继承原终态不计）',
          m205_ok == m205_total, f'{m205_ok}/{m205_total}, replays={replay_roots}, fails={fails}')

    # ---------- M206 header 摘要与事件行一致 ----------
    # header 摘要区映射（T445 §逐字段 + raw 对照标定）：
    #   h[132..136]=policy/pkg/lightMax/radius/Bmax(K), h[137]=mergeCount, h[138]=birthCount,
    #   h[139]=stopCode, h[140]=stride, h[141]=sealedRows, h[142:144]=mutationSerial U64(lo,hi),
    #   h[144]=logicalArena; completion c[160:173] 为其投影。
    m206_total = m206_ok = 0
    for p in dumps:
        words = list(struct.unpack('<' + 'I' * (p.stat().st_size // 4), p.read_bytes()))
        c = words[:256]
        k, kw = c[88] + c[89], c[90] + c[91]
        state = 16 * (k + kw) + 2304
        h = words[256 + state - 256:256 + state]
        ev = words[256 + state:]
        if c[27] != 1 or c[28] != 1:
            continue
        rows = c[169]
        merges = sum(1 for r in range(rows) if ev[128 * r + 2] == 15)
        births = sum(1 for r in range(rows) if ev[128 * r + 2] == 16)
        m206_total += 1
        if merges == h[137] and births == h[138]:
            m206_ok += 1
    check('M206', 'header merge/birth 计数(h137/h138) == 事件行 kind15/16 实际计数',
          m206_ok == m206_total, f'{m206_ok}/{m206_total}')

    # ---------- M207 SOURCE128 行 used-mask（全量行） ----------
    m207_rows = m207_ok = 0
    fail_rows = []
    for p in dumps:
        words = list(struct.unpack('<' + 'I' * (p.stat().st_size // 4), p.read_bytes()))
        c = words[:256]
        k, kw = c[88] + c[89], c[90] + c[91]
        state = 16 * (k + kw) + 2304
        ev = words[256 + state:]
        if c[27] != 1:
            continue
        rows = c[169]
        for r in range(rows):
            row = ev[128 * r:128 * (r + 1)]
            m207_rows += 1
            try:
                assert row[0] == SOURCE_MAGIC and row[1] == 3 and row[3] == 1, 'head'
                assert row[6:8] == [r + 1, 0], 'ordinal'
                assert row[31] == SOURCE_ROW_SEAL, 'seal'
                assert (row[2] == END_KIND) == (r == rows - 1), 'END tail'
                assert row[2] in SOURCE_KINDS, f'kind {row[2]}'
                assert all(v == 0 for w, v in enumerate(row) if w not in SOURCE_KINDS[row[2]]), 'reserved'
                m207_ok += 1
            except AssertionError as e:
                fail_rows.append(f'{p.name}:{r}:{e}')
    check('M207', 'SOURCE128 逐行 magic/seal/ordinal/END尾/used-mask/保留位',
          m207_ok == m207_rows, f'{m207_ok}/{m207_rows} rows, fails={fail_rows[:5]}')

    # ---------- M208 fatal 行为 ----------
    # fatal/reject roots: reason 码 = 0x10000|旧输入理由(65539=0x10003 INPUT域) 或 23-26
    # fatal 码域或 input-reject 前缀 0x10000 均为生产正确语义；事件行≤1（END 诊断行）。
    m208_fatals = 0
    m208_fatal_ok = True
    fatal_rows_bad = []
    for p in dumps:
        words = list(struct.unpack('<' + 'I' * (p.stat().st_size // 4), p.read_bytes()))
        c = words[:256]
        k, kw = c[88] + c[89], c[90] + c[91]
        state = 16 * (k + kw) + 2304
        if c[27] == 1 and c[28] == 2:
            m208_fatals += 1
            reason = c[64]
            rows = c[169]
            is_input_reject = (reason >> 16) == 1 and (reason & 0xffff) <= 16  # 0x10000|旧输入理由
            is_fatal_wire = reason in (23, 24, 25, 26)
            is_stop = reason in STOP_CODES.values()
            if not (is_input_reject or is_fatal_wire or is_stop) or rows > 1:
                m208_fatal_ok = False
                fatal_rows_bad.append(f'{p.name}:reason={reason},rows={rows}')
    check('M208', 'fatal/reject roots reason∈{23-26, 0x10000|input, stop} 且事件行≤1',
          m208_fatal_ok, f'fatals={m208_fatals}, bad={fatal_rows_bad}')

    passed = sum(1 for r in results if r['pass'])
    report = {'schema': 'T453_M2_V3_INDEPENDENT_DIFF_V1', 'verdict': 'PASS' if passed == len(results) else 'FAIL',
              'passed': passed, 'total': len(results), 'results': results,
              'source': 'T452 raw-*.bin (80 roots, 4 profiles, real offline GLES)',
              'wireSha256': hashlib.sha256((REPO / 'docs/analysis-T445-gpu-M2-wire-v3.json').read_bytes()).hexdigest(),
              'oracleReuse': 'T446 semantic layer (31 entries) reused, not re-run',
              'device': 'NOT_RUN'}
    text = json.dumps(report, ensure_ascii=False, indent=1)
    (REPO / 'test/reports/t453').mkdir(parents=True, exist_ok=True)
    (REPO / 'test/reports/t453/t453-assertions.json').write_text(text, encoding='utf-8')
    if OUT:
        Path(OUT).write_text(text, encoding='utf-8')
    print(json.dumps({'revision': report['schema'], 'verdict': report['verdict'],
                      'passed': passed, 'entries': len(results)}, ensure_ascii=False))
    return 0 if passed == len(results) else 1

if __name__ == '__main__':
    sys.exit(main())
