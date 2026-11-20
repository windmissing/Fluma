# T712 v2 · DM 修订真机续测——真实 GL 到达，g9dm 族 coherent@1063 未修

日期：2026-11-20，effector（tester 兼），真机（011124264R000337）。
被测：T712-dm-signed（T706 DM 修订包，inputSha bc157d25→signed c5fbb714，
sign=PASS）+ g9_dm_differential 743304B（T706 ELF）+ libgrs_dm_t647.so
5203432B，部署 /data/local/tmp/t712；fixtures dm192+cold-credit（8B）。
证据：`test/reports/t491/t712b/`（v2-dm20.json、execution.jsonl 原件经
设备直接读取）。

## 执行链（T706 合同：独立 ELF+依赖+fixture）

驱动秒级返回，execution.jsonl：
```
{"revision":"T549_GPU_EXECUTION_V1","evidenceOrigin":"ACTUAL_GL_EXECUTION",
 "renderer":"Maleoon 935F","k":192,"residentPayloadBytes":55339648,...}
{"stage":"CREATE","result":8}
```
驱动日志：`0:1063: S0001: Function call discards 'coherent' access qualifier`

## 判定

- **T706 DM 修订正确加载**（真实 GL 执行到达，ACTUAL_GL_EXECUTION）
- **CREATE result=8 持续**——g9dm 族 shader 仍在 1063 行 coherent 缺陷：
  **T706 修订未包含 g9dm 族限定符变换**（RX 族已推进 1097→1987，g9dm 族
  仍在基线 1063）
- DM20/22/29/31 全臂阻塞至 g9dm 限定符变换交付（RX 族已证同型局部拷贝
  变换可行——1097 已消）

## 回执

回 architect/developer：g9dm 1063 行 coherent 变换（RX 模式已证）；
T712 设施全就绪（ELF+so+输入+预算），每轮修复即测。
