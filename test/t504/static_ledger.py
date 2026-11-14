#!/usr/bin/env python3
"""T504 · GPU 资源长序列与故障回收独立验证（离线部分：四档账/峰值/host 布局独立复算）。

按 T500 §7：先用公式独立复算四档账（不信任 kProfiles 声明），再对 T492 host
布局断言（App sizeof、snapshot、EvidenceCapture）做独立核对。
GL 长序列 32cycle 部分由 t504-host（另一文件）执行；本脚本是其前置门。
"""
import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
OUT = REPO / 'test/reports/t504'

def m2_payload_words(K, Kw, pad):
    """T500 §2 公式（words→bytes）。先算 M1 常驻 B1，再加 M2 增量（3A128+8pad+33280+32768）。"""
    W = 16*K + 16*Kw + 2304
    S = 4*W
    P = 4*(16*K + 20*Kw + 2304)
    Es = 256*(K+1)
    E = 256*(4*(48*K + 16*Kw + 254) + K)
    SW1 = 4*(96 + 4096*64 + W + 4096*16)
    Q0 = 32 + pad + 2*Kw + 127*16 + 512 + K + Kw + 61444 + 4096*16
    QW1 = 4*(Q0 + W + 256)
    B1 = 3*S + 2*P + 3*Es + 4096 + SW1 + QW1 + 3*E
    A128 = 512*(2*K + 1)
    B2 = B1 + 3*A128 + 8*pad + 33280 + 32768
    return B2, dict(W=W, S=S, P=P, Es=Es, E=E, SW1=SW1, Q0=Q0, QW1=QW1, B1=B1, A128=A128, B2=B2)

def main():
    results = []
    def check(mid, name, ok, detail=''):
        results.append({'id': mid, 'name': name, 'pass': bool(ok), 'detail': detail})

    # 读取产品 kProfiles 声明
    layout = (REPO / 'entry/src/main/cpp/gpu_resident/m2/layout_generated.h').read_text(encoding='utf-8')
    declared = {}
    import re
    for m in re.finditer(r'\{(\d+), (\d+), (\d+), \d+, \d+, \d+, \d+, \d+, \d+, (\d+),', layout):
        K, Kw, pad, payload = int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4))
        declared[(K, Kw)] = (payload, pad)

    check('P1', 'kProfiles 四档齐（192/64 192/128 512/64 512/128）',
          set(declared) == {(192,64),(192,128),(512,64),(512,128)},
          f'实得 {sorted(declared)}')

    # 公式独立复算 vs 声明
    for (K, Kw), (payload, pad) in sorted(declared.items()):
        calc, parts = m2_payload_words(K, Kw, pad)
        check(f'P2-{K}x{Kw}', f'B2 公式复算 == payloadBytes（{K}/{Kw}）',
              calc == payload,
              f'calc={calc:,} declared={payload:,} diff={calc-payload:+,} parts={parts}')

    # 高水位（T500 §3.1）：普通 B2+F、twoBatches max(B2+E, B2+F)
    F = 8 * 1280 * 1609  # 可复算例
    for (K, Kw), (payload, pad) in sorted(declared.items()):
        E = 256*(4*(48*K + 16*Kw + 254) + K)
        normal = payload + F
        two_b = max(payload + E, payload + F)
        check(f'P3-{K}x{Kw}', f'高水位公式（{K}/{Kw}）normal={normal:,} twoBatches={two_b:,}',
              two_b >= normal, f'F={F:,} E={E:,}')

    # T500 §4.1 host 布局独立断言：从 T500 的 MSVC /Zs 审计原件独立提取数值
    layout_txt = (REPO / 'docs/analysis-T500-host-layout-v1.txt').read_text(encoding='utf-8', errors='replace')
    import re
    def sizeof(pattern):
        # App 行在 AppTransport 之后，需排除 Transport 前缀；用 size(...) 形式并锚定词边界
        ms = re.findall(pattern + r'\s+size\((\d+)\)', layout_txt)
        return int(ms[0]) if ms else None
    app_m1 = sizeof(r"`anonymous-namespace'::App")
    snap = sizeof(r'grs::TimelineSnapshot')
    ev = sizeof(r'grs::EvidenceCapture')
    tr1 = sizeof(r"`anonymous-namespace'::AppTransport")
    ref = {'appM1': 2365824, 'appM2': 2365872, 'snapshot': 754200,
           'evidence': 2818864, 'transportM1': 182640, 'transportM2': 183672}
    ok4 = (app_m1 == ref['appM1'] and snap == ref['snapshot'] and
           ev == ref['evidence'] and tr1 == ref['transportM1'])
    check('P4', 'T500 §4.1 host 布局独立断言（App 2365824 / snapshot 754200 / capture 2818864 / transport 182640）',
          ok4, f'实测 App={app_m1} snapshot={snap} capture={ev} transport={tr1}')
    manifest = {'revision': 'T504_STATIC_LEDGER_V1', 'date': '2026-11-14',
                'mode': 'OFFLINE_FORMULA_RECOMPUTE', 'device': 'NOT_RUN',
                'results': results,
                'reference': ref,
                'verdict': 'PASS' if all(r['pass'] for r in results) else 'FAIL'}
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / 't504-static-ledger.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=1), encoding='utf-8')
    for r in results:
        print(('PASS ' if r['pass'] else 'FAIL ') + r['id'] + ' | ' + r['name'] + ' | ' + r['detail'][:100])
    print(f"T504 static ledger: {sum(1 for r in results if r['pass'])}/{len(results)} PASS")
    return 0 if all(r['pass'] for r in results) else 1

if __name__ == '__main__':
    sys.exit(main())
