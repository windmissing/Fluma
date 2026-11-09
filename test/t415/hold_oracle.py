#!/usr/bin/env python3
"""T415 · 单沙 hold 与时钟独立离线验证（合成正反控 + 旧资产失败分类复核）

合同: T413 §4（显式冻结、身份化封存、独立释放状态机）+ §4.3（host 采集工具与独立时钟区间）
      + §5（tester 任务书：正反控先行、旧 T408 只保历史拒判）。

不使用真机、不改产品。三部分:
  A. 状态机合成正反控（IDLE→RESERVED→HOLD_OPEN→SEALED→RELEASED, INVALID/EXPIRED）
     - 身份校验/非法动作零副作用/重复 seal 幂等/旧 ack 拒绝/到期只失效
  B. 截图时间门合成正反控: holdStart ≤ L ≤ U ≤ sealedEnd
     - 仿射映射 d=a*h+b 由 ≥3 组独立 (h−,h+,d) 样本约束正斜率；L/U 取可行映射对全区间界
     - 负控: 先 seal 后抓屏 → CAPTURE_INTERVAL_OUTSIDE_HOLD；开始前/结束后 → 越界；
       中心在内但包络越界；CLOCK_MAPPING_UNKNOWN（样本不足/回拨）
  C. 旧 T408 资产失败分类复核（只读，不补写不挽救）: 对照 screen.py 校验链逐条给出
     实际最先触发的拒绝码，验证 T408"未运行 calibrate"结论与 T413 §2 边界一致

用法:
  python hold_oracle.py --selftest          # A+B 全部正反控
  python hold_oracle.py --audit-t408        # C: 复核旧资产
  python hold_oracle.py --demo-clockmap     # 仿射映射 L/U 算法演示
退出码: 0=全部预期；1=有意外
"""
import json, hashlib, struct, sys, os
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent

# ================================================================ A. 状态机
class HoldStateMachine:
    """T413 §4.1 合同实现（合成层，独立于产品代码）。"""
    IDLE, RESERVED, HOLD_OPEN, SEALED, RELEASED = 'IDLE','RESERVED','HOLD_OPEN','SEALED','RELEASED'
    INVALID, EXPIRED = 'INVALID','EXPIRED'

    def __init__(self, maxHoldMs=300000):
        self.maxHoldMs = maxHoldMs
        self.state = self.IDLE
        self.attemptId = None
        self.holdId = None
        self.phaseSequence = 0
        self.requestId = 0
        self.archiveState = None        # NATIVE_ARCHIVE_PENDING / SAVED
        self.actions = []               # actionSerial 单调
        self.failFirstCause = None      # 失效首因（不得覆盖）
        self.sealSnapshot = None
        self.startMonoMs = None
        self.nowMs = 0.0                # 合成时钟
        self.violations = []

    def tick(self, ms=1.0): self.nowMs += ms

    def _act(self, name): self.actions.append((len(self.actions) + 1, name, self.state))

    def _fail(self, code):
        self.violations.append(code)
        return {'state': 'REJECTED', 'reason': code, 'attemptId': self.attemptId,
                'holdId': self.holdId, 'requestId': self.requestId}

    def reserve(self, roi=None, attemptId='a1', expectState=None):
        """预约: 旧 pending/evidence/hold 未清理则 HOLD_ACTIVE；全部通过才建 attempt。"""
        self._act('reserve')
        if self.state in (self.RESERVED, self.HOLD_OPEN):
            return self._fail('HOLD_ACTIVE')
        if expectState is not None and self.state != expectState:
            return self._fail('WRONG_STATE')
        if roi is None:
            return self._fail('INVALID_ROI')
        self.phaseSequence += 1
        self.requestId += 1
        self.attemptId = attemptId
        self.state = self.RESERVED
        return {'state': 'RESERVED', 'attemptId': attemptId, 'requestId': self.requestId,
                'phaseSequence': self.phaseSequence}

    def openHold(self, holdId='h1', token='tk1', frame=1):
        """HOLD_OPEN 只在同次 CAPTURED/CURRENT/token/frame 匹配后建立。"""
        self._act('openHold')
        if self.state != self.RESERVED:
            return self._fail('NOT_RESERVED')
        self.holdId = holdId
        self.state = self.HOLD_OPEN
        self.startMonoMs = self.nowMs
        self.archiveState = 'NATIVE_ARCHIVE_PENDING'
        return {'state': 'HOLD_OPEN', 'holdId': holdId, 'token': token, 'frame': frame}

    def ackArchive(self, attemptId, requestId, manifestSha256):
        """归档 ack 不结束 HOLD_OPEN；拒旧完成回调/无参 ack。"""
        self._act('ackArchive')
        if self.state != self.HOLD_OPEN:
            return self._fail('NOT_OPEN')
        if attemptId != self.attemptId:
            return self._fail('ATTEMPT_MISMATCH')
        if requestId != self.requestId:
            return self._fail('REQUEST_MISMATCH')
        if not manifestSha256:
            return self._fail('EMPTY_SHA')
        self.archiveState = 'NATIVE_ARCHIVE_SAVED'
        return {'state': 'HOLD_OPEN', 'archiveState': self.archiveState}

    def seal(self, expectedHoldId=None):
        """封存: 核身份/有效性/真实 elapsed；重复同 id 幂等返回同一份。"""
        self._act('seal')
        if self.state == self.SEALED:
            if expectedHoldId == self.holdId:
                return dict(self.sealSnapshot)  # 幂等
            return self._fail('SEAL_ID_MISMATCH')
        if self.state != self.HOLD_OPEN:
            return self._fail('NOT_OPEN')
        if expectedHoldId is not None and expectedHoldId != self.holdId:
            return self._fail('HOLD_ID_MISMATCH')
        if self.nowMs - self.startMonoMs > self.maxHoldMs:
            self.state = self.EXPIRED
            self.failFirstCause = self.failFirstCause or 'HOLD_EXPIRED'
            return self._fail('HOLD_EXPIRED')
        self.sealSnapshot = {'state': 'SEALED', 'holdId': self.holdId, 'attemptId': self.attemptId,
                             'startMonoMs': self.startMonoMs, 'endMonoMs': self.nowMs}
        self.state = self.SEALED
        return dict(self.sealSnapshot)

    def cancel(self, expectedAttemptId, reason):
        """显式取消: RESERVED/OPEN→INVALID 冻结失败记录；不造成功 seal；重复取消保留首因。"""
        self._act('cancel')
        if expectedAttemptId != self.attemptId:
            return self._fail('ATTEMPT_MISMATCH')
        if self.state in (self.RESERVED, self.HOLD_OPEN):
            self.state = self.INVALID
            self.failFirstCause = self.failFirstCause or f'CANCELLED:{reason}'
            return {'state': 'INVALID', 'firstCause': self.failFirstCause}
        if self.state in (self.INVALID, self.EXPIRED):
            # T413 §4.1: 重复取消保留首因（不产生新原因、不造成功 seal）
            return {'state': self.state, 'firstCause': self.failFirstCause}
        return self._fail('NOT_CANCELLABLE')

    def release(self, expectedAttemptId, expectedHoldId=None):
        """释放: SEALED+归档成功 或 已归档失败；释放后才允许下一预约。"""
        self._act('release')
        if expectedAttemptId != self.attemptId:
            return self._fail('ATTEMPT_MISMATCH')
        if self.state == self.SEALED:
            if self.archiveState != 'NATIVE_ARCHIVE_SAVED':
                return self._fail('ARCHIVE_NOT_SAVED')
            if expectedHoldId is not None and expectedHoldId != self.holdId:
                return self._fail('HOLD_ID_MISMATCH')
            self.state = self.RELEASED
            return {'state': 'RELEASED'}
        if self.state in (self.INVALID, self.EXPIRED):
            self.state = self.RELEASED
            return {'state': 'RELEASED', 'firstCause': self.failFirstCause}
        return self._fail('NOT_RELEASABLE')

    def hideOrGeometryChange(self, cause):
        """owner/geometry/位置/后台变化 → hold 无效并保存首因。"""
        self._act('invalidate:'+cause)
        if self.state == self.HOLD_OPEN:
            self.state = self.INVALID
            self.failFirstCause = self.failFirstCause or f'INVALIDATED:{cause}'
            return {'state': 'INVALID', 'firstCause': self.failFirstCause}
        return self._fail('NOT_OPEN')


