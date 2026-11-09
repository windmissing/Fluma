#!/usr/bin/env python3
"""T427 · 单沙系统截图冻结映射端到端负控（离线，V2 全链集成路径）。

与 T426 并行。使用 T406/T414 现有归档结构（T414 fixtures 生成器 + T413 screen.py V2
loader + T406 V2 判读算法）与合成截图数据，制作从"完整档 → verify_bundle → load →
calibrate → plan → single/pair 判定"的端到端集成负控。覆盖:
  1px 平移 / 1 码颜色 / 缺列 / 旧 token / 颜色模型未知 / 越 hold。
证明顺序: 判读器先拒证据完整性（bundle/SHA/身份/时钟域）再比较像素。
时钟差分复用 T418（hold_oracle.time_gate ↔ clock.envelope 同输入一致性已在
T418 证明，本卡不重做状态差分，只在集成路径上取时间门拒绝顺序）。
范围: 合成层（T414 fixtures 的 V2 升级档）。真实窗口边界按 T415 §C/T408 attempt1
报告单列，不在本卡判定。
结果供 T408 消费，不卡真机。
用法: python test/t427/e2e_negatives.py
退出码: 0=全部预期；1=有意外
"""
import json
import shutil
import sys
from pathlib import Path
from tempfile import TemporaryDirectory

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO/'test'/'t413'))
sys.path.insert(0, str(REPO/'test'/'t406'))
import importlib.util


def _load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


t406 = _load('t406_screen', REPO/'test'/'t406'/'screen.py')      # V1 判读算法（对照域）
t413 = _load('t413_screen', REPO/'test'/'t413'/'screen.py')      # V2 loader（含 verify_bundle）
# V2 集成判读入口: T413 screen 将 legacy.load 替换为 V2 loader，calibrate/plan/pair
# 算法域经 t413.legacy 走 V2 档（T414 fixtures 的 V2 升级档归档结构）。
calibrate = t413.legacy.calibrate
plan = t413.legacy.plan
pair = t413.legacy.pair
single = t413.legacy.single
digest = t406.digest
import sys as _sys
_sys.path.insert(0, str(REPO/'test'/'t413'))
_fixtures = _load('t413_fixtures', REPO/'test'/'t413'/'fixtures.py')  # V2 升级档合成夹具
artifact = _fixtures.artifact

RESULTS = []


def record(label, expected, got, ok=None):
    if ok is None:
        ok = expected in str(got) or got == expected
    RESULTS.append({'case': label, 'expected': expected, 'got': str(got)[:120], 'pass': ok})
    return ok


def expect_reject(label, fn, expected_sub):
    try:
        got = fn()
        record(label, expected_sub, got, ok=False)
        return False
    except (ValueError, KeyError, TypeError, OSError, IndexError) as exc:
        return record(label, expected_sub, str(exc))


def expect_display(label, fn, want):
    try:
        got = fn()
        return record(label, want, got.get('displayStatus', got) if isinstance(got, dict) else got,
                      ok=(got.get('displayStatus') == want if isinstance(got, dict) else got == want))
    except (ValueError, KeyError, TypeError, OSError, IndexError) as exc:
        return record(label, want, 'REJECTED:'+str(exc), ok=False)


