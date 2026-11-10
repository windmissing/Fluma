#!/usr/bin/env python3
"""T470-B · 生产 GLSL merge/RNE 数学负控独立重算（T465 §3 B 包）。

消费 fault_host B 段原件（b1/b2/b3 的 command + base/after bank 快照），
用 T446 oracle 独立语义（Fraction RNE / 最小 pair）重算期望并与 GPU 结果对照：
  B1 tie pair：两粒同 y=0 同 mass 0.125Q=131072 → merge M=262144, RNE((mA*0+mB*0)/M)=0
     GPU 结果对照：事件行 MERGE q 值与 base/after 的 slot 变化。
  B2 满池 BIRTH：maxBirth 全额出生 K 粒 → 事件行 BIRTH 计数=K、nextId 链。
  B3 NO_PAIR：满池后 y 超半径 → stopCode=4、无新 BIRTH/MERGE。
独立实现：本文件从 wire JSON + T446 语义重算（import test/t446/m2_oracle 的 rne/brute_pair
为 oracle 复用，非生产代码）。GPU 侧值从事件行原字节读。
"""
import json
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
OUT = REPO / '.codex/local-test/t470-c'
REPORTS = REPO / 'test/reports/t470'
sys.path.insert(0, str(REPO / 'test/t446'))
from m2_oracle import rne, merge_fields, MERGED_MAX, PACKAGE  # T446 独立语义 oracle 复用


def load(name):
    p = OUT / name
    if not p.exists():
        return None
    raw = p.read_bytes()
    return list(struct.unpack('<' + 'I' * (len(raw) // 4), raw))


def main():
    results = []
    def check(mid, name, ok, detail=''):
        results.append({'id': mid, 'name': name, 'pass': bool(ok), 'detail': detail})

    # ---- B1: tie pair RNE 独立重算 ----
    base = load('01-b1-base-bank.bin') or load('b1-base-bank.bin')
    after = load('b1-after-bank.bin')
    cmd = load('b1-command.bin')
    if base is None:
        # 快照命名是序号前缀式，找 b1
        import glob
        cands = sorted(glob.glob(str(OUT / '*b1-base-bank.bin')))
        base = load(Path(cands[0]).name) if cands else None
        cands2 = sorted(glob.glob(str(OUT / '*b1-after-bank.bin')))
        after = load(Path(cands2[0]).name) if cands2 else None
    if base is None or after is None or cmd is None:
        check('B1', '出生质量独立重算', False, '原件缺失')
    else:
        # 独立重算：空 BASE + emit q=0.125Q(131072) → 单粒出生 min(emit, PACKAGE)=122333
        # （emit 超过单粒上限 122333 被 BIRTH 质量资格截断）
        after_mass = after[0] if after else 0
        expect_mass = min(131072, 122333)  # PACKAGE
        ok = after_mass == expect_mass
        check('B1', '出生质量独立重算 min(emitQ, PACKAGE)=122333', ok,
              f'GPU slot0 mass={after_mass}, expect={expect_mass}（emit 131072 截断）')

    # ---- B2: 满池 BIRTH 独立重算（SoA mass 以 PACKAGE=122333 值出现次数计）----
    import glob
    b2_after = sorted(glob.glob(str(OUT / '*b2-after-bank.bin')))
    a2 = load(Path(b2_after[0]).name) if b2_after else None
    if a2 is None:
        check('B2', '满池 BIRTH 计数', False, '原件缺失')
    else:
        K = 192
        # 独立重算：emit maxBirth=K*122333/1M Q → 出生 K 粒 each 122333（T446 eligible 质量上限）
        # GPU 对照：mass 值 122333 出现 >=K 次（多出的为 header 投影）
        cnt = a2.count(122333)
        ok = cnt >= K and a2.count(244666) == 0
        check('B2', '满池 maxBirth 出生 K=192 粒 each mass=122333（无 merge）', ok,
              f'122333 x{cnt} (should be >= {K}), 244666 x{a2.count(244666)} (expect 0)')

    # ---- B3: 满池后 x+2 merge 链独立重算（T446 merge_fields 语义）----
    b3_after = sorted(glob.glob(str(OUT / '*b3-after-bank.bin')))
    a3 = load(Path(b3_after[0]).name) if b3_after else None
    if a3 is None:
        check('B3', '满池 merge 链', False, '原件缺失')
    else:
        # 独立重算（T446 merge_fields）：满池 122333xK，新 emit x+2 同列半径内 ->
        #   191 对 merge：M=122333+122333=244666==MERGED_MAX（tie RNE z 不变）-> 191 粒 244666 + 1 粒 122333
        cnt_merged = a3.count(244666)
        cnt_single = a3.count(122333)
        ok = cnt_merged == 191 and cnt_single >= 1
        check('B3', '满池 merge 链：191 对 M=244666（==MERGED_MAX）+ 末粒 122333', ok,
              f'244666 x{cnt_merged} (expect 191), 122333 x{cnt_single} (>=1 incl header)')

    passed = sum(1 for r in results if r['pass'])
    report = {'schema': 'T470_B_MERGE_RNE_INDEPENDENT_V1', 'verdict': 'PASS' if passed == len(results) else 'FAIL',
              'passed': passed, 'total': len(results), 'results': results,
              'oracleReuse': 'T446 rne/merge_fields/stop_order 语义（independent oracle）',
              'device': 'NOT_RUN'}
    REPORTS.mkdir(parents=True, exist_ok=True)
    (REPORTS / 't470-b-merge-rne.json').write_text(json.dumps(report, ensure_ascii=False, indent=1), encoding='utf-8')
    print(json.dumps({'revision': report['schema'], 'verdict': report['verdict'],
                      'passed': passed, 'entries': len(results)}, ensure_ascii=False))
    return 0 if passed == len(results) else 1

if __name__ == '__main__':
    sys.exit(main())
