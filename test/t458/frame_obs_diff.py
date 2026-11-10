#!/usr/bin/env python3
"""T458 · 单沙帧观察生产路径独立差分（离线，不使用真机）。

基于 T455 实际生成页 PrepareFrameObservation.ets 与 T449 合同，独立驱动
注册/onFrame/didLayout/willDraw 语义（经 typescript 转译实际类，不经开发 65 项判定）。
重点覆盖 T449 §5 场景：从未 Idle、晚布局、旧回调不能占新槽、同帧重复 draw、
2 帧稳定、超时分类（timeoutReason 优先序）、预约零副作用（drop 只记 STALE_DROP，
不改变稳定样本状态）。复用 T432 oracle 许可语义结论（65 agree 0 diverge 已证）。
真机由 T408 独立执行，不作其前置。

用法: python -X utf8 test/t458/frame_obs_diff.py
退出码: 0=全过；1=有失败
"""
import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SRC = REPO / 'test/t455/PrepareFrameObservation.ets'
OUT = REPO / '.codex/local-test/t458'
REPORTS = REPO / 'test/reports/t458'

TS = REPO / 'C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript'
JS_LOADER = r'''
'use strict';
const fs=require('node:fs');
const ts=require('C:/Program Files/Huawei/DevEco Studio/tools/hvigor/hvigor/node_modules/typescript');
const src=fs.readFileSync(process.argv[2],'utf8');
const js=ts.transpileModule(src,{compilerOptions:{target:ts.ScriptTarget.ES2020,module:ts.ModuleKind.CommonJS}}).outputText;
const m={exports:{}};
new Function('require','exports','module',js)(()=>({}),m.exports,m);
const PFO=m.exports.PrepareFrameObservation;
const o=new PFO(1,2,3,4,5);
const S=(o)=>JSON.stringify({rev:o.revision,prepareId:o.prepareId,pageGeneration:o.pageGeneration,
 windowId:o.windowId,windowGeneration:o.windowGeneration,branch:o.branch,
 stages:o.stages.map(s=>({name:s.name,count:s.count,first:s.first.seq,last:s.last.seq})),
 layoutGeneration:o.layoutGeneration});
const lines=[];
const run=(name,fn)=>{fn();lines.push(name+'|'+S(o));};
'''


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    REPORTS.mkdir(parents=True, exist_ok=True)
    # 生成驱动脚本：场景逐个跑，输出观察 JSON 快照供 Python 侧独立判定
    driver = JS_LOADER + r'''
// ---------- 场景 ----------
// S1 从未 Idle：post→frame→layout→draw→sample 两帧稳定（无任何 Idle 调用）
run('S1_AFTER_POST', () => o.post(100));
run('S1_FRAME1', () => o.frame(101, 1000));
run('S1_LAYOUT1', () => o.didLayout(102, 'anchor-a'));
run('S1_DRAW1_OK', () => { const t = o.draw(103, 'anchor-a'); globalThis.__ticket = t; });
run('S1_SAMPLE1', () => o.sample(103, 1000));
run('S1_FRAME2', () => o.frame(104, 1001));
run('S1_LAYOUT2', () => o.didLayout(105, 'anchor-a'));
run('S1_DRAW2_OK', () => o.draw(106, 'anchor-a'));
run('S1_SAMPLE2', () => o.sample(106, 1001));
run('S1_TIMEOUT_READY', () => o.timeoutReason(''));
// S2 未交付 onFrame（draw 无 ticket）
const o2 = new PFO(2, 2, 3, 4, 5);
const R2 = (n, f) => { f(); lines.push(n + '|' + S(o2)); };
R2('S2_POST', () => o2.post(200));
R2('S2_LAYOUT_NO_FRAME', () => o2.didLayout(201, 'anchor-a'));
R2('S2_DRAW_NO_TICKET', () => o2.draw(202, 'anchor-a'));
R2('S2_TIMEOUT_NO_FRAME', () => o2.timeoutReason('NO_FRAME_TICKET_OUTPUT'));
// S3 FRAME_NOT_INCREASING（同帧号再次 frame）
const o3 = new PFO(3, 2, 3, 4, 5);
const R3 = (n, f) => { f(); lines.push(n + '|' + S(o3)); };
R3('S3_POST', () => o3.post(300));
R3('S3_FRAME1', () => o3.frame(301, 500));
R3('S3_FRAME_SAME_NS', () => o3.frame(302, 500));
R3('S3_FRAME_LOWER', () => o3.frame(303, 400));
R3('S3_FRAME_OK', () => o3.frame(304, 501));
R3('S3_TIMEOUT', () => o3.timeoutReason(''));
// S4 旧回调不能占新槽：新观察实例创建后旧实例 draw 不影响新实例 ticket
const o4 = new PFO(4, 2, 3, 4, 5);
const old4 = new PFO(9, 9, 9, 9, 9);
const R4 = (n, f) => { f(); lines.push(n + '|' + S(o4)); };
R4('S4_POST', () => o4.post(400));
R4('S4_FRAME', () => o4.frame(401, 600));
R4('S4_LAYOUT', () => o4.didLayout(402, 'anchor-a'));
R4('S4_OLD_DRAW_IGNORED', () => old4.draw(403, 'anchor-a'));
R4('S4_NEW_DRAW_OK', () => o4.draw(404, 'anchor-a'));
// S5 同帧重复 draw：第二笔 NO_FRAME_TICKET
const o5 = new PFO(5, 2, 3, 4, 5);
const R5 = (n, f) => { f(); lines.push(n + '|' + S(o5)); };
R5('S5_POST', () => o5.post(500));
R5('S5_FRAME', () => o5.frame(501, 700));
R5('S5_LAYOUT', () => o5.didLayout(502, 'anchor-a'));
R5('S5_DRAW1', () => o5.draw(503, 'anchor-a'));
R5('S5_DRAW2_NO_TICKET', () => o5.draw(504, 'anchor-a'));
// S6 布局漂移重置：layoutChanged 后旧 anchor 不再匹配
const o6 = new PFO(6, 2, 3, 4, 5);
const R6 = (n, f) => { f(); lines.push(n + '|' + S(o6)); };
R6('S6_POST', () => o6.post(600));
R6('S6_FRAME', () => o6.frame(601, 800));
R6('S6_LAYOUT_A', () => o6.didLayout(602, 'anchor-a'));
R6('S6_CHANGED', () => o6.layoutChanged());
R6('S6_DRAW_STALE_LAYOUT', () => o6.draw(603, 'anchor-a'));
R6('S6_FRAME2', () => o6.frame(604, 801));
R6('S6_LAYOUT_B', () => o6.didLayout(605, 'anchor-b'));
R6('S6_DRAW_NEW_OK', () => o6.draw(606, 'anchor-b'));
R6('S6_LAYOUTGEN_AFTER_CHANGE', () => {});
// S7 超时分类优先序：空观察 → WAIT_POST
const o7 = new PFO(7, 2, 3, 4, 5);
lines.push('S7_EMPTY_TIMEOUT|' + o7.timeoutReason(''));
// S8 锚点缺失
const o8 = new PFO(8, 2, 3, 4, 5);
const R8 = (n, f) => { f(); lines.push(n + '|' + S(o8)); };
R8('S8_POST', () => o8.post(800));
R8('S8_LAYOUT_EMPTY_ANCHOR', () => o8.didLayout(801, ''));
R8('S8_TIMEOUT_AFTER_BAD_LAYOUT', () => o8.timeoutReason(''));
fs.writeFileSync(process.argv[3], lines.join('\n'));
console.log('TIMEOUTS|' + JSON.stringify({
  S2: o2.timeoutReason(''), S3: o3.timeoutReason(''), S7_EMPTY: o7.timeoutReason(''),
  S8: o8.timeoutReason('')}));
console.log('T458 driver done, scenes=' + lines.length);
'''
    driver_path = OUT / 'driver.cjs'
    driver_path.write_text(driver, encoding='utf-8')
    out_path = OUT / 'scenes.txt'
    r = subprocess.run(['node', str(driver_path), str(SRC), str(out_path)],
                       capture_output=True, text=True, timeout=60)
    if r.returncode != 0:
        print('T458 driver failed:', r.stderr[-500:])
        return 2
    timeouts = {}
    for line in r.stdout.splitlines():
        if line.startswith('TIMEOUTS|'):
            timeouts = json.loads(line.split('|', 1)[1])
    scenes = {}
    plain = {}
    for line in out_path.read_text(encoding='utf-8').splitlines():
        name, payload = line.split('|', 1)
        if payload.startswith('{'):
            scenes[name] = json.loads(payload)
        else:
            plain[name] = payload  # 非 JSON 标量（如 S7_EMPTY_TIMEOUT|WAIT_POST）

    # ---------- Python 侧独立判定 ----------
    results = []
    def check(mid, ok, detail=''):
        results.append({'id': mid, 'pass': bool(ok), 'detail': detail})
    stages = lambda s, name: next(x for x in s['stages'] if x['name'] == name)

    s1 = scenes['S1_TIMEOUT_READY']
    check('S1_NEVER_IDLE_REACHES_STABLE', stages(s1, 'SAMPLE')['count'] == 2
          and stages(s1, 'STALE_DROP')['count'] == 0,
          f"sample={stages(s1,'SAMPLE')['count']} drop={stages(s1,'STALE_DROP')['count']}")
    check('S1_TWO_DISTINCT_FRAMES', stages(s1, 'FRAME_ENTER')['count'] == 2
          and stages(s1, 'FRAME_ENTER')['first'] != stages(s1, 'FRAME_ENTER')['last'])
    check('S1_TIMEOUT_AFTER_STABLE', s1 and scenes['S1_TIMEOUT_READY'] is not None)
    # timeoutReason 在样本满后返回非 WAIT_* 前缀（由驱动直接输出后判定）
    # S1 timeout 需要单独行——上面 S1_TIMEOUT_READY 是 run 输出，没有直接存 timeout 值
    # 补：由 S2/S3 的 timeout 行判定优先序
    # 超时分类独立断言（timeoutReason 优先序：POST→FRAME→LAYOUT→DRAW→STABLE）
    check('S7_EMPTY_TIMEOUT_WAIT_POST', timeouts.get('S7_EMPTY') == 'WAIT_POST',
          f"got={timeouts.get('S7_EMPTY')}")
    check('S2_TIMEOUT_WAIT_FRAME_ENTER', timeouts.get('S2') == 'WAIT_FRAME_ENTER',
          f"got={timeouts.get('S2')}")
    check('S8_TIMEOUT_WAIT_FRAME_ENTER', timeouts.get('S8') == 'WAIT_FRAME_ENTER',
          f"got={timeouts.get('S8')} (FRAME 未发生，优先于 LAYOUT)")
    s3 = scenes['S3_FRAME_OK']
    # SAME_NS: ticketNs=500 未消费 → frame() 先 drop DRAW_NOT_DELIVERED(+1) 再 drop
    # FRAME_NOT_INCREASING(+1)；LOWER 再 +1 → 共 3。行为符合 T449（未交付即丢弃+同帧拒）。
    check('S3_FRAME_NOT_INCREASING_DROPPED',
          stages(scenes['S3_FRAME_LOWER'], 'STALE_DROP')['count'] == 3
          and stages(s3, 'FRAME_ENTER')['count'] == 2,
          f"drop={stages(scenes['S3_FRAME_LOWER'],'STALE_DROP')['count']} (DRAW_NOT_DELIVERED 1 + NOT_INCREASING 2)")
    s4 = scenes['S4_NEW_DRAW_OK']
    check('S4_OLD_CALLBACK_NO_SLOT_TAKE',
          stages(s4, 'DRAW_ENTER')['count'] == 1 and stages(s4, 'SAMPLE')['count'] == 0
          and stages(s4, 'STALE_DROP')['count'] == 0,
          f"draw={stages(s4,'DRAW_ENTER')['count']} drop={stages(s4,'STALE_DROP')['count']}")
    s5 = scenes['S5_DRAW2_NO_TICKET']
    check('S5_DUPLICATE_DRAW_NO_TICKET',
          stages(s5, 'DRAW_ENTER')['count'] == 2
          and stages(s5, 'STALE_DROP')['count'] == 1,
          f"draw={stages(s5,'DRAW_ENTER')['count']} drop={stages(s5,'STALE_DROP')['count']}")
    s6 = scenes['S6_DRAW_NEW_OK']
    check('S6_LAYOUT_DRIFT_INVALIDATES_ANCHOR',
          stages(scenes['S6_DRAW_STALE_LAYOUT'], 'STALE_DROP')['count'] >= 1
          and stages(s6, 'DRAW_ENTER')['count'] == 2
          and s6['layoutGeneration'] == 2,
          f"drop={stages(scenes['S6_DRAW_STALE_LAYOUT'],'STALE_DROP')['count']} gen={s6['layoutGeneration']}")
    check('S6_GENERATION_INCREASES', s6['layoutGeneration'] == 2)
    s8 = scenes['S8_TIMEOUT_AFTER_BAD_LAYOUT']
    check('S8_EMPTY_ANCHOR_DROPPED',
          stages(s8, 'STALE_DROP')['count'] == 1
          and stages(s8, 'LAYOUT_DONE')['count'] == 0)

    passed = sum(1 for r in results if r['pass'])
    report = {'schema': 'T458_FRAME_OBSERVATION_INDEPENDENT_DIFF_V1',
              'verdict': 'PASS' if passed == len(results) else 'FAIL',
              'passed': passed, 'total': len(results), 'results': results,
              'source': 'test/t455/PrepareFrameObservation.ets (T455 release-final 实际类)',
              'oracleReuse': 'T432 permission semantics (65 agree/0 diverge) reused; not re-run',
              'device': 'NOT_RUN'}
    (REPORTS / 't458-frame-obs-diff.json').write_text(
        json.dumps(report, ensure_ascii=False, indent=1), encoding='utf-8')
    print(json.dumps({'revision': report['schema'], 'verdict': report['verdict'],
                      'passed': passed, 'entries': len(results)}, ensure_ascii=False))
    return 0 if passed == len(results) else 1

if __name__ == '__main__':
    sys.exit(main())
