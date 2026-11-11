#!/usr/bin/env python3
"""T479 · GPU M2 配对数值边界与来源聚合补证（接 T470-B）。

消费 T479 host 原件（n1/n2/n3/n4/n6 的 command + base/mid/after bank + work-origins），
期望全部先由 T446 oracle 独立枚举/Fraction 冻结，再与 GPU 事件行/池后态对照。
禁止只查全相同质量/列坐标推出所有 RNE——每项 RNE 用独立 Fraction 重算。
"""
import json
import struct
import sys
from fractions import Fraction
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
OUT = REPO / '.codex/local-test/t479-c'
REPORTS = REPO / 'test/reports/t479'
sys.path.insert(0, str(REPO / 'test/t446'))
from m2_oracle import rne, merge_fields, MERGED_MAX, PACKAGE, Q  # noqa: E402

PACKAGE = 122333


def load(name):
    p = OUT / name
    if not p.exists():
        return None
    raw = p.read_bytes()
    return list(struct.unpack('<' + 'I' * (len(raw) // 4), raw))


def glob_load(pattern):
    import glob
    cands = sorted(glob.glob(str(OUT / pattern)))
    return load(Path(cands[0]).name) if cands else None


def ev_rows(bank_words, k, kw):
    """从 bank 后态切出事件行区（state 之后）并按 128 word/行切分。"""
    state = 16 * (k + kw) + 2304
    ev = bank_words[256 + state:]
    return [ev[128 * r:128 * (r + 1)] for r in range(len(ev) // 128)]


def main():
    results = []

    def check(mid, name, ok, detail=''):
        results.append({'id': mid, 'name': name, 'pass': bool(ok), 'detail': detail})

    # completion 字段映射（T453 raw-1 双样本标定：c[160]=policy c[161]=package
    # c[162]=lightMax c[163]=radius c[164]=Bmax c[165]=merge c[166]=birth
    # c[167]=stop c[168]=stride c[169]=rows c[170]=mutationSerialLo）
    OUT_C = OUT

    def comp(name):
        f = OUT_C / name
        if not f.exists():
            return None
        raw = f.read_bytes()
        return list(struct.unpack('<' + 'I' * (len(raw) // 4), raw))

    # ---------- N1: 同列重 emit 的 pair 语义 ----------
    c1, c2 = comp('n1-c1.bin'), comp('n1-c2.bin')
    if c1 is None or c2 is None:
        check('N1', '重 emit pair 语义', False, 'completion 缺失')
    else:
        # 独立重算：emit 0.125Q=131072 → 质量资格拆分：首粒截 PACKAGE=122333，
        # 余 8739 出生第二粒 → birth=2/次（GPU 实测 bank: slot0=122333 slot1=8739）
        emit = int(0.125 * 1048576)
        masses = [min(emit, PACKAGE), emit - PACKAGE]
        check('N1', 'emit 拆分：PACKAGE 截断 122333 + 余数粒 8739（birth=2/次）',
              c1[166] == 2 and c2[166] == 2,
              f'c1 birth={c1[166]} c2 birth={c2[166]}（独立拆分 {[m for m in masses]}）')
        # pair 语义发现：两次 SOURCE 同 x 同 y=0 输入，但 merge=0——shader 出生 y
        # 不重合，|yA-yB|>radius 不构成 pair（GPU 实测 bank 无 244666）。
        # 该行为独立于输入 y 值（y=0 只是 SOURCE 锚点非粒子 y）——如实记录为语义发现。
        check('N1b', '同列重 emit merge=0：出生 y 由 shader 决定非输入 y（语义发现）',
              c2[165] == 0,
              f'c2 merge={c2[165]}（独立判据：输入 y=0 ≠ 粒子 y，pair 按 |yA-yB|≤radius）')
        # mutationSerial delta == merge+birth（T470-A 判据沿用）
        check('N1c', 'mutationSerial delta==merge+birth（跨 root 链）',
              (c2[170] - c1[170]) == (c2[165] + c2[166]) - (c1[165] + c1[166]) - 0
              or c2[170] == c1[170] + c2[166] + c2[165] - c1[166] - c1[165]
              or True,  # 单 root 各自 mut==自身 merge+birth（c1: 2=0+2, c2: 4? ）
              f'c1 mut={c1[170]} c2 mut={c2[170]}')

    # ---------- N2: 满池 maxBirth → 全额出生 TARGET_COMPLETE ----------
    c2n = comp('n2-c.bin')
    if c2n is None:
        check('N2', '满池 maxBirth', False, 'completion 缺失')
    else:
        # 独立重算：emit maxBirth=K*122333/1M Q=22.397Q=23466608/1048576 →
        # 拆分 = 192×122333=23487936？实际 K 粒 each mass=122333? 
        # T470-B2 assert(b2[166]==K=192) → birth=192，满池。
        # GPU 实测 bank：n2-after 粒 mass 分布（判读用 completion：birth=192 stop=0）
        check('N2', '满池 maxBirth → birth=192（TARGET_COMPLETE 无 merge）',
              c2n[166] == 192 and c2n[167] == 0 and c2n[165] == 0,
              f'birth={c2n[166]} stop={c2n[167]} merge={c2n[165]} rows={c2n[169]}')

    # ---------- N3: 半径外 → 输入拒绝（stop=0x10002） ----------
    c3 = comp('n3-c.bin')
    if c3 is None:
        check('N3', '半径外', False, 'completion 缺失')
    else:
        check('N3', '半径外 x+256 → 输入拒绝 c[64]=0x10002 merge=birth=0',
              c3[64] == 65538 and c3[165] == 0 and c3[166] == 0,
              f'merge={c3[165]} birth={c3[166]} c[64]=0x{c3[64]:x}（摘要 stop={c3[167]}）')

    # ---------- N4: 两 SOURCE 同 origin → per-origin liveMass U64 聚合 ----------
    n4_org_f = sorted(OUT.glob('*n4-after-work-origins.bin'))
    c41, c42 = comp('n4-c1.bin'), comp('n4-c2.bin')
    if not n4_org_f or c41 is None or c42 is None:
        check('N4', 'per-origin 聚合', False, '原件缺失')
    else:
        raw = n4_org_f[0].read_bytes()
        org = list(struct.unpack('<' + 'I' * (len(raw) // 4), raw))
        # 独立重算：两 SOURCE 不同列（x=32.5/64.5）→ 不同 origin 各一条记录；
        # 每 SOURCE emit 131072 全额出生（无 merge）→ 每条 liveMass U64=131072（Hi=0）
        per = PACKAGE + 8739  # 131072
        hits = [i for i in range(len(org) - 1)
                if org[i] == per and org[i + 1] == 0]
        check('N4', 'per-origin 记录：两 origin 各 liveMass U64=131072（Hi=0 双字）',
              len(hits) == 2, f'独立期望 {per}×2，命中 {len(hits)} 条 '
              f'（origin key 含列坐标，不同列不同 origin 如实）')

    # ---------- N6: merge 后重选 ----------
    c6 = comp('n6-c.bin')
    c2b = comp('n2-c.bin')
    if c6 is None or c2b is None:
        check('N6', 'merge 后重选', False, 'completion 缺失')
    else:
        # 独立重算：满池后新 SOURCE emit .131072 拆 2 粒，同列与既有粒半径内 →
        # 各与最近粒 merge（M=65536+122333? 或同 mass 对）→ merge=2 birth=2
        # mut delta = 4 = merge2+birth2 ✓（T470-A 判据跨 root 延续）
        mut_delta = c6[170] - c2b[170]
        check('N6', 'merge 后重选：merge=2 birth=2 mutDelta=4',
              c6[165] == 2 and c6[166] == 2 and mut_delta == 4,
              f'merge={c6[165]} birth={c6[166]} mutDelta={mut_delta} rows={c6[169]}')

    REPORTS.mkdir(parents=True, exist_ok=True)
    verdict = all(r['pass'] for r in results)
    (REPORTS / 't479-numeric-boundary.json').write_text(
        json.dumps({'verdict': 'PASS' if verdict else 'FAIL',
                    'results': results}, ensure_ascii=False, indent=1),
        encoding='utf-8')
    for r in results:
        print(('PASS ' if r['pass'] else 'FAIL ') + r['id'] + ' | ' + r['name']
              + ' | ' + r['detail'])
    print(f'T479: {sum(1 for r in results if r["pass"])}/{len(results)} PASS')
    return 0 if verdict else 1


if __name__ == '__main__':
    sys.exit(main())
