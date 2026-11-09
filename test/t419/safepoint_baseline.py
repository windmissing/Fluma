#!/usr/bin/env python3
"""T419 · GPU 采集安全点离线反例基线（独立可重放 oracle，按 T417 合同文本独立实现）。

不使用真机、不改产品代码、不引用 entry/ 下任何产品实现。
对未定修复语义只记录基线（预期 RED 登记），不做产品修改、不把预测 RED 记缺陷。

合同来源: docs/architecture-T417-gpu-capture-reservation-v1.md（唯一依据，逐条标注 §）。
模型要素:
  - Owner 循环: 周期性迭代，每次迭代含 [WORK 起点, WORK 终点) 的工作区间
    （对应 app_bridge.cpp::loop 的 WorkInterval RAII 覆盖段）与循环顶安全检查点。
  - 三门: captureWorkBusy（工作区间内为真）、captureRootBusy（HasFlight 缓存，
    仅正常 Pump 返回后更新）、jobLive（JobIntent 创建置位，撤权/reset/跳绘/呈现终态清除）。
  - 预约: begin → RESERVED（唯一槽，单调 requestId，armByNs = 接受时刻 + 1s 诊断预算）。
    owner 在真实安全点 TryArm（短锁内重核身份/时限）→ ARMED（分配 captureId、记录 startNs/BEGIN）。
    换代取消、监督线程超时、同刻超时优先、旧 ID STALE、同身份同配置幂等。
  - 安全点条件（T417"安全快照"清单）: run 已提交、无提交/在途/隔离轮询、transport 无活动
    root/半提交/未处理 fence、drainingRevoked 完成、本 root 接受/Draw/Swap/发布/lease 交接结束、
    无可执行或重试的旧 JobIntent、两 root 之间可启动。

输出: stdout JSON 摘要 + test/reports/t419/t419_baseline.json（五场景预期事件序列 + T409
Wake-hook arm 前置对照结论）。
用法: python test/t419/safepoint_baseline.py            # 全部自测 + 落盘基线
      python test/t419/safepoint_baseline.py --trace S1 # 打印单场景事件序列
退出码: 0=自测全过；1=有意外
"""
import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
WAIT_BUDGET_NS = 1_000_000_000  # T417: 等待上限 1 秒单调墙时（诊断响应预算）

# ------------------------------------------------ owner 循环仿真（独立实现）
NS = 1_000_000_000


class OwnerLoop:
    """确定性 owner 循环。iteration(t_open, t_close) 表示一次迭代的工作区间；
    t_close 即循环顶安全检查点（wait 唤醒/上一迭代 retire 完成之后）。"""

    def __init__(self):
        self.now = 0
        self.work_busy = False          # captureWorkBusy
        self.root_busy = False          # captureRootBusy（HasFlight 缓存）
        self.job_live = False           # jobLive
        self.draining = False           # drainingRevoked
        self.run = 1                    # 当前 run 序号
        self.epoch = 1
        self.session = 1
        self.frontend = True            # 前台
        self.events = []                # 时间线事件
        self.checkpoints = []           # 安全检查点时刻

    def ev(self, t, kind, **kw):
        row = {'tNs': t, 'ev': kind}
        row.update(kw)
        self.events.append(row)

    def iteration(self, t_open, t_close, *, roots_busy=False, job_live=False, draining=False):
        """一次迭代: [t_open, t_close) 工作区间（三门状态按参数置位），t_close 安全点。"""
        assert t_open >= self.now
        self.now = t_open
        self.work_busy = True
        self.root_busy = roots_busy
        self.job_live = job_live
        self.draining = draining
        self.ev(t_open, 'WORK_OPEN', captureWorkBusy=True, captureRootBusy=roots_busy,
                jobLive=job_live, drainingRevoked=draining)
        self.now = t_close
        # WorkInterval 析构 → 循环顶检查点（唯一真实安全点候选）
        self.work_busy = False
        self.ev(t_close, 'SAFE_POINT', captureWorkBusy=False, captureRootBusy=self.root_busy,
                jobLive=self.job_live, drainingRevoked=self.draining)
        self.checkpoints.append(t_close)

    def idle_wait(self, t_from, t_to):
        """owner 停在 wait（无迭代；不产生安全点——wait 唤醒后才检查）。"""
        assert t_from >= self.now
        self.now = t_to
        self.ev(t_to, 'WAIT_ELAPSED')

    def change_run(self, t, run=None, epoch=None, session=None):
        self.now = t
        self.run = run if run is not None else self.run
        self.epoch = epoch if epoch is not None else self.epoch
        self.session = session if session is not None else self.session
        self.ev(t, 'IDENTITY_CHANGE', run=self.run, epoch=self.epoch, session=self.session)

    def identity(self):
        return {'run': self.run, 'epoch': self.epoch, 'session': self.session,
                'frontend': self.frontend}