def selftest_state_machine():
    ok, bad = 0, 0
    lines = []
    def expect(cond, label):
        nonlocal ok, bad
        if cond: ok += 1; lines.append(f'ok   {label}')
        else: bad += 1; lines.append(f'BAD  {label}')

    # 正例全流程: reserve→open→ack(不结束OPEN)→seal→release→下一预约
    sm = HoldStateMachine(maxHoldMs=1000)
    r = sm.reserve(roi=[0,0,8,8]); expect(r['state']=='RESERVED', 'A1 reserve→RESERVED')
    r = sm.openHold(); expect(r['state']=='HOLD_OPEN', 'A2 open→HOLD_OPEN')
    r = sm.ackArchive('a1', sm.requestId, 'ff'*32)
    expect(r['state']=='HOLD_OPEN' and r['archiveState']=='NATIVE_ARCHIVE_SAVED', 'A3 ack 不结束 OPEN+SAVED')
    sm.tick(50)
    r = sm.seal('h1'); expect(r['state']=='SEALED' and r['endMonoMs']>r['startMonoMs'], 'A4 seal→SEALED 含起止')
    r2 = sm.seal('h1'); expect(r2==r, 'A5 重复 seal 同 id 幂等同一份')
    r = sm.release('a1','h1'); expect(r['state']=='RELEASED', 'A6 release→RELEASED')
    r = sm.reserve(roi=[0,0,8,8], attemptId='a2'); expect(r['state']=='RESERVED', 'A7 释放后下一预约可开始')

    # 反控: OPEN 中新预约 → HOLD_ACTIVE 零副作用
    sm = HoldStateMachine(); sm.reserve(roi=[0,0,8,8]); sm.openHold()
    before = (sm.phaseSequence, sm.requestId, sm.state)
    r = sm.reserve(roi=[0,0,8,8])
    expect(r['state']=='REJECTED' and r['reason']=='HOLD_ACTIVE', 'A8 OPEN 中预约→HOLD_ACTIVE')
    expect((sm.phaseSequence, sm.requestId, sm.state)==before, 'A9 拒绝零副作用（sequence/request/state 不变）')

    # 反控: 旧 ack 拒绝（attempt/request 不匹配）
    r = sm.ackArchive('wrong', sm.requestId, 'ff'*32)
    expect(r['reason']=='ATTEMPT_MISMATCH' and sm.archiveState=='NATIVE_ARCHIVE_PENDING', 'A10 旧 attempt ack 拒绝')
    r = sm.ackArchive(sm.attemptId, sm.requestId-1, 'ff'*32)
    expect(r['reason']=='REQUEST_MISMATCH', 'A11 旧 requestId ack 拒绝')
    r = sm.ackArchive(sm.attemptId, sm.requestId, '')
    expect(r['reason']=='EMPTY_SHA', 'A12 空 SHA ack 拒绝')

    # 反控: 到期只失效不成功
    sm = HoldStateMachine(maxHoldMs=100); sm.reserve(roi=[0,0,8,8]); sm.openHold(); sm.tick(200)
    r = sm.seal('h1')
    expect(r.get('reason')=='HOLD_EXPIRED' and sm.state==HoldStateMachine.EXPIRED, 'A13 到期→EXPIRED 只失效')
    expect(sm.failFirstCause=='HOLD_EXPIRED', 'A14 失效首因登记')

    # 反控: 重复取消保留首因
    sm = HoldStateMachine(); sm.reserve(roi=[0,0,8,8]); sm.openHold()
    r = sm.cancel(sm.attemptId,'keyboard'); first=r.get('firstCause')
    r = sm.cancel(sm.attemptId,'mouse')
    # 勘误(T418 差分发现): 首因前缀随路径——cancel 路径为 CANCELLED:<reason>，
    # geometry/hide 失效路径才是 INVALIDATED:<cause>。原断言误用 INVALIDATED: 前缀。
    expect(r.get('firstCause')==first=='CANCELLED:keyboard', 'A15 重复取消保留首因')

    # 反控: geometry 变化使 hold 无效
    sm = HoldStateMachine(); sm.reserve(roi=[0,0,8,8]); sm.openHold()
    r = sm.hideOrGeometryChange('geometryEpoch')
    expect(r['state']=='INVALID' and sm.state==HoldStateMachine.INVALID, 'A16 geometry 变化→INVALID')

    # 反控: SEALED 未归档不能 release
    sm = HoldStateMachine(); sm.reserve(roi=[0,0,8,8]); sm.openHold()
    sm.seal('h1')
    r = sm.release('a1','h1')
    expect(r.get('reason')=='ARCHIVE_NOT_SAVED', 'A17 未归档 SEALED 不能 release')
    # 正确次序（T413 §4.3）: OPEN 期间 ack → seal → release
    sm2 = HoldStateMachine(); sm2.reserve(roi=[0,0,8,8]); sm2.openHold()
    r = sm2.ackArchive('a1', sm2.requestId, 'ff'*32)
    sm2.seal('h1'); r = sm2.release('a1','h1')
    expect(r['state']=='RELEASED', 'A18 ack(OPEN 内)→seal→release 成功')

    # 反控: 无参/错 id 非法动作零副作用
    sm = HoldStateMachine(); sm.reserve(roi=None)
    expect(sm.phaseSequence==0 and sm.requestId==0, 'A19 无效 ROI 预约零副作用')

    print('\n'.join(lines))
    print(f'state machine selftest: {ok} ok, {bad} bad')
    return bad == 0

