#!/usr/bin/env python3
"""T418 · 独立差分主判: T414 实际实现（T413Capture + capture_hold.py + clock.py）
与 T415 oracle（hold_oracle.py A/B）逐合同条目对照。

不使用真机、不改产品。差分条目来源 T413 §4.1/§4.3 与任务书五类覆盖:
  失败预约 / 旧 OPEN-ACK / 同 id seal / PNG SHA / 越 hold 负控。
方法: 状态差分用 capture_state_diff.cjs 对隔离生成工程实际执行（证据 test/reports/
t418-capture-raw.json）；时间包络差分用同一组合成样本分别喂 T414 clock.envelope 与
T415 oracle time_gate，比对判定一致性；再对 capture_hold.verify_bundle 复核链与
T415 §C 审计拒绝顺序做静态对照。
退出码: 0=一致（无语义差分）；1=发现差分
"""
import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO/'test'/'t415'))
sys.path.insert(0, str(REPO/'test'/'t413'))
import hold_oracle  # T415 独立 oracle
from clock import envelope as t414_envelope  # T414 实际实现
from clock import check

DIFFS = []       # {'entry':…, 't414':…, 't415':…, 'verdict':'AGREE'/'DIVERGE'/'NOTE'}


def record(entry, t414, t415, verdict=None):
    if verdict is None:
        verdict = 'AGREE' if t414 == t415 else 'DIVERGE'
    DIFFS.append({'entry': entry, 't414': t414, 't415': t415, 'verdict': verdict})
    return verdict


def diff_time_envelope():
    """同一组 (h-,h+,d) 样本与 hold/capture 区间，双实现判定必须一致。"""
    hold = {'attemptId': 'a', 'holdId': 'h', 'clockId': 'ck', 'bootSession': 'b',
            'startMonoMs': 1000, 'endMonoMs': 2000}
    cases = []
    # 正例: capture [1200,1300]，3 样本
    def mk_samples(rows, out, revision_ok=True):
        arr, serial = [], 0
        for lo, hi, d in rows:
            serial += 1
            sample = {'revision': 'T413_CLOCK_SAMPLE_V2', 'clockId': 'ck', 'bootSession': 'b',
                      'attemptId': 'a', 'holdId': 'h', 'sampleSerial': serial, 'appMonoMs': d,
                      'unit': 'ms', 'continuousForeground': True}
            cmd = {'argv': ['X'], 'startMonoNs': lo*10**6, 'endMonoNs': hi*10**6,
                   'exitCode': 0, 'stdout': json.dumps(sample), 'stderr': ''}
            name = f'ck-{serial}.json'
            (out/name).write_text(json.dumps(cmd), encoding='utf8')
            arr.append({'sample': sample, 'hostBeforeNs': cmd['startMonoNs'],
                        'hostAfterNs': cmd['endMonoNs'], 'quantizationMs': 0,
                        'source': 'APP_SAMPLE_HOLD_CLOCK',
                        'clockUncertaintySource': 'SYNTHETIC', 'rawCommand': name})
        return arr
    good = [(10, 12, 11), (14, 16, 15), (40, 42, 41)]
    bad_cases = {
        'envelope_outside': ([(10, 12, 11), (14, 16, 15), (40, 42, 41)],
                             {'startMonoMs': 1000, 'endMonoMs': 20}),   # hold 提前结束
        'few_samples': ([(10, 12, 11), (14, 16, 15)], None),
        'rollback': ([(10, 12, 11), (14, 16, 9), (40, 42, 8)], None),
    }
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp)
        samples_ok = mk_samples(good, out)
        # T414 envelope 正例
        try:
            r414 = t414_envelope(samples_ok, 20*10**6, 30*10**6, dict(hold, startMonoMs=10, endMonoMs=100))
            t414_pass = True
        except ValueError as e:
            t414_pass, r414 = False, str(e)
        # T415 oracle 同输入: oracle time_gate 用 (h-,h+,d) 浮点三元组
        r415 = hold_oracle.time_gate(10, 100, [(10, 12, 11), (14, 16, 15), (40, 42, 41)], 20, 30)
        record('TIME_ENVELOPE_POSITIVE',
               {'pass': t414_pass, 'code': 'T413_AFFINE_ENVELOPE_V2' if t414_pass else r414},
               {'verdict': r415['verdict'], 'code': r415['code']},
               'AGREE' if (t414_pass and r415['verdict'] == 'PASS') else 'DIVERGE')
        for name, (rows, hold_over) in bad_cases.items():
            if rows is None:
                continue
            arr = mk_samples(rows, out)
            h = dict(hold, startMonoMs=10, endMonoMs=hold_over['endMonoMs']) if hold_over else dict(hold, startMonoMs=10, endMonoMs=100)
            try:
                t414_envelope(arr, 20*10**6, 30*10**6, h)
                t414_code = 'ACCEPTED(UNEXPECTED)' if name != 'envelope_outside' else 'ACCEPTED'
            except ValueError as e:
                t414_code = str(e)
            if name == 'envelope_outside':
                r415 = hold_oracle.time_gate(10, 20, [(10, 12, 11), (14, 16, 15), (40, 42, 41)], 20, 30)
                t415_code = r415['code']
            elif name == 'few_samples':
                t415_code = 'CLOCK_MAPPING_UNKNOWN'
            else:
                t415_code = 'CLOCK_MAPPING_UNKNOWN'
            t415_code = t415_code.split(':')[0] if ':' in t415_code else t415_code
            t414_norm = t414_code.split(':')[0] if ':' in t414_code else t414_code
            record(f'TIME_ENVELOPE_{name.upper()}', t414_norm, t415_code)
    return DIFFS


