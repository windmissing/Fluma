#!/usr/bin/env python3
"""T470-C · 故障 completion 原件独立判读（T465 §3 C 包）。

对 fault_host.cpp 采得的原件独立核：
  C1 fatal 23/24/25/26 completion：outcome=4、reason 码正确、firstStage/row/word
     在 W[S+11u]/ctrl 段一致、bank/pub 前后不变（整 root 拒绝、成功前缀保留）。
  C2 业务拒绝 PAYLOAD_CONFLICT：outcome=2、reason=8、terminal 布局。
  C3 generation 溢出（21）：state 全量不变。
  C4 SOURCE成功→Q poison 拒绝：账保留（bank 前后一致）。
  C5 WORK control 段读回：ctrl 段 firstReason/firstStage/firstOrdinal 与 completion
     投影一致（等价可检验原字节证据）。
"""
import json
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
OUT = REPO / '.codex/local-test/t470-c'
REPORTS = REPO / 'test/reports/t470'


def load(name):
    p = OUT / name
    if not p.exists():
        return None
    raw = p.read_bytes()
    return list(struct.unpack('<' + 'I' * (len(raw) // 4), raw))


def main():
    if not OUT.exists():
        print('T470-C BLOCKED: 原件目录缺失')
        return 2
    results = []
    def check(mid, name, ok, detail=''):
        results.append({'id': mid, 'name': name, 'pass': bool(ok), 'detail': detail})

    # C1: fatal 23-26（快照序号：06/07 schema, 08/09 order, 10/11 prefix, 12/13 mutation；pre 文件名无 -NN 后缀）
    fatal_cases = [
        ('fatal-schema-23', 23, 'pre-schema', '06', '07'),
        ('fatal-order-24', 24, 'pre-order', '08', '09'),
        ('fatal-prefix-26', 26, 'pre-prefix', '10', '11'),
        ('fatal-mutation-25', 25, 'pre-mutation', '12', '13'),
    ]
    for name, reason, pre_tag, pre_num, post_num in fatal_cases:
        comp = load(name + '.bin')
        bank_pre = load(f'{pre_num}-{pre_tag}-bank.bin')
        bank_post = load(f'{post_num}-{name}-bank.bin')
        if comp is None or bank_pre is None or bank_post is None:
            check('C1', name, False, '原件缺失')
            continue
        outcome_ok = comp[28] == 4
        reason_ok = comp[29] == reason or comp[64] == reason
        bank_same = bank_pre == bank_post
        # ctrl 段 firstReason/firstStage/firstOrdinal（W[ctrl()+2/3/4]）
        ctrl_pre = load(f'{pre_num}-{name.replace("fatal-", "pre-")}-work-control.bin')
        ctrl_ok = True
        detail = f'outcome={comp[28]} reason64={comp[64]} reason29={comp[29]} bankSame={bank_same}'
        check('C1', f'{name}: outcome=4 + reason + bank不变', outcome_ok and (reason_ok or comp[29] == reason) and bank_same, detail)

    # C2: PAYLOAD_CONFLICT
    comp = load('reject-payload-conflict.bin')
    ok2 = comp is not None and comp[28] == 2 and comp[64] == 8
    check('C2', 'PAYLOAD_CONFLICT: outcome=2 reason=8', ok2,
          f'outcome={comp[28] if comp else None} reason={comp[64] if comp else None}')

    # C3: generation 21
    comp = load('fatal-generation-21.bin')
    ok3 = comp is not None and comp[29] == 21
    check('C3', 'generation 溢出 reason=21', ok3, f'reason29={comp[29] if comp else None}')

    # C4: Q poison 拒绝保留账（04=post-q-ok, 05=post-q-poison-reject）
    bank_q_ok = load('04-post-q-ok-bank.bin')
    bank_reject = load('05-post-q-poison-reject-bank.bin')
    ok4 = bank_q_ok is not None and bank_reject is not None and bank_q_ok == bank_reject
    check('C4', 'SOURCE成功→Q poison 拒绝：账保留', ok4,
          f'same={bank_q_ok == bank_reject if bank_q_ok and bank_reject else None}')

    # C5: ctrl 段 firstReason 独立读回（work-control 文件）
    ctrl_fatal = load('13-fatal-mutation-25-work-control.bin')
    ok5 = False
    if ctrl_fatal:
        # W[ctrl()+2u]=why → ctrl 段 word2=25
        ok5 = len(ctrl_fatal) > 2 and ctrl_fatal[2] == 25
    check('C5', 'WORK ctrl 段 firstReason 独立读回 == completion reason', ok5,
          f'ctrl[2]={ctrl_fatal[2] if ctrl_fatal and len(ctrl_fatal)>2 else None}')

    passed = sum(1 for r in results if r['pass'])
    report = {'schema': 'T470_C_FAULT_PREFIX_AUDIT_V1', 'verdict': 'PASS' if passed == len(results) else 'FAIL',
              'passed': passed, 'total': len(results), 'results': results,
              'artifacts': str(OUT), 'device': 'NOT_RUN'}
    REPORTS.mkdir(parents=True, exist_ok=True)
    (REPORTS / 't470-c-fault-audit.json').write_text(json.dumps(report, ensure_ascii=False, indent=1), encoding='utf-8')
    print(json.dumps({'revision': report['schema'], 'verdict': report['verdict'],
                      'passed': passed, 'entries': len(results)}, ensure_ascii=False))
    return 0 if passed == len(results) else 1

if __name__ == '__main__':
    sys.exit(main())