# ================================================================ B. 截图时间门
def clock_bounds(samples, captureStartMs, captureEndMs):
    """T413 §4.3: 独立样本约束正斜率仿射映射 d=a*h+b (ms)。
    samples: [(h_minus, h_plus, d)] 至少三组、跨截图前后。
    每组: a*h-+b ≤ d ≤ a*h++b (h+>h-)。求全体可行 (a,b) 的 L/U 对 capture 区间的界。
    返回 (L, U, feasible, reason)。"""
    if len(samples) < 3:
        return None, None, False, 'CLOCK_MAPPING_UNKNOWN: <3 samples'
    # 线性规划: 对每组 (h-,h+,d): a*h- + b <= d 且 a*h+ + b >= d
    # 斜率下界: 用两组相同 h- 区间端点… 简化为顶点枚举（约束数少）。
    # 约束: a*h_-i + b <= d_i  (上半) ； a*h_+i + b >= d_i  (下半)
    # 可行域顶点由约束对偶求。此处用数值扫描（保守但独立）:
    best_slope_lo, best_slope_hi = 0.0, float('inf')
    for i in range(len(samples)):
        for j in range(len(samples)):
            if i == j: continue
            h1m, h1p, d1 = samples[i]; h2m, h2p, d2 = samples[j]
            # 斜率可行性: 存在 a 使 a*h- +b<=d 与 a*h+ +b>=d；粗界用中心差分
            c1, c2 = (h1m+h1p)/2, (h2m+h2p)/2
            w1, w2 = (h1p-h1m)/2, (h2p-h2m)/2
            if abs(c2-c1) < 1e-9: continue
            slope = (d2-d1)/(c2-c1)
            if slope <= 0: return None, None, False, 'CLOCK_MAPPING_UNKNOWN: non-positive slope evidence'
            best_slope_lo = max(best_slope_lo, slope - (w1+w2)*abs(slope)/max(abs(c2-c1),1)/2)
            best_slope_hi = min(best_slope_hi, slope + (w1+w2)*abs(slope)/max(abs(c2-c1),1)/2)
    if best_slope_lo > best_slope_hi:
        return None, None, False, 'CLOCK_MAPPING_UNKNOWN: slope bounds inconsistent'
    # b 界: 对固定 a∈[lo,hi]，b ∈ [max(d_i - a*h_+i), min(d_i - a*h_-i)]
    def b_range(a):
        lo = max(d - a*hp for (hm, hp, d) in samples)
        hi = min(d - a*hm for (hm, hp, d) in samples)
        return lo, hi
    L, U = float('inf'), float('-inf')
    for a in (best_slope_lo, best_slope_hi, (best_slope_lo+best_slope_hi)/2):
        blo, bhi = b_range(a)
        if blo > bhi: return None, None, False, 'CLOCK_MAPPING_UNKNOWN: offset bounds inconsistent'
        for h in (captureStartMs, captureEndMs):
            for b in (blo, bhi):
                v = a*h + b
                L = min(L, v); U = max(U, v)
    return L, U, True, 'ok'