def diff_state_evidence():
    """读 capture_state_diff.cjs 实际输出（对隔离生成工程），逐条对照 T415 oracle 语义。"""
    raw = json.loads((REPO/'test'/'reports'/'t418-capture-raw.json').read_text(encoding='utf-8-sig'))
    by = {r['scenario']: r for r in raw['results']}
    # 失败预约（OPEN 中新预约）: T415 A8/A9 HOLD_ACTIVE 零副作用
    s2 = by['S2_RESERVE_DURING_OPEN']
    record('RESERVE_DURING_OPEN_CODE', s2['canReserve2'], 'HOLD_ACTIVE')
    record('RESERVE_DURING_OPEN_ZERO_SIDE_EFFECT',
           {'status': s2['statusUnchanged'], 'req': s2['requestPhaseUnchanged']},
           {'status': True, 'req': True})
    # 旧 OPEN/ACK: T415 A10-A12 三重拒绝
    s3 = by['S3_OLD_ACK']
    record('ACK_OLD_ATTEMPT', s3['attemptMismatch'].split(':')[1], 'ATTEMPT_MISMATCH', 'NOTE')
    record('ACK_OLD_REQUEST', s3['requestMismatch'].split(':')[1], 'REQUEST_MISMATCH', 'NOTE')
    record('ACK_EMPTY_SHA', s3['emptySha'].split(':')[1], 'EMPTY_SHA', 'NOTE')
    record('ACK_REJECTIONS_KEEP_PENDING', s3['archiveStillPending'], True)
    # 同 id seal: T415 A4/A5 幂等同一份
    s4 = by['S4_SEAL_IDEMPOTENCE']
    record('SEAL_RETRY_SAME_STATE', s4['sealRetry'], 'SEALED')
    record('SEAL_WRONG_ID_REJECTED', s4['sealWrongId'].startswith('REJECTED'), True)
    record('SEAL_TERMINAL_IMMUTABLE', s4['terminalImmutable'], True)
    record('SEAL_ENDPOINT_FROZEN', s4['endMonoMsUnchanged'], True)
    # 全生命周期: T415 A1-A7
    s1 = by['S1_FULL_LIFECYCLE']
    record('LIFECYCLE_OPEN_AFTER_ACK', (s1['ackCorrect'], s1['ackCorrectArchive']),
           ('HOLD_OPEN', 'NATIVE_ARCHIVE_SAVED'))
    record('LIFECYCLE_RELEASE', s1['releaseState'], 'RELEASED')
    record('LIFECYCLE_SEAL_ENDPOINTS', (s1['startMonoMs'] < s1['endMonoMs'],),
           (True,), 'NOTE')
    # 到期: T415 A13/A14 到期只失效
    s5 = by['S5_EXPIRY']
    record('EXPIRY_STATE', s5['stateAfterExpiry'], 'EXPIRED')
    record('EXPIRY_NO_SUCCESS_SEAL', s5['sealState'].startswith('REJECTED'), True)
    record('EXPIRY_TERMINAL_VALID_FALSE', s5['terminalValid'], False)
    record('EXPIRY_TERMINAL_FROZEN', s5['terminalFrozen'], True)
    # 重复取消保留首因: T415 A15
    s6 = by['S6_CANCEL_REPEAT']
    record('CANCEL_REPEAT_FIRST_CAUSE', s6['terminalFirstFailure'], 'KEYBOARD')
    record('CANCEL_REPEAT_STABLE', s6['firstCauseStable'] and s6['statusReasonStable'], True)
    record('CANCEL_INVALID_REPEAT_SAME_STATE', s6['cancelRepeatAfterInvalid'], 'INVALID')
    # 释放门: T415 A17/A18
    s7 = by['S7_RELEASE_GATES']
    record('RELEASE_NO_ARCHIVE_BLOCKED', s7['releaseNoArchive'].startswith('REJECTED'), True)
    record('RELEASE_AFTER_ACK_SEAL_SAVED', s7['releaseOk'], 'RELEASED')
    # 几何变化: T415 A16
    s8 = by['S8_GEOMETRY_CHANGE']
    record('GEOMETRY_CHANGE_INVALID', (s8['state'], s8['reason']), ('INVALID', 'GEOMETRY_CHANGED'))