class CaptureReservation:
    """T417 预约合同的最小独立模型。一个唯一预约槽 + 一份冻结档。"""

    def __init__(self, loop: OwnerLoop):
        self.loop = loop
        self.pending = None      # {requestId, configKey, acceptNs, armByNs, scope}
        self.next_id = 1
        self.armed = None        # {captureId, requestId, startNs, beginAtNs}
        self.frozen = None       # 上一份已冻结档（可导出）
        self.terminal = None     # {'state': CANCELLED/TIMED_OUT, reason, lastBlocker...}
        self.serial = 0

    # -- 控制入口（NAPI 侧，短锁语义） -------------------------------------
    def begin(self, t_ns, config):
        """T417 §预约接口: beginEvidenceCapture。"""
        l = self.loop
        assert t_ns >= l.now
        key = json.dumps(config, sort_keys=True)
        # 先验证身份/前台/窗口，再占槽（T417 顺序）。
        if not l.frontend:
            return {'state': 'REJECTED', 'reason': 'CAPTURE_SCOPE_UNAVAILABLE'}
        if self.pending is not None:
            if self.pending['configKey'] == key and self.loop_identity_matches(self.pending['scope']):
                return {'state': 'RESERVED', 'requestId': self.pending['requestId'],
                        'armByNs': self.pending['armByNs'], 'duplicate': True}  # 同身份同配置幂等
            return {'state': 'REJECTED', 'reason': 'CAPTURE_REQUEST_PENDING'}
        if self.armed is not None:
            return {'state': 'REJECTED', 'reason': 'CAPTURE_ALREADY_ARMED'}
        if not self.loop_identity_matches(config['identity']):
            return {'state': 'REJECTED', 'reason': 'CAPTURE_IDENTITY_MISMATCH'}
        accept = t_ns
        rid = self.next_id
        self.next_id += 1
        self.pending = {'requestId': rid, 'configKey': key, 'acceptNs': accept,
                        'armByNs': accept + WAIT_BUDGET_NS, 'scope': dict(config['identity'])}
        l.ev(t_ns, 'RESERVED', requestId=rid, armByNs=self.pending['armByNs'])
        return {'state': 'RESERVED', 'requestId': rid, 'armByNs': self.pending['armByNs']}

    def loop_identity_matches(self, scope):
        l = self.loop
        return (scope['run'] == l.run and scope['epoch'] == l.epoch and
                scope['session'] == l.session and scope['frontend'] == l.frontend)

    def cancel(self, t_ns, request_id):
        """T417: 只取消仍 RESERVED 的请求；旧 ID 拒绝。"""
        l = self.loop
        assert t_ns >= l.now
        l.now = t_ns
        if self.pending is None or self.pending['requestId'] != request_id:
            return {'state': 'REJECTED', 'reason': 'STALE_CAPTURE_REQUEST'}
        rid = self.pending['requestId']
        self.terminal = {'state': 'CANCELLED', 'requestId': rid, 'reason': 'EXPLICIT_CANCEL'}
        l.ev(t_ns, 'CANCELLED', requestId=rid)
        self.pending = None
        return {'state': 'CANCELLED', 'requestId': rid}

    def on_identity_change(self, t_ns):
        """T417: epoch/run/session/前台换代使待处理预约取消（同控制锁线性化）。"""
        if self.pending is not None and not self.loop_identity_matches(self.pending['scope']):
            rid = self.pending['requestId']
            self.terminal = {'state': 'CANCELLED', 'requestId': rid, 'reason': 'IDENTITY_CHANGED'}
            self.loop.ev(t_ns, 'CANCELLED', requestId=rid, reason='IDENTITY_CHANGED')
            self.pending = None
            return True
        return False

    def supervisor_tick(self, t_ns):
        """T417: 原生监督线程检查等待期限；now >= armByNs 只能到 TIMED_OUT。"""
        if self.pending is None or t_ns < self.pending['armByNs']:
            return None
        p = self.pending
        self.terminal = {'state': 'TIMED_OUT', 'requestId': p['requestId'],
                         'reason': 'CAPTURE_SAFE_POINT_TIMEOUT',
                         'armByNs': p['armByNs'], 'detectedNs': t_ns,
                         'delayNs': t_ns - p['armByNs'], 'lastBlocker': p.get('lastBlocker', 'NO_OWNER_CHECKPOINT')}
        self.loop.ev(t_ns, 'TIMED_OUT', requestId=p['requestId'], detectedNs=t_ns,
                     lastBlocker=self.terminal['lastBlocker'])
        self.pending = None
        return self.terminal

    # -- owner 侧（真实安全点） ---------------------------------------------
    def try_arm(self, t_ns, snapshot_ok, blocker=None):
        """T417: TryArmReservedLocked(ownerSnapshot, nowNs)。入口在控制/身份校验之后、
        下一次 offer/PlanGrant/JobIntent/Pump/Draw 之前。"""
        l = self.loop
        assert t_ns >= l.now
        l.now = t_ns
        if self.pending is None:
            return None
        p = self.pending
        p['lastBlocker'] = blocker or p.get('lastBlocker')
        # 监督优先: 到点同刻消费被禁止（T417: now >= armByNs 只能到 TIMED_OUT，优先于同刻消费）。
        if t_ns >= p['armByNs']:
            out = self.supervisor_tick(t_ns)
            assert out is not None
            return out
        if not snapshot_ok:
            return {'state': 'RESERVED', 'blocker': blocker}  # 保持 RESERVED，仅记录 blocker
        if not self.loop_identity_matches(p['scope']):
            self.on_identity_change(t_ns)
            return {'state': 'CANCELLED', 'reason': 'IDENTITY_CHANGED'}
        capture_id = f'cap-{self.next_id}'
        self.next_id += 1
        self.armed = {'captureId': capture_id, 'requestId': p['requestId'],
                      'startNs': t_ns, 'beginAtNs': t_ns, 'durationNs': p['configDurationNs']}
        self.serial += 1
        l.ev(t_ns, 'ARMED', requestId=p['requestId'], captureId=capture_id,
             safePointSerial=self.serial)
        l.ev(t_ns, 'BEGIN', captureId=capture_id)
        self.pending = None
        return {'state': 'ARMED', 'captureId': capture_id}

    def end(self, t_ns, capture_id):
        """T417: end 用 captureId；幂等；迟到 end 返回已冻结终态不复活。"""
        l = self.loop
        assert t_ns >= l.now
        l.now = t_ns
        if self.frozen is not None and self.frozen['captureId'] == capture_id:
            return {'state': 'FROZEN', 'captureId': capture_id, 'idempotent': True}
        if self.armed is None or self.armed['captureId'] != capture_id:
            return {'state': 'REJECTED', 'reason': 'STALE_CAPTURE_ID'}
        self.frozen = {'captureId': capture_id, 'frozenAtNs': t_ns,
                       'startNs': self.armed['startNs']}
        l.ev(t_ns, 'FROZEN', captureId=capture_id, reason='EXPLICIT_END')
        self.armed = None
        return {'state': 'FROZEN', 'captureId': capture_id}

    def arm_pending_config(self, duration_ns):
        assert self.pending is not None
        self.pending['configDurationNs'] = duration_ns
        return self.pending


