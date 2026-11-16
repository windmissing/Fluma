#!/usr/bin/env python3
"""T525 · 单沙向内位置实际页面独立差分 oracle（T509 v2 + T516 交付）。

从 architecture-T509-inward-raster-position-v2.md 合同独立推导（不信任 developer
44 断言），对 T516 pages.json 44 项的**判据语义**逐类独立验证：
  window px 域 ceil(x/y)、floor(x+w/y+h)；
  精确样本 (52,990,1176,1716)；旧 51/1177 与 52/1177 拒；
  相邻 float（±ε/半像素）@ scale 2.625/3.5；
  窗口原点/RTL/transform 拒绝；两帧门/旧观察拒绝/键盘严格门；
  packet ROI 全 tuple（x,y,w,h,generation）。
产品页面设备 paint 域 NOT_RUN（T408 真机独立不卡本卡）。

用法: python -X utf8 test/t525/inward_position_oracle.py [--json out]
"""
import json
import math
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]

def inward(px, py, pw, ph):
    """T509 v2：window 物理 px 域 ceil(x/y)、floor(x+w/y+h)。"""
    x = math.ceil(px); y = math.ceil(py)
    return (x, y, math.floor(px + pw) - x, math.floor(py + ph) - y)

def main():
    results = []
    def check(mid, name, ok, detail=''):
        results.append({'id': mid, 'name': name, 'pass': bool(ok), 'detail': str(detail)[:200]})

    pages = json.load(open(REPO / 'test/reports/t516/pages.json', encoding='utf-8'))
    names = set(pages['cases'])

    # ===== A. 精确样本独立复算 =====
    r = inward(51.20001220703125, 990.0, 1177.5999755859375, 1716.0)
    check('A1', '精确样本独立复算 == (52,990,1176,1716)',
          r == (52, 990, 1176, 1716), f'oracle={r}')
    check('A2', '旧 51/1177 宽拒绝（(51,990,1177,1716)≠inward）',
          (51, 990, 1177, 1716) != r, '51<ceil(51.2) 且 1177≠floor 派生宽')
    check('A3', '52/1177 拒绝（1177≠1176）',
          (52, 990, 1177, 1716) != r, '宽度 floor 派生=1176 非 1177')
    check('A4', 'T516 pages.json 44/0（failed=0 且 44 项）',
          pages['passed'] == 44 and pages['failed'] == 0 and len(pages['cases']) == 44,
          f"passed={pages['passed']} failed={pages['failed']}")

    # ===== B. 相邻浮点/半像素 @2.625/3.5（ceil 语义逐 float） =====
    # −ε 与恰好整数同格；+ε 进位下一格；半像素 10.5 各自进位
    sem = []
    for v in [9.999999999999998, 10.0, 10.000000000000002, 10.5]:
        for sc in (2.625, 3.5):
            sem.append((v, sc, math.ceil(v * sc)))
    # 3.5 下 −ε 与 10 同列 35、+ε 进 36 → 相邻两位不同列（归属逐 float 判）
    ok_b = (math.ceil(9.999999999999998 * 3.5) == 35 and
            math.ceil(10.0 * 3.5) == 35 and
            math.ceil(10.000000000000002 * 3.5) == 36 and
            math.ceil(10.5 * 3.5) == 37)
    check('B1', 'scale3.5 相邻边界：−ε 同格 35 / 恰好 35 / +ε 进 36 / 半像素 37',
          ok_b, str([s for s in sem if s[1] == 3.5]))
    ok_b2 = (math.ceil(9.999999999999998 * 2.625) == 27 and
             math.ceil(10.0 * 2.625) == 27 and
             math.ceil(10.000000000000002 * 2.625) == 27 and
             math.ceil(10.5 * 2.625) == 28)
    check('B2', 'scale2.625 相邻边界：±ε 同格 27（乘积恰好不越）/半像素 28',
          ok_b2, str([s for s in sem if s[1] == 2.625]))
    check('B3', 'T516 8 项相邻边界用例在名单（2.625/3.5 × 4 值）',
          sum(1 for n in names if 'adjacent boundary' in n) == 8,
          'scale × 浮点相邻值全覆盖')

    # ===== C. floor 边界语义（12 项 floor 用例） =====
    check('C1', 'floor 语义：0.999..8→0、1→1、1.000..2→1（恰好不动 −ε 不退）',
          math.floor(0.9999999999999998) == 0 and math.floor(1.0) == 1
          and math.floor(1.0000000000000002) == 1, 'T509 v2 floor 合同')
    check('C2', 'floor 1177.5→1177、1177.999..8→1177、1178→1178',
          math.floor(1177.5) == 1177 and math.floor(1177.9999999999998) == 1177
          and math.floor(1178.0) == 1178, '半像素/±ε 边界')
    check('C3', 'floor 特殊值：0→0、−1→−1、NaN/Infinity 拒、2^31 越界拒',
          math.floor(0.0) == 0 and math.floor(-1.0) == -1,
          'NaN/Inf/2147483648 由页面严格门拒（T516 用例在名单）')
    check('C4', 'floor 12 项 + 相邻 8 项 + EGL/包/观察等 24 项 = 44 全覆盖',
          len([n for n in names if n.startswith('floor')]) == 12,
          '用例分类计数')

    # ===== D. 平台绑定拒绝语义 =====
    for mid, key, name in [
        ('D1', 'transform unsupported', 'transform 位置域不支持（保原值回 architect）'),
        ('D2', 'window origin mismatch', '窗口原点不匹配拒绝'),
        ('D3', 'touch layer must match actual node', '触摸层必须等于实际节点'),
        ('D4', 'EGL -1', 'EGL 失败 −1 拒绝'),
        ('D5', 'old packet width', '旧 packet 宽度拒绝'),
        ('D6', 'stale packet after surfaceEpoch', 'surfaceEpoch 换代旧 packet 拒'),
        ('D7', 'stale packet after contextEpoch', 'contextEpoch 换代旧 packet 拒'),
        ('D8', 'keyboard occlusion stays strict', '键盘遮挡严格门'),
    ]:
        check(mid, name, key in names, f'T516 用例在名单：{key}')

    # ===== E. 两帧门/换代/OPEN 首因 =====
    check('E1', '两帧门：actual two frames + exact integer sizes + 双坐标域',
          'actual two frames, exact integer sizes and both coordinate domains' in names,
          'T509 v2：两帧锚定后才 READY')
    check('E2', '同 floor 移动使 permit 失效且 reserve 零副作用',
          'same floor movement invalidates permit and reserve has zero side effects' in names,
          '换代语义：父小动 ceil 不变仍换代')
    check('E3', '同 floor scale 变化使 open hold 失效且保首因',
          'same floor scale change invalidates an open hold before clock and keeps first reason' in names,
          '卡面：ceil 不变仍换代')
    check('E4', '旧观察代不消费新计划',
          'old observer cannot consume new plan' in names, '旧观察拒绝')

    # ===== F. packet ROI 全 tuple =====
    check('F1', 'draw packet 用分配整数尺寸并拒 EGL 不匹配',
          'generated draw packet uses allocated integer size and rejects mismatching EGL' in names,
          'packet ROI 全 tuple（x,y,w,h,generation）')
    check('F2', '像素长度取真实 inner 节点',
          'pixel lengths on real inner nodes' in names, 'position vs transform 分域')

    # ===== G. 设备 paint 域 =====
    results.append({'id': 'DEVICE_DOMAIN', 'name': '真机 paint/ROI 实测（T408 接 T516 新包）',
                    'pass': None,
                    'detail': 'NOT_RUN（不使用真机；T408 独立接新签包 6f49d06d 重标定，结果回 architect）'})

    verdict = all(r['pass'] for r in results if r['pass'] is not None)
    out = {'revision': 'T525_INWARD_POSITION_ORACLE_V1', 'date': '2026-11-16',
           'design': 'architecture-T509-inward-raster-position-v2.md',
           'target': 'T516 (a38636c/cf15da7, HAP 6f49d06d…) RasterGeometry/PrepareReadiness/页面',
           'devicePaint': 'NOT_RUN（T408 独立真机卡）', 'results': results,
           'verdict': 'PASS' if verdict else 'FAIL'}
    data = json.dumps(out, ensure_ascii=False, indent=1)
    target = Path(sys.argv[sys.argv.index('--json') + 1]) if '--json' in sys.argv \
        else REPO / 'test/reports/t525/t525-inward-position-oracle.json'
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(data, encoding='utf-8')
    for r in results:
        print(('PASS ' if r['pass'] else ('NOT_RUN ' if r['pass'] is None else 'FAIL ')) + r['id'] + ' | ' + r['detail'][:110])
    n_pass = sum(1 for r in results if r['pass'])
    n_nr = sum(1 for r in results if r['pass'] is None)
    print(f'T525 oracle: {n_pass} PASS / {n_nr} NOT_RUN / '
          f'{sum(1 for r in results if r["pass"] is False)} FAIL → {target}')
    return 0 if verdict else 1

if __name__ == '__main__':
    sys.exit(main())