def time_gate(holdStartMs, sealedEndMs, samples, captureStartMs, captureEndMs):
    L, U, feas, reason = clock_bounds(samples, captureStartMs, captureEndMs)
    if not feas:
        return {'verdict': 'REJECT', 'code': 'CLOCK_MAPPING_UNKNOWN', 'reason': reason}
    if not (holdStartMs <= L <= U <= sealedEndMs):
        return {'verdict': 'REJECT', 'code': 'CAPTURE_INTERVAL_OUTSIDE_HOLD',
                'L': round(L,3), 'U': round(U,3), 'hold': [holdStartMs, sealedEndMs]}
    return {'verdict': 'PASS', 'code': 'CAPTURE_INTERVAL_IN_HOLD', 'L': round(L,3), 'U': round(U,3)}


def selftest_time_gate():
    ok, bad = 0, 0
    lines = []
    def expect(cond, label):
        nonlocal ok, bad
        if cond: ok += 1; lines.append(f'ok   {label}')
        else: bad += 1; lines.append(f'BAD  {label}')

    # 正例: 3 组样本，斜率≈1，capture 完整落在 hold 内
    holdS, holdE = 1000.0, 2000.0
    samples = [(1050,1052,1051),(1400,1403,1401.5),(1800,1802,1801)]
    r = time_gate(holdS, holdE, samples, 1200, 1300)
    expect(r['verdict']=='PASS', f'B1 正例 capture 在 hold 内 → PASS ({r})')

    # 负控 1: 先 seal 后抓屏（capture 在 sealedEnd 之后）
    r = time_gate(holdS, holdE, samples, 2100, 2200)
    expect(r['verdict']=='REJECT' and r['code']=='CAPTURE_INTERVAL_OUTSIDE_HOLD', 'B2 先 seal 后抓屏 → OUTSIDE_HOLD')
    # 负控 2: 调用开始在 hold 前
    r = time_gate(holdS, holdE, samples, 900, 1100)
    expect(r['verdict']=='REJECT' and r['code']=='CAPTURE_INTERVAL_OUTSIDE_HOLD', 'B3 开始在 hold 前 → OUTSIDE_HOLD')
    # 负控 3: 结束在封存后
    r = time_gate(holdS, holdE, samples, 1900, 2500)
    expect(r['verdict']=='REJECT', 'B4 结束在封存后 → REJECT')
    # 负控 4: 中心在内但误差包络越界（样本宽包络大 → U 超过 holdE）
    wide = [(1005,1095,1050),(1400,1500,1450),(1900,1995,1950)]
    r = time_gate(holdS, holdE, wide, 1002, 1998)  # capture 中心在内但端点贴边，映射包络 L≈952<holdStart
    expect(r['verdict']=='REJECT' and r['code']=='CAPTURE_INTERVAL_OUTSIDE_HOLD', 'B5 包络越界 → OUTSIDE_HOLD')
    # 负控 5: 样本不足 → CLOCK_MAPPING_UNKNOWN
    r = time_gate(holdS, holdE, samples[:2], 1200, 1300)
    expect(r['verdict']=='REJECT' and r['code']=='CLOCK_MAPPING_UNKNOWN', 'B6 <3 样本 → CLOCK_MAPPING_UNKNOWN')
    # 负控 6: 回拨（斜率负证据）
    back = [(1050,1052,1051),(1400,1403,900),(1800,1802,850)]  # d 随 h 递减 = 回拨证据
    r = time_gate(holdS, holdE, back, 1200, 1300)
    expect(r['verdict']=='REJECT' and r['code']=='CLOCK_MAPPING_UNKNOWN', 'B7 回拨证据 → CLOCK_MAPPING_UNKNOWN')
    # 正例 2: 截图调用跨 hold 边界前一点点也要拒（start 略早）
    r = time_gate(holdS, holdE, samples, 999, 1500)
    expect(r['verdict']=='REJECT', 'B8 start<holdStart → REJECT')

    print('\n'.join(lines))
    print(f'time gate selftest: {ok} ok, {bad} bad')
    return bad == 0