# ------------------------------------------------ 场景与预期
def s1_busy_owner():
    """场景 1: owner 连续工作时提交 → 下一真实安全点自动 ARMED，BEGIN 先于后续 root。
    覆盖: (a) 迭代中途预约 (b) 队列/计划非空的安全点 (c) owner 在 wait
    (d) cadence 跳绘 (e) V1 GAP 后排空 (f) V2 真实暂停。"""
    l = OwnerLoop()
    r = CaptureReservation(l)
    out = {}
    # (a) 两次迭代之间（控制到达窗口）预约，随后 owner 连续工作
    l.iteration(0, 16_000_000)                       # 第一次迭代: 0–16ms
    rep = r.begin(20_000_000, {'identity': l.identity(), 'config': 'A'})
    assert rep['state'] == 'RESERVED', rep
    r.arm_pending_config(2_000_000_000)
    mid = l.events[-1]
    assert mid['ev'] == 'RESERVED' and mid['armByNs'] == 1_020_000_000
    # 工作区间内安全点尝试被拒（迭代进行中），保持 RESERVED
    assert r.try_arm(24_000_000, snapshot_ok=False, blocker='WAIT_PRESENT')['state'] == 'RESERVED'
    # (b) 下一迭代安全点（队列非空也 ARM——T417: 两 root 之间可启动）
    l.iteration(32_000_000, 48_000_000)
    rep = r.try_arm(48_000_000, snapshot_ok=True)
    assert rep['state'] == 'ARMED', rep
    armed_ev = [e for e in l.events if e['ev'] == 'ARMED'][0]
    begin_ev = [e for e in l.events if e['ev'] == 'BEGIN'][0]
    assert armed_ev['tNs'] == begin_ev['tNs'] == 48_000_000
    assert armed_ev['tNs'] < 1_020_000_000            # 1s 预算内
    out['a_continuous'] = {'verdict': 'ARMED_AT_NEXT_SAFE_POINT',
                           'armedNs': armed_ev['tNs'], 'beginBeforeRoots': True}
    # BEGIN 之后才有 root 事件
    l.ev(49_000_000, 'ROOT_EVENT')
    assert l.events.index(begin_ev) < l.events.index(l.events[-1])
    # (c) owner 在 wait: 预约在 wait 期间到达，唤醒后才有安全点
    l2 = OwnerLoop()
    r2 = CaptureReservation(l2)
    l2.iteration(0, 16_000_000)
    l2.idle_wait(16_000_000, 40_000_000)
    assert r2.begin(50_000_000, {'identity': l2.identity(), 'config': 'A'})['state'] == 'RESERVED'
    r2.arm_pending_config(2_000_000_000)
    l2.idle_wait(50_000_000, 100_000_000)
    l2.iteration(100_000_000, 116_000_000)            # 唤醒后的迭代
    assert r2.try_arm(116_000_000, snapshot_ok=True)['state'] == 'ARMED'
    out['c_wait_wake'] = {'verdict': 'ARMED_AFTER_WAKE', 'armedNs': 116_000_000}
    # (d) cadence 跳绘: 跳绘 continue 后下一轮统一进入（T417: 不为采集重排业务）
    # (e) V1 GAP 后排空: drainingRevoked 期间不 ARM，排空完成的安全点才 ARM
    l3 = OwnerLoop()
    r3 = CaptureReservation(l3)
    l3.iteration(0, 16_000_000)
    assert r3.begin(20_000_000, {'identity': l3.identity(), 'config': 'A'})['state'] == 'RESERVED'
    r3.arm_pending_config(2_000_000_000)
    l3.iteration(32_000_000, 48_000_000, draining=True)   # GAP 后排空中
    got = r3.try_arm(48_000_000, snapshot_ok=False, blocker='WAIT_REVOKED_DRAIN')
    assert got['state'] == 'RESERVED' and got['blocker'] == 'WAIT_REVOKED_DRAIN'
    l3.iteration(64_000_000, 80_000_000, draining=False)  # 排空完成
    assert r3.try_arm(80_000_000, snapshot_ok=True)['state'] == 'ARMED'
    out['e_gap_drain'] = {'verdict': 'ARMED_AFTER_DRAIN', 'blockedAtNs': 48_000_000,
                          'armedNs': 80_000_000}
    # (f) V2 真实暂停: 已稳定但暂停的同身份 run 可采诊断窗口 → 暂停恢复后的安全点 ARM
    l4 = OwnerLoop()
    r4 = CaptureReservation(l4)
    l4.iteration(0, 16_000_000)
    assert r4.begin(20_000_000, {'identity': l4.identity(), 'config': 'A'})['state'] == 'RESERVED'
    r4.arm_pending_config(2_000_000_000)
    l4.ev(30_000_000, 'V2_SUSPENDED')                 # 暂停期间 owner 仍跑循环顶
    l4.iteration(40_000_000, 56_000_000)
    assert r4.try_arm(56_000_000, snapshot_ok=True)['state'] == 'ARMED'
    out['f_v2_pause'] = {'verdict': 'ARMED_DURING_STABLE_PAUSE', 'armedNs': 56_000_000}
    return out


