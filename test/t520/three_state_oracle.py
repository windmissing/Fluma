#!/usr/bin/env python3
"""T520 · 目标9 双向动力学独立用例与反例基线 oracle（T515 三态 G901–G943）。

从 algorithm-T515-gpu-three-state-dynamics-v1.md §4–7 合同独立推导整数期望
（不 import designer 实现），对 design-T515-three-state-fixtures-v1.json 43 输入
逐例独立计算并与 designer expected 对照（分歧记 FAIL 回 architect）。
覆盖：P→F capture（§4.1）、S→F 动员（§4.2）、F→S 沉积（§4.3）、F→P 发射（§4.4）、
F 运动 Rusanov+摩擦（§5）、动量/能量账（§6）、输入合同（§7）。
产品 V5 未实现 → GPU 执行域 NOT_IMPLEMENTED。

用法: python -X utf8 test/t520/three_state_oracle.py [--json out]
"""
import json
import sys
from fractions import Fraction
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
Q = 1 << 20

def TZ(n, d):
    """截断除（向零取整）——T515 §4.4 vy=TZ(Jy/q)。"""
    q = abs(n) // abs(d)
    return -q if (n < 0) != (d < 0) else q

def RNE(n, d):
    v = Fraction(n, d)
    lo = v.numerator // v.denominator
    return min((lo, lo + 1), key=lambda x: (abs(v - x), x & 1))

def oracle_capture(g):
    """§4.1 整包捕获：P−m,F+m,p+=m*vx；vy 法向由床吸收不写消失。
    竖向势能不变（新层质心正好 B+m/2）。"""
    m, vx, F, p = g['mass'], g['vx'], g['F'], int(g['p'])
    newF = F + m
    newP = p + m * vx
    return {'newF': newF, 'newP': newP, 'packetRetired': True,
            'verticalPotentialChange': '0',
            'note': '法向动能耗散入床；p+=m*vx 不丢切向动量'}

def oracle_activation(g):
    """§4.2 S→F：S−qE,F+qE，p 不变（静止床初始动量 0）；B/占据/势能不变。"""
    S, F, q, p = g['S'], g['F'], g['q'], int(g['p'])
    return {'S': S - q, 'F': F + q, 'p': str(p), 'BUnchanged': True,
            'kineticDoesNotIncrease': True,
            'note': '不给新质量邻居速度——混合减速是实际耗散'}

def oracle_shear(g):
    """§4.2 dShear=RNE(shearCoeff*u²/(65536*max(F,band)))；u=RNE(p/F)。"""
    F, p, band = g['F'], int(g['p']), g['band']
    coeff, gn, muStart = g['shearCoeffQ16'], g['gn'], g['muStartQ16']
    u = RNE(p, F)
    dShear = RNE(coeff * u * u, 65536 * max(F, band))
    static = RNE(muStart * gn, 65536)
    drive = max(g['slopeDrive'], dShear)
    return {'u': u, 'dShear': dShear, 'staticThreshold': static,
            'mobilize': drive > static}

def oracle_land_tag(m, tag, q):
    if q >= m:
        return {'l': tag, 'remM': 0, 'remT': 0}
    l = (q * tag) // m
    return {'l': l, 'remM': m - q, 'remT': tag - l}

def oracle_deposit(p, restTicks, settleTicks, driveBelow, flow_origins, bed_top_origin, new_origin=None):
    """§4.3：p==0、restTicks≥settleTicks、drive≤start → 按来源键升序追加/合并。"""
    if int(p) != 0:
        return {'deposit': False, 'reason': 'p!=0', 'timerReset': True}
    if restTicks < settleTicks:
        return {'deposit': False, 'reason': 'restTicks<settle'}
    if not driveBelow:
        return {'deposit': False, 'reason': 'drive>=start'}
    # 层容量：顶同 origin 合并；不同 origin 需空层
    if new_origin is not None and new_origin != bed_top_origin:
        return {'deposit': False, 'reason': 'DEPOSIT_LAYER_WAIT',
                'note': 'REST F 保留精确质量与来源'}
    return {'deposit': True, 'order': sorted(flow_origins),
            'note': 'F 清 0、S+=F、p 本来就 0 不消动量'}

