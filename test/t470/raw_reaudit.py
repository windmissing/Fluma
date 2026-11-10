#!/usr/bin/env python3
"""T470-A · M2 V3 既存 raw 再判读（T465 §3 A 包，不使用真机，不改产品）。

对 T452 host 现存 60 个 raw（独立冷档 dump = completion256 + state + events）
按 T465 §1 裁定执行修正后的独立判据；原 T453 断言映射错误的字段此处改读真正值。

独立判据（本文件自足实现，不 import 生产代码/开发 assert）：
  A1 数量/outcome 修正：60 raw = SOURCE接受32 + SOURCE业务拒绝16 + SOURCE重放8 +
     QUANTUM接受4（T465 §1 表）；无 fatal 23–26 原档，如实记缺。
  A2 mutationSerial U64（h[142]|h[143]<<32，非 h141 行数投影）：
     run 内逐接受 root 的 serial delta == 本 root MERGE+BIRTH 事件行计数；
     run 边界（serial 下降 / fixture 重建 BASE=0）以 serial==m+b 识别；
     拒绝/重放 root serial 不变。
  A3 nextId 链（行字段28/29）：非 END 行 nextIdBefore(r+1)==nextIdAfter(r) 全链连续；
     END 行（kind18）不携带 id 链，跳过（24 处断链全部为 END 行，wire 语义一致）。
  A4 header/completion 投影 c[160:173]==h[132:145] 全 60 raw。
  A5 缺口登记：pre-state/命令原字节不在 raw 内（fixture 输入未落盘），
     per-origin liveMass 聚合（originU64 段在 WORK 区，raw 未 dump）——精确标缺，
     不从输出反推 oracle。

用法: python -X utf8 test/t470/raw_reaudit.py
退出码: 0=全过；1=有失败；2=输入缺失
"""
import hashlib
import json
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
HOST = REPO / '.codex/local-test/t452-host'
REPORTS = REPO / 'test/reports/t470'


def main():
    dumps = sorted(HOST.glob('raw-*.bin'), key=lambda p: int(p.stem.split('-')[1]))
    if not dumps:
        print('T470-A BLOCKED: raw-*.bin 缺失')
        return 2
    raws = {}
    for p in dumps:
        words = list(struct.unpack('<' + 'I' * (p.stat().st_size // 4), p.read_bytes()))
        c = words[:256]
        k, kw = c[88] + c[89], c[90] + c[91]
        state = 16 * (k + kw) + 2304
        raws[int(p.stem.split('-')[1])] = {
            'c': c, 'h': words[256 + state - 256:256 + state],
            'ev': words[256 + state:], 'k': k, 'kw': kw,
            'kind': c[27], 'outcome': c[28],
        }
    results = []
    def check(mid, name, ok, detail=''):
        results.append({'id': mid, 'name': name, 'pass': bool(ok), 'detail': detail})

    # A1 数量/outcome
    from collections import Counter
    dist = Counter((r['kind'], r['outcome']) for r in raws.values())
    a1 = (dist[(1, 1)] == 32 and dist[(1, 3)] == 8 and dist[(1, 2)] == 16 and dist[(2, 1)] == 4
          and len(raws) == 60)
    check('A1', '60 raw = 接受32+拒绝16+重放8+QUANTUM4（T465 §1 表）', a1,
          'dist=' + str({('k%d o%d' % t): n for t, n in dist.items()}))

    # A2 mutationSerial U64 delta
    ok2 = True; fails2 = []; boundary = []
    prev_serial = None
    for i in sorted(raws):
        r = raws[i]
        mut = r['h'][142] | (r['h'][143] << 32)
        if r['kind'] == 1 and r['outcome'] == 1:
            rows_n = r['c'][169]
            m = sum(1 for rr in range(rows_n) if r['ev'][128 * rr + 2] == 15)
            b = sum(1 for rr in range(rows_n) if r['ev'][128 * rr + 2] == 16)
            delta = mut if prev_serial is None or mut < prev_serial else mut - prev_serial
            if delta != m + b:
                if mut == m + b:
                    boundary.append(i)
                else:
                    ok2 = False; fails2.append(f'raw-{i}: delta={delta}!=m+b={m+b}')
            prev_serial = mut
        elif r['kind'] == 1 and r['outcome'] in (2, 3):
            if prev_serial is not None and mut != prev_serial:
                ok2 = False; fails2.append(f'raw-{i}: reject/replay serial moved')
            prev_serial = mut
    check('A2', 'mutationSerial U64 逐root delta==MERGE+BIRTH（fixture边界 BASE=0 单列）', ok2,
          f'fails={fails2 if fails2 else 0}; fixture边界={boundary}（serial==m+b，BASE=0 起步）')

    # A3 nextId 链（非 END 行）
    ok3 = True; fails3 = []; checked3 = 0
    for i in sorted(raws):
        r = raws[i]
        if r['kind'] != 1 or r['outcome'] != 1:
            continue
        rows_n = r['c'][169]
        prev_next = None
        for rr in range(rows_n):
            row = r['ev'][128 * rr:128 * (rr + 1)]
            if row[2] == 18:
                continue
            nb, na = row[28], row[29]
            checked3 += 1
            if prev_next is not None and nb != prev_next:
                ok3 = False; fails3.append(f'raw-{i} row{rr}')
            prev_next = na
    check('A3', 'nextId 链非 END 行全连续（END 行 wire 语义无链）', ok3,
          f'checked={checked3}, fails={fails3 if fails3 else 0}')

    # A4 投影
    a4 = all(r['c'][160:173] == r['h'][132:145] for r in raws.values())
    check('A4', 'completion[160:173] 投影 == header[132:145] 全 60 raw', a4)

    # A5 缺口登记（NOT_IMPLEMENTED 性质：设施缺失，非产品缺陷）
    not_impl = [
        'per-origin liveMass 聚合审计：originU64 段在 WORK control 区（[334752,342944)），raw 未 dump 该区——需 B 包执行时读回',
        'pre-state/command 原字节：fixture 输入序列未随 raw 落盘——独立期望需 B 包保存输入后再算',
        'fatal 23–26 原档：60 raw 无 fatal（fatal 不写 raw），归 C 包故障 hook 补采',
    ]

    passed = sum(1 for r in results if r['pass'])
    report = {'schema': 'T470_A_RAW_REAUDIT_V1', 'verdict': 'PASS' if passed == len(results) else 'FAIL',
              'passed': passed, 'total': len(results), 'results': results,
              'rawCount': len(raws), 'rawDir': str(HOST),
              'wireSha256': hashlib.sha256((REPO / 'docs/analysis-T445-gpu-M2-wire-v3.json').read_bytes()).hexdigest(),
              'notImplemented': not_impl, 'device': 'NOT_RUN'}
    REPORTS.mkdir(parents=True, exist_ok=True)
    (REPORTS / 't470-a-raw-reaudit.json').write_text(json.dumps(report, ensure_ascii=False, indent=1), encoding='utf-8')
    print(json.dumps({'revision': report['schema'], 'verdict': report['verdict'],
                      'passed': passed, 'entries': len(results), 'raw': len(raws)}, ensure_ascii=False))
    return 0 if passed == len(results) else 1

if __name__ == '__main__':
    sys.exit(main())
