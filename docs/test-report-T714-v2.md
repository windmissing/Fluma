# T714 v2 · 四臂修复真机复测——路由/时钟/期限修复生效，创建链推进

日期：2026-11-20，effector（tester 兼），真机（011124264R000337）。
被测：T714 树双臂（M1-THREE_OFF inputSha 04011a05→signed e7974456、
M2-TWO_ON a978771a→98065974，sign 均 PASS）。
证据：`test/reports/t491/t714b/`（m1.json/m1b.json、m2.json、
m2out/native.json、m2-TWO_ON.json、v2-twoarm.json）。

## 对照 T701 基线（上轮 RED）

| 维度 | T701 树 | **T714 树** |
|---|---|---|
| M1 时钟 | CLOCK_V1（缺期限族） | **CLOCK_V2**（修复生效） |
| M1 物理 | 0:0 冻结 | **0:1 推进**（run0:1/epoch2/STALE_PAGE_SESSION） |
| M2 入口 | 无参启动落 NEWPIPE_FAULT | **直达 M2V3 页**（路由修复） |
| M2 创建 | glLinkProgram obj0 65.4s 卡死 | **apiCursor 移到 api=1/obj2/10.3s**（链推进） |

## 判定

- **T714 修复生效**：路由+时钟版本+root 期限族三域均过——M1 臂从冻结
  变为推进，M2 臂创建链从 LINK 卡死推进到新 API 域（api=1 obj2 inCall
  10.3s，seq=29）
- **首帧发布仍卡 STALE_PAGE_SESSION**（T477 token!=session 遗留族）——
  M1 臂新阻塞点明确
- M2 新观测入档（api=1/obj2/10.3s 第 1 次）

## 回执

回 architect：T714 三域修复确认；剩余 STALE_PAGE_SESSION（T477 族）+
M2 创建新 API 域两流。
