# T711 v2 · M1/M2 四臂首帧对照——首轮设备基准（RED：M1 臂 STATE_LOST、M2 臂 glLinkProgram）

日期：2026-11-20，effector（tester 兼），真机（011124264R000337）。
被测：T701 八构建（M1/M2 × THREE_OFF/TWO_OFF/THREE_ON/TWO_ON，八臂签名全
PASS：cb379e18/a8ac611d/da9f33fd/5b64a3d7/b873156d/7f73ec8c/24adbfe3/d09e82e8）。
证据：`test/reports/t491/t711b/`（arms.json、m1-THREE_OFF.json、
m1on3.json、m2threeoff/t468-create-1-*-native.json、m2-THREE_OFF.json、
v2-fourarm.json）。

## 首轮四臂实况

| 臂 | 页面 | 状态 |
|---|---|---|
| M1-THREE_OFF | M1 页（CLOCK_V1） | STALE/STATE_LOST run0:0 epoch2 session1，物理 0:0 |
| M1-TWO_ON | M1 页 | 同上，STATE_LOST 持续 25s+ |
| M2-TWO_ON | 无参启动落 NEWPIPE_FAULT（无路由 flag，本轮 NOT_RUN） | — |
| M2-THREE_OFF | M2V3 页 | STATE_LOST；**apiCursor api=6 glLinkProgram obj0 ENTER endNs=0，inCall 65.4s**（bootSerial=1） |

## 判定

- **M1 双臂 STATE_LOST 持续**——T701 树 M1 臂缺 T597-fixed 250ms root 期限
  族修复（CLOCK_V1 vs 正式树 CLOCK_V2），首帧对照在 T701 树为 **RED 基线**
  （无有效新管线帧）
- **M2 臂 glLinkProgram 开放第 4 次独立观测**（65.4s obj0，接 T591/T590 三
  次汇聚）——同 1728 行 shader 族，跨树稳定复现（T701 树同样卡 CREATE）
- M2-TWO_ON 需 Index M2V3 入口（无参启动无路由）——下轮补跑

## 回执

四臂首轮基准锁定；对照结论：**首帧对照在 T701 树 RED**（M1 臂需 root
期限修复家族、M2 臂需 shader 生成修复）——回 architect（与 T594 v4
coherent@1097 同一代码族缺陷）。