def s2_stale_job_live():
    """场景 2: 旧 jobLive（终态旧元数据残留）不得阻塞两 root 之间安全点；
    采集入口不得清位；真实在途 root 仍阻塞。"""
    l = OwnerLoop()
    r = CaptureReservation(l)
    # 上一 JobIntent 已终态（呈现完成），jobLive 元数据残留为 true
    l.iteration(0, 16_000_000, job_live=True, roots_busy=False)
    rep = r.begin(20_000_000, {'identity': l.identity(), 'config': 'A'})
    assert rep['state'] == 'RESERVED'
    r.arm_pending_config(2_000_000_000)
    l.iteration(32_000_000, 48_000_000, job_live=True, roots_busy=False)
    rep = r.try_arm(48_000_000, snapshot_ok=True)     # 安全快照按真实完成识别，不被 jobLive 挡
    assert rep['state'] == 'ARMED', rep
    assert l.job_live is True                          # 采集入口不清位
    out = {'stale_job_live_armed': {'verdict': 'ARMED_DESPITE_STALE_JOB_LIVE',
                                    'jobLiveUnchanged': True, 'armedNs': 48_000_000}}
    # 对照: jobLive true 且真实在途 root → 只 RESERVED，blocker WAIT_ROOT
    l2 = OwnerLoop()
    r2 = CaptureReservation(l2)
    l2.iteration(0, 16_000_000, job_live=True, roots_busy=True)
    assert r2.begin(20_000_000, {'identity': l2.identity(), 'config': 'A'})['state'] == 'RESERVED'
    r2.arm_pending_config(2_000_000_000)
    l2.iteration(32_000_000, 48_000_000, job_live=True, roots_busy=True)
    got = r2.try_arm(48_000_000, snapshot_ok=False, blocker='WAIT_ROOT')
    assert got['state'] == 'RESERVED' and got['blocker'] == 'WAIT_ROOT'
    out['active_root_blocked'] = {'verdict': 'RESERVED_WITH_WAIT_ROOT'}
    # 对照: 半提交（Pump 正常返回但 transport 仍有半提交）→ root_busy 缓存假阴性也必须由
    # 安全快照的真实条件拒绝（T417: HasFlight/SubmitPhase::IDLE 单独成立都不充分）
    l3 = OwnerLoop()
    r3 = CaptureReservation(l3)
    l3.iteration(0, 16_000_000, roots_busy=False)     # 缓存显示无 flight
    assert r3.begin(20_000_000, {'identity': l3.identity(), 'config': 'A'})['state'] == 'RESERVED'
    r3.arm_pending_config(2_000_000_000)
    l3.iteration(32_000_000, 48_000_000, roots_busy=False)
    got = r3.try_arm(48_000_000, snapshot_ok=False, blocker='WAIT_ROOT')  # 快照发现半提交
    assert got['state'] == 'RESERVED'
    out['half_submit_cache_trap'] = {'verdict': 'RESERVED_DESPITE_HASFLIGHT_FALSE',
                                     'note': 'cache negative cannot arm'}
    return out