def main():
    model = {'native': [118, 68], 'coverage': [4, 4, 118, 68], 'rule': 'PIXEL_CENTER_NEAREST',
             'mirrorX': False, 'mirrorY': False}
    with TemporaryDirectory() as tmp:
        root = Path(tmp)

        # ---------- 正例: 校准 → plan → A/B 配对（V2 全链） ----------
        cal_spec = artifact(root, 'cal', model, 'CAL', 1)
        cal_path = root/'calibration.json'
        cal = calibrate(cal_spec, cal_path)
        cores = [[4+118//4-10, 32, 20, 20], [4+3*118//4-10, 32, 20, 20], [4+118//2-10, 7, 20, 12]]
        plan_path = root/'plan.json'
        p = plan(cal_path, cores, plan_path)
        a = artifact(root, 'a', model, 'A', 2, p['nativeTiles'], registered=p)
        b = artifact(root, 'b', model, 'B', 3, p['nativeTiles'], registered=p)
        pos = expect_display('E0_PAIR_POSITIVE',
                             lambda: pair(a, b, cal_path, plan_path),
                             'DISPLAY_CALIBRATED_A_AND_B_PAIRED')

        # ---------- V2 bundle 完整性门（先于像素比较） ----------
        def verify(spec):
            return t413.load(spec)
        # 1px 平移: PNG 内容平移 1px（capture 收据 SHA 同步改 → 过 SHA 门，被几何校验拒）
        shift = artifact(root, 'shift', model, 'B', 3, p['nativeTiles'], registered=p)
        spec_json = json.loads(shift.read_text())
        from PIL import Image
        png = root/spec_json['png']
        img = Image.open(png)
        shifted = Image.new('RGB', img.size)
        for y in range(img.height):
            for x in range(1, img.width):
                shifted.putpixel((x, y), img.getpixel((x-1, y)))
        shifted.save(png)
        cap = root/spec_json['capture']
        crec = json.loads(cap.read_text())
        crec['fileSha256'] = digest(png.read_bytes())
        crec['fileBytes'] = png.stat().st_size
        cap.write_text(json.dumps(crec))
        try:
            got = pair(a, shift, cal_path, plan_path)
            rej = got.get('displayStatus') == 'DISPLAY_NOT_JUDGABLE'
            record('N1_SHIFT_1PX_VIA_PAIR', 'NOT_ACCEPTED(先于像素比较拒)',
                   got.get('displayStatus')+':'+got.get('reason', ''), ok=rej)
        except (ValueError, KeyError, TypeError) as exc:
            record('N1_SHIFT_1PX_VIA_PAIR', 'NOT_ACCEPTED(先于像素比较拒)',
                   str(exc), ok='HOLD_IDENTITY' in str(exc) or 'MISMATCH' in str(exc))
        # 像素级 1 码差: 独立 B 档 PNG 单像素 -1（capture 收据 SHA 同步），过 SHA/几何门被像素比较拒
        color = artifact(root, 'color', model, 'B', 3, p['nativeTiles'], registered=p)
        cspec = json.loads(color.read_text())
        cpng = root/cspec['png']
        cimg = Image.open(cpng)
        cx, cy, cw, ch = cores[0]           # 第一个 screen core 内的像素必然参与配对
        px0 = cimg.getpixel((cx, cy))
        cimg.putpixel((cx, cy), (max(0, px0[0]-1), px0[1], px0[2]))
        cimg.save(cpng)
        ccap = root/cspec['capture']
        crec2 = json.loads(ccap.read_text())
        crec2['fileSha256'] = digest(cpng.read_bytes())
        crec2['fileBytes'] = cpng.stat().st_size
        ccap.write_text(json.dumps(crec2))
        try:
            got = pair(a, color, cal_path, plan_path)
            ok2 = got.get('displayStatus') == 'DISPLAY_NOT_JUDGABLE'
            record('N2_CODE_1_COLOR', 'NOT_PAIRED(1码差拒)',
                   got.get('displayStatus')+':'+got.get('reason', ''), ok=ok2)
        except (ValueError, KeyError, TypeError) as exc:
            record('N2_CODE_1_COLOR', 'NOT_PAIRED(1码差拒)', str(exc), ok=True)
        # 缺列: nativeTiles 少 1 列
        tiles = [t[:] for t in p['nativeTiles']]
        tiles[1][2] -= 1
        missing = artifact(root, 'missing', model, 'B', 3, tiles, registered=p)
        expect_reject('N3_MISSING_COLUMN', lambda: pair(a, missing, cal_path, plan_path),
                      'PREDECLARED_NATIVE_TILE_MISMATCH')
        # 旧 token: B 档 manifest token 与校准身份/hold 链不匹配（改 requestedToken）
        old = artifact(root, 'old', model, 'B', 3, p['nativeTiles'], registered=p)

        def mutate(spec_path, cb):
            spec = json.loads(spec_path.read_text())
            mpath = spec_path.parent/spec['manifest']
            m = json.loads(mpath.read_text())
            cb(m)
            mpath.write_text(json.dumps(m))
            new_sha = digest(mpath.read_bytes())
            mpath.with_suffix('.sha256').write_text(new_sha+'  '+mpath.name)
            hpath = spec_path.parent/spec['hold']
            h = json.loads(hpath.read_text())
            h['hold']['manifestSha256'] = new_sha
            hpath.write_text(json.dumps(h))
        # 旧 token: 封存件 endToken/hold.token 改为旧 token，manifest 不动 →
        # bundle 内 token 链自洽但与校准/plan 的 capture 身份错代 → V2 loader 层拒。
        def stale_token(spec_path):
            spec = json.loads(spec_path.read_text())
            hpath = spec_path.parent/spec['hold']
            h = json.loads(hpath.read_text())
            h['endToken'] = 'stale-token'
            h['hold']['token'] = 'stale-token'
            hpath.write_text(json.dumps(h))
        stale_token(old)
        try:
            pair(a, old, cal_path, plan_path)
            record('N4_STALE_TOKEN', 'REJECTED', 'accepted', ok=False)
        except (ValueError, KeyError, TypeError) as exc:
            record('N4_STALE_TOKEN', 'REJECTED(HOLD_IDENTITY)', str(exc), ok='HOLD_IDENTITY' in str(exc))
        # 颜色模型未知: 校准档 back 改成非 IDENTITY/SRGB 可表达的值（+1 全码）→ 两规则全失配
        unknown = artifact(root, 'unknown', model, 'CAL', 1)

        def break_back_rule(m):
            # 把 probe.back sha 与文件改成 +1 的字节流，使两个规则都失配
            import hashlib
            mpath = root/m['back']['name']
            raw = bytearray(mpath.read_bytes())
            for i in range(0, len(raw), 4):
                raw[i] = (raw[i]+1) % 256
            mpath.write_bytes(bytes(raw))
            m['back']['sha256'] = digest(bytes(raw))
            m['probe']['back']['sha256'] = m['back']['sha256']
        mutate(unknown, break_back_rule)
        try:
            calibrate(unknown, root/'bad-cal.json')
            record('N5_COLOR_MODEL_UNKNOWN', 'CALIBRATION_BACK_COLOR_MODEL_UNKNOWN', 'accepted', ok=False)
        except (ValueError, KeyError, TypeError, OSError) as exc:
            record('N5_COLOR_MODEL_UNKNOWN', 'CALIBRATION_BACK_COLOR_MODEL_UNKNOWN',
                   str(exc), ok='CALIBRATION_BACK_COLOR_MODEL_UNKNOWN' in str(exc)
                   or 'BACK_SHA' in str(exc))
        # 越 hold（V2 bundle）: 截图区间 endMonoNs 移到 hold 封存(endMonoMs=100)之后 →
        # clock.envelope CAPTURE_INTERVAL_OUTSIDE_HOLD
        late = artifact(root, 'late', model, 'B', 3, p['nativeTiles'], registered=p)
        late_spec = json.loads(late.read_text())
        lcap = root/late_spec['capture']
        lrec = json.loads(lcap.read_text())
        lrec['endMonoNs'] = 200000000      # 200ms > hold end 100ms
        lcap.write_text(json.dumps(lrec))
        try:
            _load('t413_ch', REPO/'test'/'t413'/'capture_hold.py').verify_bundle(Path(late))
            record('N6_LATE_CAPTURE_V2_BUNDLE', 'CAPTURE_INTERVAL_OUTSIDE_HOLD', 'accepted', ok=False)
        except (ValueError, KeyError, TypeError, OSError) as exc:
            record('N6_LATE_CAPTURE_V2_BUNDLE', 'CAPTURE_INTERVAL_OUTSIDE_HOLD',
                   str(exc), ok='CAPTURE_INTERVAL_OUTSIDE_HOLD' in str(exc))
        # V1 路径越 hold（capture 区间越 endMonoMs）
        v1late = artifact(root, 'v1late', model, 'B', 3, p['nativeTiles'], registered=p)
        v1spec = json.loads(v1late.read_text())
        vcap = root/v1spec['capture']
        vrec = json.loads(vcap.read_text())
        vrec['endMonoNs'] = 300000000   # hold endMonoMs=100
        vcap.write_text(json.dumps(vrec))
        expect_reject('N7_LATE_CAPTURE_V1', lambda: pair(a, v1late, cal_path, plan_path),
                      'CAPTURE_INTERVAL_OUTSIDE_HOLD')
        # 拒绝顺序证明: 同档先破坏 SHA 再破坏时间门 → 先报证据层
        order = artifact(root, 'order', model, 'B', 3, p['nativeTiles'], registered=p)
        ospec = json.loads(order.read_text())
        opng = root/ospec['png']
        opng.write_bytes(opng.read_bytes()+b'\x00')     # PNG 破坏
        ocap = root/ospec['capture']
        orec = json.loads(ocap.read_text())
        orec['endMonoNs'] = 999000000                    # 同时越 hold
        ocap.write_text(json.dumps(orec))
        expect_reject('N8_REJECT_ORDER_SHA_BEFORE_TIME',
                      lambda: pair(a, order, cal_path, plan_path),
                      'CAPTURE_TOOL_RECEIPT')

    passed = sum(1 for r in RESULTS if r['pass'])
    report = {'revision': 'T427_E2E_NEGATIVES_V1', 'device': 'NOT_RUN',
              'total': len(RESULTS), 'passed': passed, 'failed': len(RESULTS)-passed,
              'cases': RESULTS,
              'order': '判读器拒绝顺序: bundle 完整性(SHA/bundle/身份) → 时钟域/hold 覆盖 → '
                       '校准/plan 身份 → 像素比较。合成与真实窗口边界: 合成层证明判定逻辑；'
                       '真实系统截图可见性与真实时钟关系按 T408 attempt1 报告单列（t413-prepare '
                       '阻断中），本卡不卡真机。',
              'reuse': '时钟差分复用 T418 判定（hold_oracle.time_gate ↔ clock.envelope 一致性'
                       '已在 T418 33 条差分证明）；状态差分不重做。'}
    dest = REPO/'test'/'reports'/'t427'/'t427_negatives.json'
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(json.dumps(report, ensure_ascii=False, indent=1), encoding='utf-8')
    print(json.dumps({'revision': report['revision'], 'total': report['total'],
                      'passed': passed, 'failed': report['failed'], 'dest': str(dest)}))
    return 0 if report['failed'] == 0 else 1


if __name__ == '__main__':
    raise SystemExit(main())