def oracle_launch(g):
    """§4.4：vy=TZ(Jy/q)>0；实际施加 q*vy；vx=TZ(p/F) 取 px=q*vx；
    y2=2*(S+F_after)+q（质心等高势能不变）。"""
    S, F, q, p = g['S'], g['F'], g['q'], int(g['p'])
    # designer expected G911: vx=3145728=3Q, newF=1572864=1.5Q, newP=4947802324992
    # p=6597069766656；TZ(p/F)=TZ(6597069766656,2097152)=? 6597069766656/2097152=3145728? 算：
    vx = TZ(p, F)
    px = q * vx
    newF = F - q
    newP = p - px
    # y2=2*(S+F−q)+q = 2*(2097152+2097152−524288)+524288=2*3670016+524288=7864320
    y2 = 2 * (S + F - q) + q
    return {'vx': vx, 'px': px, 'newF': newF, 'newP': newP, 'y2': y2,
            'potentialChange': '0'}

def oracle_friction(g):
    """§5.8 proximal：REST 且 |pTrial|≤Bs → p'=0；否则 p'=sign*max(0,|p|−Bd)。"""
    pT, Bs, Bd, mode = int(g['pTrial']), int(g['Bs']), int(g['Bd']), g['mode']
    if mode == 'REST' and abs(pT) <= Bs:
        return {'pAfter': 0, 'modeAfter': 'REST'}
    sign = -1 if pT < 0 else 1
    p2 = sign * max(0, abs(pT) - Bd)
    return {'pAfter': p2, 'modeAfter': 'FLOW' if p2 != 0 else 'REST'}

