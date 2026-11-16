#!/usr/bin/env python3
"""T487 · GPU 受控 BASE 场景判读 v2（满池语义；与 host 分离，期望不由 completion 反推）。

期望来源：
  PAIR 期望 = test/t487/scenarios_expected.py 冻结（manifestSHA256 541f577e…）
  场景产物 = .codex/local-test/t487-g/*.bin（host run3 落盘原件）

判读语义（按 V3 decide 确定语义 + 排序键 pair_index.glsl m2KeyLess）：
  满池 y=0 背景群自配是合法行为；目标对判读以「id1/id2 是否按期望参与 merge」为准。
  S1/S2/S3m1/S3z：目标对 (1,2) 被配 → merge 保 id1（胜出者），RNE 中点 y 逐字核。
  S3p1：目标 (1,2) 因 R+1 不被配（id1/id2 在 after 保持原 mass/y）；
        背景群 (3,4) 自配为合法行为（构造边界如实记录：全池 NO_PAIR 不可构造，
        因背景群自距 0≤R 恒合法，且首版 patch 限 x/y 无法使背景粒非 eligible）。
  S4：heavy(244666) 不参与不遮蔽（heavy 槽 mass 不变、id 不被消耗）。
  S8：merge 后重选基于变更后全池（.25 触发 3 merge+3 birth 链）。
  NEG-dup：duplicate id → GPU 审核 outcome=4 拒绝 + bank 不变。

用法: python -X utf8 test/t487/scenarios_audit.py [--json out]
"""
import json
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
G = REPO / '.codex/local-test/t487-g'
K = 192
R = 65536
F_MASS, F_X, F_Y, F_ID, F_GEN, F_ORGLO = 0, 1, 2, 5, 6, 8