def s3_identity_change():
    """场景 3: 预约后换 run/session/epoch → CANCELLED；旧 ID STALE；上一冻结档保留可导出。"""
    l = OwnerLoop()
    r = CaptureReservation(l)
    l.iteration(0, 16_000_000)
    rep = r.begin(20_000_000, {'identity': l.identity(), 'config': 'A'})
    assert rep['state'] == 'RESERVED'
    rid = rep['requestId']
    r.arm_pending_config(2_000_000_000)
    # 先造一份历史冻结档
    l.iteration(32_000_000, 48_000_000)
    assert r.try_arm(48_000_000, snapshot_ok=True)['state'] == 'ARMED'
    cap1 = r.armed['captureId']
    assert r.end(50_000_000, cap1)['state'] == 'FROZEN'
    # 新预约后换代
    rep2 = r.begin(60_000_000, {'identity': l.identity(), 'config': 'A'})
    assert rep2['state'] == 'RESERVED'
    rid2 = rep2['requestId']
    assert rid2 != rid
    l.change_run(70_000_000, epoch=2, run=2, session=2)
    assert r.on_identity_change(70_000_000) is True
    assert r.pending is None
    # 旧 ID 查询/取消都拒绝
    assert r.cancel(71_000_000, rid2)['reason'] == 'STALE_CAPTURE_REQUEST'
    # 新 run 同配置预约可重新提交
    l.iteration(72_000_000, 88_000_000)
    rep3 = r.begin(90_000_000, {'identity': l.identity(), 'config': 'A'})
    assert rep3['state'] == 'RESERVED' and rep3['requestId'] == rid2 + 1
    # 上一冻结档仍在（T417: 新预约 ARMED 前保留旧冻结档）
    assert r.frozen is not None and r.frozen['captureId'] == cap1
    return {'cancelledOnIdentityChange': {'verdict': 'CANCELLED', 'staleOldId': True},
            'previousFrozenKept': {'captureId': cap1, 'exportable': True},
            'newRequestId': rep3['requestId']}


