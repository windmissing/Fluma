# T627 v4 · 跨 DSO broker 共享运行独立验证——T675 设施七阶段全 PASS

日期：2026-11-20，effector（tester 兼），真机（011124264R000337）。
被测：T675 broker 四件套（libadapter_a 14800B / libadapter_b 14768B /
libgrs_cold_slots 200824B / t636driver3 95072B）+ T675 driver_v3 契约
（两参：dso-dir + attempt-prefix；每行 boot/pid/pidStartTicks/attempt 键）。
证据：`test/reports/t636/`（t627-v4-c.ndjson 88 行原件、t627-v4-c.json、
t627-v4-allpass.json）。

## 执行

/data/local/tmp/t675 直跑 driver3——resolve 行 16 条（A/B 两 adapter 全 8
符号真实地址+ABI 记录）+ M1–M5 全矩阵。

## 结果：七阶段全 PASS

| 阶段 | 内容 | 结果 |
|---|---|---|
| M1 | fresh0 门+B wrapper 操作 A 票据（BuilderDone/ReadBegin/ReadEnd/Freed 跨 DSO） | **PASS** |
| M2a | 自 builder ReadBegin 拒 + BuilderDone 后可读 | **PASS** |
| M2b | reader 全局门隔离对照 | **PASS** |
| M3 | Freed-while-reader 排序 | **PASS** |
| M4-route | route 累积信用（4097 拒/route 满） | **PASS** |
| M4-process | 进程帽 charge（cap−4096 charge→4097 拒→4096 准） | **PASS** |
| M5 | GPU 域 fresh0/同 ctx 拒/错 free+retire 守卫/**修正序 ContextRetired** | **PASS** |

## 判定

- **跨 DSO broker 共享运行独立验证完成**——T656 修正序 ContextRetired
  （B 保持 gB 活动→Retired 清 ctx 租约→A fresh0 Reserve 准）在 v3 语义观测
  后正式纳入矩阵并 PASS
- v3 的 D2 观测（Freed 无视活跃 reader 立即终结）在 M3 排序阶段按合同
  期望值执行 PASS（driver_v3 序列已按 T656 修正案重排）
- driver_v3 每行 boot/pid/pidStartTicks/attempt 全键 + 真实符号地址/ABI——
  单 attempt 无拼接合规

## 回执

T627 验收域（跨 DSO 共享运行）七阶段全 PASS 收口。M4-process 信用模型
（524375552 帽内 charge）首次设备实测。