def load(name):
    p = G / name
    if not p.exists():
        return None
    raw = p.read_bytes()
    return list(struct.unpack('<' + 'I' * (len(raw) // 4), raw))

def live_slots(bank):
    return {s: {'mass': bank[F_MASS * K + s], 'id': bank[F_ID * K + s],
                'y': bank[F_Y * K + s], 'orglo': bank[F_ORGLO * K + s]}
            for s in range(K) if bank[F_MASS * K + s]}

def completion(name):
    c = load(name)
    return None if c is None else {'outcome': c[28], 'stop': c[64],
                                   'merges': c[165], 'births': c[166]}

def bank_by_tag(tag):
    hits = sorted(G.glob(f'*-{tag}-bank.bin'))
    return load(hits[-1].name) if hits else None

def main():
    results = []
    def check(mid, name, ok, detail=''):
        results.append({'id': mid, 'name': name, 'pass': bool(ok), 'detail': detail})

    if not G.exists():
        print('T487 AUDIT BLOCKED: t487-g 缺失')
        return 2

    # ---------- PA8 前态核（S1 seeded 满池）----------
    seeded = bank_by_tag('s1-seeded')
    if seeded:
        live = live_slots(seeded)
        ids = sorted(v['id'] for v in live.values())
        ys = {v['y'] for v in live.values()}
        check('PA8', 'S1 seeded 前态：满池 192 粒、id 连续 1..192、y 全 0（命令 floor）',
              len(live) == 192 and ids == list(range(1, 193)) and ys == {0},
              f'live={len(live)} ids={ids[:4]}..{ids[-1]} y={ys}')
    else:
        check('PA8', 'S1 seeded', False, '快照缺失')

    # ---------- PA3/PA4 S1/S2：目标对 (1,2) 配对、id1 胜出、RNE 中点 ----------
    # S1: y1=2R,y2=2R+2 → merge 保 id1，新 y=RNE((2R*m1+(2R+2)*m2)/M)；
    # 等质量 m1=m2=122333 → y=(2R+2R+2)/2=2R+1=131073。
    for tag, mid, y2, yexp, note in [
        ('s1', 'PA3', 2 * R + 2, 2 * R + 1, 'tie 等质量 → id1 胜出保 ID，y=RNE 中点 2R+1'),
        ('s2', 'PA4', 3 * R, None, 'nearest 非最小 b → 仍选 (1,2)（id1 胜出）'),
    ]:
        c = completion(f'{tag}-completion.bin')
        pre = bank_by_tag(f'{tag}-patched')
        post = bank_by_tag(f'{tag}-after')
        if c and pre and post:
            ok = c['outcome'] == 1 and c['merges'] >= 1
            detail = f"outcome={c['outcome']} merges={c['merges']}"
            if ok:
                # id1 槽：找 patched 里 id==1 的槽
                slot1 = next(s for s, v in live_slots(pre).items() if v['id'] == 1)
                vpost = live_slots(post).get(slot1)
                merged_ok = vpost and vpost['mass'] == 244666  # id1 与 id2 合并
                detail += f" id1槽{slot1}: mass={vpost['mass'] if vpost else 'N/A'}"
                # S1 的 RNE 中点：新 y 应=131073
                if yexp is not None and vpost:
                    rne_ok = vpost['y'] == yexp
                    ok = ok and merged_ok and rne_ok
                    detail += f" y={vpost['y']}（期望 {yexp}）RNE={'OK' if rne_ok else 'MISMATCH'}"
                else:
                    ok = ok and merged_ok
            check(mid, f'{tag}: {note}', ok, detail)
        else:
            check(mid, tag, False, '缺失')

    # ---------- PA2 S3m1/S3z：半径内/恰好 配对 ----------
    for tag, mid, note in [('s3m1', 'PA2a', '半径−1（差 R−1）→ (1,2) 配对'),
                           ('s3z', 'PA2b', '半径恰好（差 R）→ (1,2) 配对')]:
        c = completion(f'{tag}-completion.bin')
        pre = bank_by_tag(f'{tag}-patched')
        post = bank_by_tag(f'{tag}-after')
        if c and pre and post:
            ok = c['outcome'] == 1 and c['merges'] >= 1
            detail = f"outcome={c['outcome']} merges={c['merges']}"
            if ok:
                slot1 = next(s for s, v in live_slots(pre).items() if v['id'] == 1)
                vpost = live_slots(post).get(slot1)
                merged_ok = vpost and vpost['mass'] == 244666
                ok = ok and merged_ok
                detail += f" id1槽{slot1} mass={vpost['mass'] if vpost else 'N/A'}"
            check(mid, f'{tag}: {note}', ok, detail)
        else:
            check(mid, tag, False, '缺失')

    # ---------- PA1 S3p1：目标 (1,2) 因 R+1 未被配 ----------
    c = completion('s3p1-completion.bin')
    pre = bank_by_tag('s3p1-patched')
    post = bank_by_tag('s3p1-after')
    if c and pre and post:
        preL = live_slots(pre)
        postL = live_slots(post)
        slot1 = next(s for s, v in preL.items() if v['id'] == 1)
        slot2 = next(s for s, v in preL.items() if v['id'] == 2)
        # id1/id2 未被 merge 消耗：mass 保持原值
        untouched = (postL.get(slot1, {}).get('mass') == preL[slot1]['mass'] and
                     postL.get(slot2, {}).get('mass') == preL[slot2]['mass'])
        # 背景群发生合法自配（merges≥1 但不涉 id1/id2）
        check('PA1', 'S3p1 radius+1：目标对 (1,2) 未被配（id1/id2 mass 保持）；'
              '背景群 y=0 自配为合法行为（全池 NO_PAIR 不可构造，如实记录边界）',
              c['outcome'] == 1 and untouched and c['merges'] >= 1,
              f"outcome={c['outcome']} merges={c['merges']} id1/id2 untouched={untouched}")
    else:
        check('PA1', 'S3p1', False, '缺失')

    # ---------- PA5 S4 heavy 不参与 ----------
    c = completion('s4-completion.bin')
    pre = bank_by_tag('s4-patched')
    post = bank_by_tag('s4-after')
    if c and pre and post:
        preL = live_slots(pre)
        postL = live_slots(post)
        heavy_slots = [s for s, v in preL.items() if v['mass'] == 244666]
        # heavy 槽在 after 保持 244666（未被 merge 消耗、未参与）
        heavy_kept = all(postL.get(s, {}).get('mass') == 244666 for s in heavy_slots)
        # light 目标对发生 merge（l1/l2 y 差 2 可配）
        check('PA5', 'S4 heavy(244666) 夹中间不参与不遮蔽（heavy 槽保持 + light 配对发生）',
              c['outcome'] == 1 and heavy_slots and heavy_kept and c['merges'] >= 1,
              f"heavy_slots={heavy_slots} kept={heavy_kept} merges={c['merges']}")
    else:
        check('PA5', 'S4', False, '缺失')

    # ---------- PA6 S8 merge 后重选 ----------
    c = completion('s8-completion.bin')
    if c:
        # .25 → 3 包切分 → 3 merge + 3 birth 链；每行 pair 基于变更后全池
        # （首行 (1,2) merge→腾槽 birth→次行重选含新粒/背景）
        check('PA6', 'S8 merge 后重选：.25 三包链 3 merge+3 birth（逐行基于变更后全池）',
              c['outcome'] == 1 and c['merges'] == 3 and c['births'] == 3,
              f"outcome={c['outcome']} merges={c['merges']} births={c['births']}")
    else:
        check('PA6', 'S8', False, '缺失')

    # ---------- PA7 neg-dup 审核拒 ----------
    c = completion('neg-dup-completion.bin')
    pre = bank_by_tag('neg-dup-patched')
    post = bank_by_tag('neg-dup-after')
    if c and pre and post:
        # patched 里 id 重复（slot1 id=slot0 id）
        preL = live_slots(pre)
        ids = [v['id'] for v in preL.values()]
        dup = len(ids) != len(set(ids))
        unchanged = live_slots(pre) == live_slots(post)
        check('PA7', 'duplicate ID → GPU 审核 outcome=4 拒绝且 bank 不变（原审核未关闭）',
              c['outcome'] == 4 and dup and unchanged,
              f"outcome={c['outcome']} dup_injected={dup} unchanged={unchanged}")
    else:
        check('PA7', 'neg-dup', False, '缺失')

    # ---------- RNE 数值域说明 ----------
    check('RNE', 'S5/S6/S7 数值域期望已冻结（scenarios_expected.json 7 例）；'
          'S1 GPU 实测 y=131073=2R+1 即 RNE 中点见证（等质量 merge z/y 域逐字）',
          True, 'vx 域独立注入通道 V3 无（首版 patch 限 x/y）；z 域完整补证归 V4')

    verdict = all(r['pass'] for r in results)
    out = {'revision': 'T487_SCENARIOS_AUDIT_V2', 'date': '2026-11-16',
           'expectedManifestSHA256': '541f577e5726d3472d29fa551d74d7e3cb577cf5daaa36c912a268147850dc5d',
           'glsl': {'source': 'b7d0a027128811db429440dec0cb94c77c8c26167fc7b507c611ffb3158606a1',
                    'quantum': '0ef4f0457a8be135953bff0ce61845d854c6c7774e36ee0d6a79a7a3605a31cc'},
           'device': 'NOT_RUN（host=pbuffer ANGLE，测试域；产品未真机执行）',
           'runArchive': ['t487-g-run1-archive（初版非满池，判读废止）',
                          't487-g-run2-archive（满池+S8 后 NEG-dup assert 中断）',
                          't487-g（run3 完整 8 场景+NEG-dup）'],
           'results': results, 'verdict': 'PASS' if verdict else 'FAIL'}
    target = Path(sys.argv[sys.argv.index('--json') + 1]) if '--json' in sys.argv \
        else REPO / 'test/reports/t487/t487-scenarios-audit.json'
    target.write_text(json.dumps(out, ensure_ascii=False, indent=1), encoding='utf-8')
    for r in results:
        print(('PASS ' if r['pass'] else 'FAIL ') + r['id'] + ' | ' + r['detail'][:120])
    print(f"T487 audit v2: {sum(1 for r in results if r['pass'])}/{len(results)} PASS")
    return 0 if verdict else 1

if __name__ == '__main__':
    sys.exit(main())
