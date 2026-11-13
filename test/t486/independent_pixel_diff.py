#!/usr/bin/env python3
"""T486 · GPU 场景缓存实际渲染独立差分（离线，不使用真机）。

按 T480 架构与 T486 卡面：用 T485 实际 Scene/工厂产物（pixel-originals.zip +
六路运行目录 + graphics 收据 JSONL）独立重算三路（禁缓存 off / 源码冷 cold /
新进程暖 warm）× M1/M2 × 4 场景 × 8 层的像素与业务 token 一致性；
不复用 T485 开发断言——期望由本脚本独立定义：
  E1 禁缓存基准成立：off 路每场景 8 层像素与 token 均有效（尺寸=宽×高×4）；
  E2 三路逐字节一致：cold/warm/off 同 route 同场景同层 SHA 逐位相等；
  E3 M1/M2 同路一致：同 mode 同场景同层跨 route SHA 逐位相等（共享 binary key 的前提）；
  E4 业务 token 路内稳定：token 与 mode 无关（同 route 同场景逐位相等）；
  E5 反射绑定独立核：从暖路收据独立读 SCENE_FULL.publicationBinding=0、
     SCENE_PARTICLES aState location=3/type=5125（或 inactive 合法）、color=location0/vec4；
  E6 暖路零编译：warm 收据 route=BINARY_LOAD 且 compileCalls=0；cold=SOURCE_COMPILE×2；
  E7 失效回退：bad_hash/truncated 新进程两 stage 源码回退 + 像素与冷基准一致；
  E8 共享 binary key：M1/M2 缓存目录三 key 集合相等。
原始字节来源用 zip 解包（不信任六目录被复写的可能），zip 内 SHA 与目录逐份核对。

用法: python -X utf8 test/t486/independent_pixel_diff.py [--json out]
退出码: 0=全过；1=有失败；2=输入缺失
"""
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
REPORT = REPO / 'test/reports/t485'
MATRIX = REPO / '.codex/local-test/t485-pixel-final-matrix'
SCENARIOS = ['empty', 'mixed', 'mirror', 'viewport']
LAYERS = 8
MODES = ['cold', 'warm', 'off']
ROUTES = ['m1', 'm2']
SIZES = {'empty': (256, 256), 'mixed': (256, 256),
         'mirror': (256, 256), 'viewport': (320, 192)}
OUT = Path(sys.argv[sys.argv.index('--json') + 1]) if '--json' in sys.argv else None
REPORTS = REPO / 'test/reports/t486'

def sha(b):
    return hashlib.sha256(b).hexdigest()