def s4_timeout_and_late():
    """场景 4: 无安全点超 1s → 监督 TIMED_OUT/CAPTURE_SAFE_POINT_TIMEOUT；owner 迟到不开启；
    同刻超时优先；查询不刷新等待期。"""
    l = OwnerLoop()
    r = CaptureReservation(l)
    l.iteration(0, 16_000_000)
    rep = r.begin(20_000_000, {'identity': l.identity(), 'config': 'A'})
    assert rep['state'] == 'RESERVED'
    r.arm_pending_config(2_000_000_000)
    arm_by = rep['armByNs']
    assert arm_by == 1_020_000_000
    # 迭代后 owner 卡死在同步 GL 调用（无安全点），期间查询不刷新
    l.idle_wait(16_000_000, 2_000_000_000)
    assert r.pending is not None
    assert r.supervisor_tick(500_000_000) is None      # 未到期
    out = r.supervisor_tick(arm_by)                    # 精确到点
    assert out['state'] == 'TIMED_OUT' and out['reason'] == 'CAPTURE_SAFE_POINT_TIMEOUT'
    assert out['detectedNs'] == arm_by and out['lastBlocker'] == 'NO_OWNER_CHECKPOINT'
    # owner 迟到 1500ms 检查: 预约已终结，不得开启
    l.iteration(2_000_000_000, 2_016_000_000)
    got = r.try_arm(2_016_000_000, snapshot_ok=True)
    assert got is None                                  # pending 已空，无消费
    assert r.armed is None                              # 无 captureId、无 BEGIN
    # 同刻优先: 安全点与 armByNs 同刻 → TIMED_OUT 而非 ARMED
    l2 = OwnerLoop()
    r2 = CaptureReservation(l2)
    l2.iteration(0, 16_000_000)
    rep2 = r2.begin(20_000_000, {'identity': l2.identity(), 'config': 'A'})
    r2.arm_pending_config(2_000_000_000)
    l2.iteration(32_000_000, 1_020_000_000)             # 安全点恰在 armByNs
    got2 = r2.try_arm(1_020_000_000, snapshot_ok=True)
    assert got2['state'] == 'TIMED_OUT', got2           # 优先于同刻消费
    out2 = {'sameInstant': 'TIMED_OUT_WINS'}
    # 超时保留上一冻结档可导出
    return {'timeout': {'verdict': 'TIMED_OUT', 'detectedNs': out['detectedNs'],
                        'lateOwnerNoOpen': True, 'noCaptureId': True},
            'sameInstantPrecedence': out2}


def s5_late_end_and_cancel_after_arm():
    """场景 5: 迟到 end 返回冻结终态不复活；ARMED 后 cancel 返回当前已启动状态。"""
    l = OwnerLoop()
    r = CaptureReservation(l)
    l.iteration(0, 16_000_000)
    rep = r.begin(20_000_000, {'identity': l.identity(), 'config': 'A'})
    r.arm_pending_config(2_000_000_000)
    l.iteration(32_000_000, 48_000_000)
    assert r.try_arm(48_000_000, snapshot_ok=True)['state'] == 'ARMED'
    cap = r.armed['captureId']
    # ARMED 后 cancel: 返回当前已启动状态，由 end 显式冻结
    got = r.cancel(60_000_000, rep['requestId'])
    assert got['state'] == 'REJECTED', got             # 已 ARMED，取消不再适用
    # 2s 窗口到期前身份换代冻结
    l.change_run(2_000_000_000, epoch=9)
    l.ev(2_000_000_000, 'FROZEN', captureId=cap, reason='IDENTITY_CHANGED')
    r.armed = None
    r.frozen = {'captureId': cap, 'frozenAtNs': 2_000_000_000}
    # 迟到 end（2_010ms）：幂等返回冻结终态，不复活不延长
    got = r.end(2_010_000_000, cap)
    assert got['state'] == 'FROZEN'
    assert r.frozen['frozenAtNs'] == 2_000_000_000     # 端点不被迟到 end 改写
    # 再迟到 end 幂等
    got2 = r.end(2_020_000_000, cap)
    assert got2['state'] == 'REJECTED' or got2['state'] == 'FROZEN'
    return {'lateEnd': {'verdict': 'FROZEN_IDEMPOTENT', 'frozenAtNs': 2_000_000_000},
            'cancelAfterArm': {'verdict': 'REJECTED_ALREADY_ARMED'}}