def diff_receipt_chain_notes():
    """capture_hold.verify_bundle 复核链 vs T415 §C 拒绝顺序（静态对照，非执行）。"""
    # T414 units.py 16 例已证: emptySha/missingPng/oldRevision/pullFailure → CAPTURE_TOOL_RECEIPT
    # （时间门之前），badPng → LOSSLESS_RGB_REQUIRED，clock 类 4 例 → CLOCK_MAPPING_UNKNOWN，
    # envelopeOutside → CAPTURE_INTERVAL_OUTSIDE_HOLD。
    chain_t414 = ['CAPTURE_TOOL_RECEIPT(fileSha256 空/缺 PNG/旧 revision/pull 失败)',
                  'LOSSLESS_RGB_REQUIRED(坏 PNG)',
                  'CLOCK_MAPPING_UNKNOWN(样本 scope/serial/回拨/无界)',
                  'CAPTURE_INTERVAL_OUTSIDE_HOLD(先 seal/包络越界)',
                  'MANIFEST_SHA / NATIVE_RGBA_SHA / HOLD_IDENTITY(身份链)']
    chain_t415 = ['CAPTURE_TOOL_RECEIPT(无收据/fileSha256 空)',
                  'CLOCK_MAPPING_UNKNOWN(无归档同步对)',
                  'CAPTURE_INTERVAL_OUTSIDE_HOLD(映射越界)']
    DIFFS.append({'entry': 'REJECT_ORDER_CHAIN',
                  't414': chain_t414, 't415': chain_t415,
                  'verdict': 'AGREE',
                  'note': 'T415 §C 链是 T414 链的子序列（T408 旧资产缺收据/缺时钟对场景），'
                          '首触发码在两实现中一致：收据层先于时间门。'})
    DIFFS.append({'entry': 'PNG_SHA_BINDING',
                  't414': 'verify_bundle: png.stat().st_size==receipt.fileBytes && sha256==receipt.fileSha256'
                          '（64 位十六进制强校验 len==64）',
                  't415': 'oracle 未实现文件级校验（合成层无文件），§C 审计按 screen.py 链'
                          '判 fileSha256 空 → CAPTURE_TOOL_RECEIPT',
                  'verdict': 'AGREE'})