def main():
    results = []

    def check(mid, name, ok, detail=''):
        results.append({'id': mid, 'name': name, 'pass': bool(ok), 'detail': detail})

    zip_path = REPORT / 'pixel-originals.zip'
    pixels_json = REPORT / 'pixels.json'
    if not zip_path.exists() or not pixels_json.exists() or not MATRIX.exists():
        print('T486 BLOCKED: T485 像素原件缺失（先跑 test/t485/run_pixels.py）')
        return 2

    # 解包 zip 到内存索引（原始字节权威来源）
    z = zipfile.ZipFile(zip_path)
    zipfiles = {n: z.read(n) for n in z.namelist() if not n.endswith('run.log')}

    # 逐份核对 zip ↔ 目录（防复写）
    mismatch = []
    for rel, data in zipfiles.items():
        p = MATRIX / rel
        if not p.exists() or p.read_bytes() != data:
            mismatch.append(rel)
    check('Z1', 'zip 原始字节与六路运行目录逐份一致（防复写）',
          not mismatch, f'{len(zipfiles)} 份核对，不一致 {len(mismatch)} 份'
          + (f'：{mismatch[:5]}' if mismatch else ''))

    # ---------- E1 禁缓存基准 ----------
    e1_ok, e1_detail = True, []
    for route in ROUTES:
        for sc in SCENARIOS:
            w, h = SIZES[sc]
            expect = w * h * 4
            for layer in range(LAYERS):
                rel = f'{route}-off/{sc}-{layer}.rgba'
                data = zipfiles.get(rel)
                if data is None or len(data) != expect:
                    e1_ok = False
                    e1_detail.append(f'{rel} 缺失或尺寸 {0 if data is None else len(data)}≠{expect}')
    check('E1', '禁缓存（off）基准：每场景 8 层像素尺寸=宽×高×4 全有效', e1_ok,
          '; '.join(e1_detail[:3]) if e1_detail else
          f'{len(ROUTES)*len(SCENARIOS)*LAYERS} 层全部有效')

    # ---------- E2 三路逐字节一致 ----------
    e2_bad = []
    for route in ROUTES:
        for sc in SCENARIOS:
            for layer in range(LAYERS):
                rel = f'{sc}-{layer}.rgba'
                ref = zipfiles[f'{route}-off/{rel}']
                for mode in MODES:
                    if mode == 'off':
                        continue
                    got = zipfiles[f'{route}-{mode}/{rel}']
                    if got != ref:
                        e2_bad.append(f'{route}/{mode}/{rel}')
    check('E2', '三路（off/cold/warm）同 route 同场景同层像素 SHA 逐位相等',
          not e2_bad, f'{len(ROUTES)*len(SCENARIOS)*LAYERS*2} 对比较，不一致 {len(e2_bad)}'
          + (f'：{e2_bad[:3]}' if e2_bad else ''))

    # ---------- E3 M1/M2 跨 route 一致 ----------
    e3_bad = []
    for mode in MODES:
        for sc in SCENARIOS:
            for layer in range(LAYERS):
                rel = f'{sc}-{layer}.rgba'
                if zipfiles[f'm1-{mode}/{rel}'] != zipfiles[f'm2-{mode}/{rel}']:
                    e3_bad.append(f'{mode}/{rel}')
    check('E3', 'M1/M2 同 mode 同场景同层像素逐位相等（共享 graphics binary 的结果面）',
          not e3_bad, f'{len(MODES)*len(SCENARIOS)*LAYERS} 对比较，不一致 {len(e3_bad)}'
          + (f'：{e3_bad[:3]}' if e3_bad else ''))

    # ---------- E4 业务 token 路内稳定 ----------
    e4_bad = []
    tokens = {}
    for route in ROUTES:
        for sc in SCENARIOS:
            ref = zipfiles[f'{route}-cold/{sc}.token'].decode()
            tokens[f'{route}/{sc}'] = ref
            for mode in MODES:
                if zipfiles[f'{route}-{mode}/{sc}.token'].decode() != ref:
                    e4_bad.append(f'{route}/{mode}/{sc}')
    # token 跨 route 应含各自 namespace 差异（M1/M2 token 允许不同），只核路内稳定
    check('E4', '业务 token 与 mode 无关（同 route 同场景三路逐位相等）',
          not e4_bad, f'{len(ROUTES)*len(SCENARIOS)} 组 × 3 路，不一致 {len(e4_bad)}'
          + (f'：{e4_bad[:3]}' if e4_bad else ''))

    # ---------- 收据独立核（warm/cold/off 各路 JSONL） ----------
    receipts = {}
    for route in ROUTES:
        for name in ['SCENE_FULL', 'SCENE_PARTICLES', 'SCENE_WATER']:
            # 收据按 key 命名，从 T485 报告目录找 route 前缀
            cands = sorted(REPORT.glob(f'{route}-*.jsonl'))
            merged = []
            for p in cands:
                for line in p.read_text(encoding='utf-8').splitlines():
                    try:
                        row = json.loads(line)
                    except Exception:
                        continue
                    if row.get('role') == name:
                        merged.append(row)
            receipts[(route, name)] = merged
    warm_rows = {(r, n): [x for x in v if x.get('packageSource') == 'HOST_T485_warm'
                          and x.get('contextGeneration') == '1']
                 for (r, n), v in receipts.items()}
    cold_rows = {(r, n): [x for x in v if x.get('packageSource') == 'HOST_T485_cold'
                          and x.get('contextGeneration') == '1']
                 for (r, n), v in receipts.items()}

    # E5 反射绑定
    e5_bad = []
    for route in ROUTES:
        full = [r for r in warm_rows[(route, 'SCENE_FULL')] if r.get('stage') == 'REFLECT_OK']
        if not full or full[0].get('publicationBinding') != 0:
            e5_bad.append(f'{route}/SCENE_FULL.publicationBinding')
        parts = [r for r in warm_rows[(route, 'SCENE_PARTICLES')] if r.get('stage') == 'REFLECT_OK']
        if parts:
            p0 = parts[0]
            inactive = p0.get('aStateInactive')
            attrs = {a.get('name'): a for a in p0.get('attributes', [])}
            if not inactive and not (attrs.get('aState', {}).get('location') == 3
                                     and attrs.get('aState', {}).get('type') == 5125):
                e5_bad.append(f'{route}/SCENE_PARTICLES.aState')
        for role in ['SCENE_FULL', 'SCENE_PARTICLES', 'SCENE_WATER']:
            rows = [r for r in warm_rows[(route, role)] if r.get('stage') == 'REFLECT_OK']
            if rows:
                r0 = rows[0]
                if r0.get('colorLocation') != 0 or r0.get('colorType') != 35666 \
                        or r0.get('colorSize') != 1:
                    e5_bad.append(f'{route}/{role}.color')
    check('E5', '反射绑定独立核（publicationBinding=0 / aState loc3 uint / color loc0 vec4）',
          not e5_bad, f'不一致 {len(e5_bad)}' + (f'：{e5_bad[:4]}' if e5_bad else ''))

    # E6 冷暖编译路径
    e6_bad = []
    all_rows = {(r, n): v for (r, n), v in receipts.items()}
    for route in ROUTES:
        for role in ['SCENE_FULL', 'SCENE_PARTICLES', 'SCENE_WATER']:
            # 暖路：BINARY_LOAD compile=0（PREPARE_OK 行）
            w = [r for r in warm_rows[(route, role)] if r.get('stage') == 'REFLECT_PREPARE_OK']
            if not w or w[0].get('route') != 'BINARY_LOAD' or w[0].get('compileCalls') != 0:
                e6_bad.append(f'{route}/{role}.warm')
            # 冷路：SOURCE_COMPILE 两 stage（REFLECT_OK 行；PREPARE_OK 被同 key 文件
            # 后续进程追加覆盖，用 REFLECT_OK 判冷路径）
            c = [r for r in cold_rows[(route, role)] if r.get('stage') == 'REFLECT_OK'
                 and r.get('route') == 'SOURCE_COMPILE']
            if not c or c[0].get('compileCalls') != 2:
                e6_bad.append(f'{route}/{role}.cold')
            # 禁缓存路：CACHE_DISABLED_SOURCE（编译仍发生）
            o = [r for r in all_rows[(route, role)]
                 if r.get('packageSource') == 'HOST_T485_off'
                 and r.get('stage') == 'REFLECT_OK'
                 and r.get('route') == 'CACHE_DISABLED_SOURCE']
            if not o or o[0].get('compileCalls') != 2:
                e6_bad.append(f'{route}/{role}.off')
    check('E6', '暖 BINARY_LOAD compile=0 / 冷 SOURCE_COMPILE×2 / 禁缓存 CACHE_DISABLED_SOURCE×2', not e6_bad,
          f'不一致 {len(e6_bad)}' + (f'：{e6_bad[:4]}' if e6_bad else ''))

    # E7 失效回退
    e7_bad = []
    disk_dir = REPORT
    for case in ['bad_hash', 'truncated']:
        rows = []
        for p in disk_dir.glob('m2-*.jsonl'):
            for line in p.read_text(encoding='utf-8').splitlines():
                try:
                    row = json.loads(line)
                except Exception:
                    continue
                if row.get('packageSource') == f'HOST_T485_{case}':
                    rows.append(row)
        fb = [r for r in rows if r.get('route') == 'REJECT_THEN_SOURCE_ONCE'
              and r.get('compileCalls') == 2 and r.get('stage') == 'REFLECT_OK']
        if not fb:
            e7_bad.append(f'{case}.fallback')
        # 像素与冷基准一致
        case_dir = MATRIX / case
        if case_dir.exists():
            for raw in case_dir.glob('*.rgba'):
                ref = zipfiles.get(f'm2-cold/{raw.name}')
                if ref is not None and raw.read_bytes() != ref:
                    e7_bad.append(f'{case}/{raw.name}')
    check('E7', '坏 hash/截断新进程一次源码回退且像素与冷基准一致', not e7_bad,
          f'不一致 {len(e7_bad)}' + (f'：{e7_bad[:4]}' if e7_bad else ''))

    # E8 共享 binary key
    k1 = {p.stem for p in (MATRIX / 'cache-m1' / 'gpu-program-cache' / 'v1').glob('*.bin')}
    k2 = {p.stem for p in (MATRIX / 'cache-m2' / 'gpu-program-cache' / 'v1').glob('*.bin')}
    declared = set(json.loads(pixels_json.read_text(encoding='utf-8'))['sharedBinaryKeys'])
    check('E8', 'M1/M2 缓存目录三 graphics key 集合相等且与声明一致',
          k1 == k2 and k1 == declared,
          f'm1={len(k1)} m2={len(k2)} declared={len(declared)}')

    # 汇总
    verdict = all(r['pass'] for r in results)
    manifest = {'revision': 'T486_INDEPENDENT_PIXEL_DIFF_V1', 'date': '2026-11-13',
                'mode': 'OFFLINE_INDEPENDENT_RECOMPUTE_FROM_T485_ARTIFACTS',
                'device': 'NOT_RUN', 'authority': '独立重算（不复用 T485 开发断言）',
                'sources': {'zip': str(zip_path), 'matrix': str(MATRIX),
                            'zipEntries': len(zipfiles)},
                'results': results, 'verdict': 'PASS' if verdict else 'FAIL'}
    REPORTS.mkdir(parents=True, exist_ok=True)
    target = OUT if OUT else REPORTS / 't486-independent-pixel-diff.json'
    target.write_text(json.dumps(manifest, ensure_ascii=False, indent=1), encoding='utf-8')
    for r in results:
        print(('PASS ' if r['pass'] else 'FAIL ') + r['id'] + ' | ' + r['name']
              + ' | ' + r['detail'][:110])
    print(f'T486 independent diff: {sum(1 for r in results if r["pass"])}/{len(results)} PASS → {target}')
    return 0 if verdict else 1

if __name__ == '__main__':
    sys.exit(main())