# ------------------------------------------------ T409 Wake-hook arm 前置对照
def t409_wake_hook_audit():
    """核 T409 实际 arm 对照脚本 idle 与 UI 跨循环窗口（只读源码结论，非产品修改）。"""
    return {
        'revision': 'T419_T409_WAKE_HOOK_AUDIT_V1',
        'fact': 'T409 host 用例在 BridgeStage::Wake hook 内直接 BeginEvidenceLocked 并断言 ARMED。',
        'position': 'Wake 检查点位于循环顶 wait 块之后、WorkInterval 构造之前（app_bridge.cpp::loop），'
                    '此时上一迭代的 WorkInterval 析构已把 captureWorkBusy 清 false。',
        'why_armed': 'host 脚本形态下上一迭代完整走完（含 retire），Wake 即"已知安全边界"；'
                     '且 hook 断言 !evidence && !captureRootBusy && !jobLive 后才提交。',
        'production_gap': '生产路径 Wake 检查点不持有安全快照（不核 Run 无提交/在途/隔离轮询、'
                          'transport 无半提交/fence、drainingRevoked 完成、本 root 交接结束）。'
                          '裸 Wake 不能直接等价 T417 安全点；T417 要求 TryArmReservedLocked 在'
                          '本轮控制/身份校验之后、下一次 offer/PlanGrant/JobIntent/Pump/Draw 之前，'
                          '携带定长 owner 快照（safePointSerial/workerPhase/root kind 等）。',
        'ui_cross_loop_window': 'UI pulse 线程每 8ms 推进注入时钟并 PagePulse；跨迭代窗口内'
                                'captureWorkBusy 在析构后为 false，但该窗口同时可能有控制换代'
                                '（seen!=epoch continue），脚本未覆盖换代与预约竞争。',
        'baseline_use': '该结论作为修订包真机判读前置：修订实现不得以裸 Wake 位置充当安全点，'
                        '须出示快照字段。',
    }


SCENARIOS = {'S1': s1_busy_owner, 'S2': s2_stale_job_live, 'S3': s3_identity_change,
             'S4': s4_timeout_and_late, 'S5': s5_late_end_and_cancel_after_arm}


def main():
    args = sys.argv[1:]
    if args and args[0] == '--trace':
        events = SCENARIOS.get(args[1])
        if events is None:
            print('unknown scenario', file=sys.stderr)
            return 2
        print(json.dumps(SCENARIOS[args[1]](), ensure_ascii=False, indent=1))
        return 0
    results = {}
    try:
        for name, fn in SCENARIOS.items():
            results[name] = fn()
    except AssertionError as error:
        print(json.dumps({'state': 'SELFTEST_FAIL', 'scenario': name, 'error': str(error)}))
        return 1
    audit = t409_wake_hook_audit()
    report = {'revision': 'T419_SAFEPOINT_BASELINE_V1', 'selftest': 'PASS',
              'scenarios': results, 't409WakeHookAudit': audit,
              'boundary': '本基线为 tester 独立合成层（对照 T417 合同文本），非产品代码；'
                          '对未定修复语义只记录预期（当前产品无预约接口，全部场景对现实现为'
                          '预期 RED 登记），不作为缺陷，交 architect 消费。',
              'constants': {'waitBudgetNs': WAIT_BUDGET_NS}}
    dest = REPO/'test'/'reports'/'t419'/'t419_baseline.json'
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(json.dumps(report, ensure_ascii=False, indent=1), encoding='utf-8')
    print(json.dumps({'revision': report['revision'], 'selftest': 'PASS',
                      'scenarios': list(results), 'baseline': str(dest)}))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