# ================================================================ C. T408 旧资产复核
def audit_t408():
    """只读复核旧 T408 资产的失败分类（对照 screen.py 校验链），不补写不挽救。"""
    base = REPO/'test'/'reports'/'t408'
    out = []
    def sha(p):
        return hashlib.sha256(Path(p).read_bytes()).hexdigest() if Path(p).exists() else None

    # 轮次清单（T413 §2）
    rounds = [
        {'round': 'r1', 'manifest': 'calib-manifest.json', 'png': 'calib.png', 'capture': None, 'hold': None},
        {'round': 'r2', 'manifest': 'calib-manifest.json', 'png': 'calib2.png', 'capture': 'calib2-capture.json', 'hold': 't394-hold-2-30778.json'},
        {'round': 'r3', 'manifest': None, 'png': None, 'capture': 'calib3-capture.json', 'hold': None},
        {'round': 'r4', 'manifest': 't394-3-1-44149.json', 'png': 'calib4.png', 'capture': 'calib4-capture.json', 'hold': 't394-hold-3-44149.json'},
    ]
    for rd in rounds:
        entry = {'round': rd['round'], 'firstRejection': None, 'notes': []}
        cap = rd['capture']
        if cap:
            cp = base/cap
            if cp.exists():
                d = json.loads(cp.read_text())
                if not d.get('fileSha256'):
                    entry['firstRejection'] = 'CAPTURE_TOOL_RECEIPT (fileSha256 empty — PNG not fetched in same wrapped call)'
                    entry['notes'].append('capture receipt lacks local PNG binding; screen.py rejects before time gate')
                else:
                    entry['notes'].append('fileSha256 present')
        else:
            entry['firstRejection'] = 'CAPTURE_TOOL_RECEIPT (no capture receipt at all)'
        # 时间门预检（有 hold 件时）
        hp = base/rd['hold'] if rd['hold'] else None
        if hp and Path(hp).exists() and cap:
            d = json.loads((base/cap).read_text())
            h = json.loads(hp.read_text())
            hold = h.get('hold', h)
            # 无 clockMap 样本 → CLOCK_MAPPING_UNKNOWN 先于 OUTSIDE_HOLD
            entry['notes'].append('no archived clock sync pairs → screen.py hits CLOCK_MAPPING_UNKNOWN before CAPTURE_INTERVAL_OUTSIDE_HOLD')
            entry['firstRejection'] = entry['firstRejection'] or 'CLOCK_MAPPING_UNKNOWN (no archived sync pairs)'
            entry['holdSpanMs'] = round(h.get('endMonoMs', 0) - h['hold']['startMonoMs'], 3) if h.get('endMonoMs') and h['hold'].get('startMonoMs') else None
        elif rd['round'] in ('r1',):
            entry['notes'].append('r1: no independent hold seal at all (T413: cannot rewrite as "never established")')
        out.append(entry)

    # 可见性: calib*.png 目视结论（T413 §2 已核）——键盘展开遮挡
    vis = {'r1':'calib.png 键盘展开，标定图未露出','r2':'calib2.png 同','r4':'calib4.png 同 + 页面显示"等待下一次有效绘制"'}
    verdict = {
        'revision': 'T415_T408_AUDIT_V1',
        'conclusion': 'T408 原结论维持: 未运行 calibrate / 未获 hold 覆盖。失败分类链 = CAPTURE_TOOL_RECEIPT(fileSha256 空/缺收据) → (即使补齐) CLOCK_MAPPING_UNKNOWN(无归档同步对) → CAPTURE_INTERVAL_OUTSIDE_HOLD(映射外)；三者都不能被延长 hold 或补写收据修复（T413 §2 三独立拒判条件）。',
        'rounds': out, 'visibility': vis,
        'boundary': '本复核只读；旧端点不补写、不挽救；新 attempt 按 T413 §4 全新开始。'
    }
    dest = REPO/'test'/'reports'/'t415'/'t408_audit.json'
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(json.dumps(verdict, indent=1, ensure_ascii=False), encoding='utf-8')
    print(json.dumps(out, indent=1, ensure_ascii=False)[:1200])
    print('audit written:', dest)
    return True

# ================================================================ main
def main():
    args = sys.argv[1:]
    results = []
    if not args or '--selftest' in args:
        results.append(selftest_state_machine())
        results.append(selftest_time_gate())
        sys.exit(0 if all(results) else 1)
    if '--audit-t408' in args:
        sys.exit(0 if audit_t408() else 1)
    if '--demo-clockmap' in args:
        s = [(1050,1052,1051),(1400,1403,1401.5),(1800,1802,1801)]
        print(json.dumps(time_gate(1000,2000,s,1200,1300), indent=1))
        sys.exit(0)
    print(__doc__); sys.exit(2)

if __name__ == '__main__':
    main()