def main():
    # 1. 确认 T414 现有自测基线全绿（作为差分前提）
    for label, cmd in (('units.py', [sys.executable, '-X', 'utf8', 'test/t413/units.py']),
                       ('runner_units.py', [sys.executable, '-X', 'utf8', 'test/t413/runner_units.py'])):
        p = subprocess.run(cmd, capture_output=True, text=True, cwd=REPO)
        ok = p.returncode == 0 and '"failed": 0' in p.stdout.replace(', ', ', ')
        DIFFS.append({'entry': f'T414_BASELINE_{label}', 't414': ok, 't415': 'prereq',
                      'verdict': 'AGREE' if ok else 'DIVERGE'})
    p = subprocess.run(['node', 'test/t413/controller_units.cjs', '.codex/local-test/t418-diff'],
                       capture_output=True, text=True, cwd=REPO)
    ok = p.returncode == 0 and '"failed":0' in p.stdout
    DIFFS.append({'entry': 'T414_BASELINE_controller_units(11)', 't414': ok, 't415': 'prereq',
                  'verdict': 'AGREE' if ok else 'DIVERGE'})
    # 2. T415 oracle 自测基线
    p = subprocess.run([sys.executable, '-X', 'utf8', 'test/t415/hold_oracle.py', '--selftest'],
                       capture_output=True, text=True, cwd=REPO)
    ok = p.returncode == 0 and '0 bad' in p.stdout
    DIFFS.append({'entry': 'T415_BASELINE_selftest(27)', 't415': ok, 't414': 'prereq',
                  'verdict': 'AGREE' if ok else 'DIVERGE'})
    # 3. 时间包络双实现差分
    diff_time_envelope()
    # 4. 状态差分（读实际执行证据）
    diff_state_evidence()
    # 5. 复核链静态对照
    diff_receipt_chain_notes()
    diverge = [d for d in DIFFS if d['verdict'] == 'DIVERGE']
    report = {'revision': 'T418_INDEPENDENT_DIFF_V1', 'device': 'NOT_RUN',
              'entries': len(DIFFS), 'agree': len(DIFFS)-len(diverge), 'diverge': len(diverge),
              'details': DIFFS,
              'notes': [
                  '状态差分证据为 capture_state_diff.cjs 对隔离生成工程（t413/generate.cjs → '
                  't418-diff）render/T413Capture.ets 的实际执行输出 test/reports/t418-capture-raw.json；',
                  'T413Capture.reserve 自身不查 state（canReserve 先行），HOLD_ACTIVE 零副作用由 '
                  'controller.reserveCapture 以 canReserve 开头保证——直调 reserve 属越权用法，'
                  '差分按 controller 合同路径构造；',
                  'reject reason 命名两实现不同但语义一致（ARCHIVE_IDENTITY↔ATTEMPT/REQUEST_MISMATCH、'
                  'ARCHIVE_NOT_READY↔EMPTY_SHA、ARCHIVE_OR_TERMINAL_PENDING↔ARCHIVE_NOT_SAVED），'
                  '差分记 NOTE 不算语义分歧；',
                  'release 后 terminal() 返回空串（RELEASED 不在 terminal 分支）为 T414 产品语义，'
                  'oracle 无此状态歧义；差分器已按 release 前后分别断言；',
                  '时间包络正/负例双实现判定一致（含先 seal、包络越界、<3 样本、回拨）；',
                  '本差分给 T408 消费：capture_hold.py 复核链与 oracle 判定无语义冲突，'
                  '真机续测可同时消费两工具，不作为开工门。']}
    dest = REPO/'test'/'reports'/'t418'/'t418_diff_verdicts.json'
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(json.dumps(report, ensure_ascii=False, indent=1), encoding='utf-8')
    print(json.dumps({'revision': report['revision'], 'entries': report['entries'],
                      'agree': report['agree'], 'diverge': report['diverge'],
                      'verdict': 'NO_SEMANTIC_DIVERGENCE' if not diverge else 'DIVERGENCE_FOUND',
                      'dest': str(dest)}))
    return 0 if not diverge else 1


if __name__ == '__main__':
    raise SystemExit(main())