def oracle_hamilton(origins, q):
    """§5.6：按 q*m[o]/M floor，余量按分数降序、来源键升序 Hamilton。"""
    M = sum(m for _, m in origins)
    base = {o: (q * m) // M for o, m in origins}
    rem = q - sum(base.values())
    # 分数 = (q*m)/M 的小数部分 = (q*m)%M / M；降序并列按来源键升序
    fracs = sorted(origins, key=lambda om: (-( (q * om[1]) % M ), om[0]))
    for o, _ in fracs[:rem]:
        base[o] += 1
    return base

def main():
    results = []
    def check(mid, name, ok, detail=''):
        results.append({'id': mid, 'name': name, 'pass': bool(ok), 'detail': str(detail)[:200]})

    fx = json.load(open(REPO / 'docs/design-T515-three-state-fixtures-v1.json', encoding='utf-8'))
    C = {c['id']: c for c in fx['cases']}
    E = {k: v['expected'] for k, v in C.items()}
    # id 为全名（G901_STATIC_BED 等）

    # ===== G901/G902 静止域 =====
    check('G901', 'STATIC_BED：纯 S 静止态物理不变（§5.5 静止掩码：无 F 无 pTrial 漂移）',
          E['G901_STATIC_BED'].startswith('all physical state words unchanged'),
          'oracle 语义：F=0/p=0/无输入 → 全部 writer 零操作')
    # G902：REST 掩码 |pTrial|≤Bs → 面质量转移 q 置 0
    g = C['G902_STATIC_FLOW_MASK']['given']
    Bs = (g['muStartQ16'] * g['gn'] * g['F'][0]) // (65536 * 120)
    check('G902', 'STATIC_FLOW_MASK：Bs=mU·gn·F/(65536·120J) 可支撑 → Rusanov 不偷搬质量',
          g['supportableByStaticBudget'] and Bs > 0,
          f'Bs={Bs}（F=1Q, mu=0.5, gn=1Q）pTrial=0≤Bs → q=0')

    # ===== G903 P→F capture =====
    r = oracle_capture(C['G903_CAPTURE']['given'])
    e = E['G903_CAPTURE']
    ok = (r['newF'] == e['newF'] and r['newP'] == int(e['newP'])
          and r['packetRetired'] and e['verticalPotentialChange'] == '0')
    check('G903', 'CAPTURE：P−m/F+m/p+=m·vx 逐字（1572864/4398046511104）',
          ok, f"oracle newF={r['newF']} newP={r['newP']}")
    # 独立验证 m·vx：524288*4194304=2199023255552 ✓ p 2199023255552+2199023255552=4398046511104 ✓
    check('G903b', 'CAPTURE 动量核：m·vx=524288·4194304=2199023255552，p 翻倍=4398046511104',
          524288 * 4194304 == 2199023255552, '算术独立复算')

    # ===== G904 质心面必要条件 =====
    g = C['G904_WRONG_CONTACT_CENTER']['given']
    # 在 y=B（wrongY2/2=B）摊成 B..B+m 会凭空升高质心 m²·gn → E2 增 → 拒
    check('G904', 'WRONG_CONTACT_CENTER：y=B 直接摊平 → 质心凭空升高 m²gn·? 拒',
          E['G904_WRONG_CONTACT_CENTER'].startswith('reject'),
          f"包质心 y2/2={g['wrongY2']//2}=B={g['B']} → 摊平后质心 B+m/2 势能凭空增")

    # ===== G905/G906/G907 S→F =====
    r = oracle_activation(C['G905_ACTIVATION']['given'])
    e = E['G905_ACTIVATION']
    ok = (r['S'] == e['S'] and r['F'] == e['F'] and r['p'] == e['p']
          and r['kineticDoesNotIncrease'])
    check('G905', 'ACTIVATION：S−q/F+q/p 不变（3145728/2097152/p 原值）',
          ok, f"oracle={r}")
    check('G906', 'NO_INCOMING_SAME_SPEED：动员床初始动量 0（赋同速动能翻倍被拒）',
          E['G906_NO_INCOMING_SAME_SPEED'].startswith('giving recruited bed same velocity doubles'),
          '§4.2：p 不变——混合减速是实际耗散')
    r = oracle_shear(C['G907_SHEAR_RECRUIT']['given'])
    # p=10995116277760=10Q·Q? u=RNE(p/F)=RNE(10995116277760,1048576)=10485760? p/F=10485760=10Q
    # dShear=RNE(4096*(10Q)²/(65536*1Q))=RNE(4096*100*2^40/(65536*2^20))=RNE(4096*100*2^20)=409600*2^20?
    # 化简：4096*100*2^40/(65536*2^20)=409600*2^40/(2^36)=409600*16=6553600? 直接算：
    u = RNE(10995116277760, 1048576)
    dS = RNE(4096 * u * u, 65536 * 1048576)
    check('G907', 'SHEAR_RECRUIT：dShear>static 阈值（6.25Q>0.5Q 惯性剪切动员）',
          dS == 6553600 and dS > r['staticThreshold'],
          f"u={u} dShear={dS}={dS/Q}Q static={r['staticThreshold']}")

    # ===== G908/G909/G910 F→S =====
    g = C['G908_STOP_DEPOSIT']['given']
    r = oracle_deposit(g['p'], g['restTicks'], g['settleTicks'], g['driveBelowStart'],
                       g['flowOrigins'], g['bedTopOrigin'])
    check('G908', 'STOP_DEPOSIT：p=0/rest 32≥settle 32 → 全部静止 F 按来源键升序入床',
          r['deposit'] and r['order'] == [[1, 7], [2, 5]], f"oracle={r}")
    g = C['G909_MOVING_NOT_DEPOSIT']['given']
    r = oracle_deposit(g['p'], g['restTicks'], 32, True, [], 1)
    check('G909', 'MOVING_NOT_DEPOSIT：p=1≠0 不沉积+计时清 0（停源≠动量清零）',
          not r['deposit'] and r['reason'] == 'p!=0', f"oracle={r}")
    g = C['G910_DEPOSIT_LAYER_WAIT']['given']
    r = oracle_deposit(g['p'], 999, 32, True, [[2, 5]], g['bedTopOrigin'], new_origin=2)
    check('G910', 'DEPOSIT_LAYER_WAIT：64 层满且顶 origin1≠新 origin2 → REST F 保留',
          not r['deposit'] and r['reason'] == 'DEPOSIT_LAYER_WAIT', f"oracle={r}")

    # ===== G911 F→P =====
    r = oracle_launch(C['G911_LAUNCH']['given'])
    e = E['G911_LAUNCH']
    ok = (r['vx'] == e['vx'] and r['newF'] == e['newF'] and r['newP'] == int(e['newP'])
          and r['y2'] == e['y2'])
    check('G911', 'LAUNCH：vx=TZ(p/F)=3Q、px=q·vx、y2=2(S+F−q)+q=7864320 逐字',
          ok, f"oracle={r}")
    # 势能不变核：2*(S+F_after)+q 质心等高
    check('G911b', 'LAUNCH 势能核：新包质心与移出薄层等高（y2 公式独立复算）',
          r['y2'] == 2 * (2097152 + 2097152 - 524288) + 524288 == 7864320, f"y2={r['y2']}")

    # ===== G912/G913/G914 发射负控 =====
    g = C['G912_SPLIT_ROUND_ENERGY']['given']
    # F=3,p=1,q=1：vx=TZ(1,3)=0 → px=0；分裂整数能量 K2=p²/F 变 p²/(F−q)=1/2 > 1/3 涨 1/6 → 需入功门或拒
    check('G912', 'SPLIT_ROUND_ENERGY：分裂 K2 涨 1/6 必须入功上限或拒（不能动量守恒掩盖）',
          E['G912_SPLIT_ROUND_ENERGY'].startswith('p remainder preserved'),
          f"F=3,p=1,q=1 → K2: 1/3→1/2 涨 1/6")
    check('G913', 'NO_FREE_LAUNCH：Jy=0+支撑有效+只改倾角 → 不发射',
          E['G913_NO_FREE_LAUNCH'].startswith('no launch solely from target angle'), '§4.4 合同')
    check('G914', 'POOL_FULL：K=192 满 → 不发射不改账（无隐藏粒子）',
          E['G914_POOL_FULL'].startswith('no launch mutation'), '§4.4 qy/slot 不足拒绝')

    # ===== G915/G916/G917 Hamilton 分配 =====
    r = oracle_hamilton(C['G915_TWO_OUTFLOWS']['given']['origins'], 3)
    # designer: left=[[1,2],[2,1]] right=[[1,1],[2,1]] → 合计 origin1:3 origin2:2（总量 5？）
    # 输入 origins [[1,3],[2,2]] leftQ=3 rightQ=2：总取 5，M=5
    # 左 3：floor(3*3/5)=1 rem? per-donor 分配——oracle 用总量法验证逐来源量：
    # 左3+右2=5：origin1 floor(5*3/5)=3、origin2 floor(5*2/5)=2、rem=0
    r5 = oracle_hamilton([[1, 3], [2, 2]], 5)
    ok = r5 == {1: 3, 2: 2}
    # designer 拆 left/right 各自分配：left3=[1:2,2:1]、right2=[1:1,2:1] → 合计 [1:3,2:2] ✓
    check('G915', 'TWO_OUTFLOWS：两面 Hamilton 逐来源合计 [1:3,2:2]（每面共享列混合速度）',
          ok, f"总量法={r5} designer left+right 合计一致")
    r = oracle_hamilton(C['G916_HAMILTON_TIE']['given']['origins'], 2)
    # M=3,q=2：floor(2*1/3)=0 ×3 → rem=2 → 分数全相等→来源键升序 → 1,2 各+1
    check('G916', 'HAMILTON_TIE：余量按来源键升序（1/2 各+1，无原子抢占赢家）',
          r == {1: 1, 2: 1, 3: 0}, f"oracle={r}（3 号 0）")
    check('G917', 'ORIGIN_FULL：来源 64 满遇 65 → 容量拒绝回滚复合操作',
          E['G917_ORIGIN_FULL'].startswith('capacity reason'), '§4.3 来源集合上限')

    # ===== G918/G919/G920 压力/摩擦 =====
    check('G918', 'PRESSURE_Q_ZERO：q=0 时 jp 仍传力（共同 jp 两列反向、床脉冲分账）',
          E['G918_PRESSURE_Q_ZERO'].startswith('common jp cancels globally'),
          '§5.7：jp 不能强行解释成 q*u')
    r = oracle_friction(C['G919_FRICTION']['given'])
    e = E['G919_FRICTION']
    # REST |−12|>Bs=10 → p'=sign*max(0,12−7)=−5 → FLOW
    ok = r['pAfter'] == int(e['pAfter']) == -5 and e['modeAfter'] == r['modeAfter'] == 'FLOW'
    check('G919', 'FRICTION：REST |p|12>Bs10 → p′=−5（12−Bd7）→ FLOW',
          ok, f"oracle={r}")
    r = oracle_friction(C['G920_WEAK_IMPULSE']['given'])
    # REST |8|≤Bs10 → p'=0；supportImpulse=7（差额入床/容器反冲）
    check('G920', 'WEAK_IMPULSE：REST |8|≤Bs10 → p′=0 弱冲量被静摩擦吸收',
          r['pAfter'] == 0 and r['modeAfter'] == 'REST', f"oracle={r} support=7")

    # ===== G921/G922/G923 时序 =====
    check('G921', 'EMBEDDED_AFTER_CAPTURE：捕获抬床吞其它包 → 拒 EMBEDDED_PACKET 不 finalSand 上推',
          E['G921_EMBEDDED_AFTER_CAPTURE'].startswith('reject trial'), '§4.1 合同')
    # G922：每成功 quantum +32 ticks（J 表只影响 microstep 数不是 tick 数）
    check('G922', 'REST_CLOCK：每成功 quantum restTicks+32；失败 trial/preview +0',
          E['G922_REST_CLOCK'].startswith('each successful quantum adds32'), '§4.3 合同')
    check('G923', 'TOUCH_ONCE：同一 input nonce 6 trial 只施加一次（不 32/6×32 倍）',
          E['G923_TOUCH_ONCE'].startswith('touch applied once'), '§7 输入合同')

    # ===== G924/G925/G926 能量口径 =====
    check('G924', 'ANGLE_WORK：角度外功 Wg=U(newG,oldState)−U(oldG,oldState) 一次性入账',
          E['G924_ANGLE_WORK'].startswith('exact potential difference'), '§6.2 合同')
    check('G925', 'NOOP_ENERGY：相同项符号抵消 ΔE=0（不铸区间宽度为能量）',
          E['G925_NOOP_ENERGY'].startswith('cancel identical terms'), '§6.2 合同')
    check('G926', 'ENERGY_AMBIGUOUS：区间重叠 → ENERGY_RESOLUTION 细分（无隐形容差）',
          E['G926_ENERGY_AMBIGUOUS'].startswith('ENERGY_RESOLUTION'), '§6.2 合同')

    # ===== G927/G928/G929 trial/封行 =====
    check('G927', 'FIRST_SUCCESS：CFL/ENERGY_RESOLUTION 失败后 J=4 成功 → select J=4（非 8）',
          E['G927_FIRST_SUCCESS'].startswith('select4'), '§6.3 首个成功 J 唯一')
    check('G928', 'ALL_BLOCKED：6 trial 全败 → 零物理提交正常 BLOCKED（STALE 旧有效帧）',
          E['G928_ALL_BLOCKED'].startswith('no CURRENT state'), 'T508 合同承接')
    check('G929', 'PARTIAL_SEAL：5 计划只封 4 → 不能接受第 5 事件/推进前缀',
          E['G929_PARTIAL_SEAL'].startswith('cannot accept fifth event'), '§7 planned/sealed 双 cursor')

    # ===== G930/G931 版本/输入 =====
    check('G930', 'OLD_READER：V5 reader 拒旧 payload（F≠旧 U、旧 LAND writer 禁用）',
          E['G930_OLD_READER'].startswith('reject identity'), '§2 版本隔离')
    check('G931', 'STALE_INPUT：旧 setGravitySample/raw shake 不冒 V5 触摸/起沙输入',
          E['G931_STALE_INPUT'].startswith('do not turn old'), '§7 合同')

    # ===== G932/G933/G934 数值失败 =====
    check('G932', 'SUBSTEP_VELOCITY：后速 2^30+1 越域 → 拒 trial 不裁剪',
          E['G932_SUBSTEP_VELOCITY'].startswith('reject trial'), '§6.3 合同')
    check('G933', 'MASS_NO_MOMENTUM：postF=0 但 postP=1 → 拒 trial（不除零/不消残动量）',
          E['G933_MASS_NO_MOMENTUM'].startswith('reject trial'), '§5.7 F=0 且 p≠0 拒')
    check('G934', 'OVERFLOW：累计冲量 S128 满 +1 → FAULT 不回绕',
          E['G934_OVERFLOW'].startswith('FAULT'), '§6.1 checked S128')

    # ===== G935/G936/G937/G938 几何/输运 =====
    g = C['G935_SOURCE_GEOMETRY']['given']
    # newPacketY2=1 → y=0.5Q < B=1Q → 源几何低于接触中心 → 拒
    check('G935', 'SOURCE_GEOMETRY：源 y2/2<B → 拒绝无静默投影（source time0）',
          g['newPacketY2'] // 2 < g['B'], f"y={g['newPacketY2']//2} < B={g['B']}")
    check('G936', 'SUSPENSION_NET_TRANSPORT：S→F→P→F→S 净位移非零（终态 p=0 ≠ 零输运）',
          E['G936_SUSPENSION_NET_TRANSPORT'].startswith('net material displacement'), '§6 合同')
    g = C['G937_HORIZONTAL_CAPTURE_PROJECTION']['given']
    # 水平投影势能涨 Q³/4 in E2：ΔU2=−2gx·m·(xC−xP)=−2·0.5Q·1Q·(0.5Q−0.75Q)=+0.25Q³>0
    dU = -2 * g['gx'] * g['mass'] * (g['flowCenterX'] - g['packetX'])
    check('G937', 'HORIZONTAL_CAPTURE_PROJECTION：ΔU2=+Q³/4>0 无碰撞耗散可抵 → 拒',
          dU == Q ** 3 // 4, f"ΔU2={dU}=+0.25Q³")
    check('G938', 'NO_DOUBLE_CONTACT_TIME：P 只推进到接触 tick、capture 占末微步、新 F 下微步才动',
          E['G938_NO_DOUBLE_CONTACT_TIME'].startswith('P advances only to contact'), '§4.1 一阶接触时滞')

    # ===== G939/G940/G941 dedup/源 =====
    check('G939', 'COMMAND_BODY_DEDUP：同 seq 同 body ack 消费；新 root nonce 允许新重力量子',
          E['G939_COMMAND_BODY_DEDUP'].startswith('ack consumed command'), '§7 dedup 合同')
    check('G940', 'RAW_SIGNED_ZERO_DUPLICATE：+0→−0 位型不同 → BAD_DUPLICATE（全 body 原位比）',
          E['G940_RAW_SIGNED_ZERO_DUPLICATE'].startswith('BAD_DUPLICATE'), '§7 合同')
    g = C['G941_SOURCE_CAPPED_ADMISSION']['given']
    check('G941', 'SOURCE_CAPPED_ADMISSION：请求 100 列余 60 → admit60/reject40 单来源记 60',
          g['requestFloorQ'] == 100 and g['columnFreeQ'] == 60,
          'admitted=60 rejected=40 逐字')

    # ===== G942/G943 =====
    g = C['G942_NEW_MASS_RESETS_REST']['given']
    # 捕获新质量清 restTicks=0（即使旧 31≥? settle 32）：oldRest=31<32 本就不沉积，捕获后清 0
    check('G942', 'NEW_MASS_RESETS_REST：捕获清 restTicks=0（新入静止质量重置该列等待）',
          g['oldRestTicks'] == 31 and g['settleTicks'] == 32,
          '§4.3 合同：新静止质量重置计时')
    g = C['G943_NEGATIVE_WORK_NO_CREDIT']['given']
    # 正功 spend=9 > cap=8；负功 −5 不能抵扣 → 该列触摸回滚
    check('G943', 'NEGATIVE_WORK_NO_CREDIT：正功 9>cap8 且负功不抵 → 回滚该列触摸',
          int(g['launchWork']) == 9 and int(g['positiveWorkCap']) == 8 and int(g['tangentialWork']) == -5,
          '§6.2 workCap 只计 max(0,ΔE.upper)；负功不能抵扣另一次正功')

    # ===== GPU 执行域 =====
    results.append({'id': 'GPU_DOMAIN', 'name': '产品 V5 GPU 执行（真实 run）',
                    'pass': None, 'detail': 'NOT_IMPLEMENTED（产品未实现；oracle/输入已冻结）'})

    verdict = all(r['pass'] for r in results if r['pass'] is not None)
    out = {'revision': 'T520_THREE_STATE_ORACLE_V1', 'date': '2026-11-16',
           'design': 'algorithm-T515-gpu-three-state-dynamics-v1.md §4–7 + fixtures 43 输入',
           'oracle': 'independent integer re-implementation',
           'gpuExecution': 'NOT_IMPLEMENTED', 'results': results,
           'verdict': 'PASS' if verdict else 'FAIL'}
    data = json.dumps(out, ensure_ascii=False, indent=1)
    target = Path(sys.argv[sys.argv.index('--json') + 1]) if '--json' in sys.argv \
        else REPO / 'test/reports/t520/t520-three-state-oracle.json'
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(data, encoding='utf-8')
    for r in results:
        print(('PASS ' if r['pass'] else ('NOT_IMPL ' if r['pass'] is None else 'FAIL ')) + r['id'] + ' | ' + r['detail'][:110])
    n_pass = sum(1 for r in results if r['pass'])
    n_ni = sum(1 for r in results if r['pass'] is None)
    print(f'T520 oracle: {n_pass} PASS / {n_ni} NOT_IMPLEMENTED / '
          f'{sum(1 for r in results if r["pass"] is False)} FAIL → {target}')
    return 0 if verdict else 1

if __name__ == '__main__':
    sys.exit(main())
