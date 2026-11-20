# T569 v2 · DM20 设备首测——真实 GL 到达（Maleoon 935F），CREATE 卡同族 shader 缺陷

日期：2026-11-20，effector（tester 兼），真机（011124264R000337）。
被测：g9_dm_differential 独立 ELF（T670 DM 差分 harness，ELF db325543/BuildId
ee6dead0 + libgrs_dm_t647.so 5202800B，按 T670 handoff"部署各独立 ELF 及其
DT_NEEDED"路径——非 HAP 页面）+ fixtures.py dm 192 7680 --roots 1 输入
（config256/identity72/seed591872/commands768/quota1024/settings4/cold-credit8）。
证据：`test/reports/t491/t569b/`（v2-dm20.json、dm20-execution.jsonl 原件）。

## 执行链

设备 /data/local/tmp/t569 直跑（LD_LIBRARY_PATH 指向包内 so，240s 界内秒级
返回）——execution.jsonl：

```
{"revision":"T549_GPU_EXECUTION_V1","evidenceOrigin":"ACTUAL_GL_EXECUTION",
 "renderer":"Maleoon 935F","k":192,"residentPayloadBytes":55339648,...}
{"stage":"CREATE","result":8}
```

## 判定

- **实际 GL 执行到达**（evidenceOrigin=ACTUAL_GL_EXECUTION，renderer
  Maleoon 935F）——T670 差分链路（ELF+DT_NEEDED+fixtures）设备端成立
- **DM20 强启动卡 CREATE result=8**——hilog 同窗 `0:1728: S0022: Symbol
  'slot' redeclared`：**g9dm 族 shader 生成缺陷**（与 T645/T644 同签名）——
  T674 修复只覆盖 RX 作用域，g9dm 族未修
- 所有 DM 臂（DM20/22/29/31）在 shader 修复前均会撞同一堵墙——如实
  NOT_RUN/FAIL 交 architect

## 设施备忘（回 tester 域）

- hdc file send 目标已存在时**嵌套成目录**——推文件必须先清目标或用根目录
  临时名+mv/cp 归位；harness INPUT_LENGTH debug throw 已加（文件名/max/bytes）
- kProfiles K192 字段序实际为 stateBytes=591872（debug throw 确认），seed.u32
  用 fixtures 原始 591872B 即正确
- T670-dm 树 EntryAbility 已加 dm-only 适配（grsm0/grsm1 setActive 移除，
  T569-dmfix 包 T569 页面域可用，signed 515dbbbc）

## 回执

回 architect：shader 生成缺陷跨 g9rx/g9dm 两族（同 1728 行签名），修复需在
生成器层统一处理（T674 范围扩展或新卡）。DM20/22/29/31 设备原件在修复后
每轮可即测（ELF+输入+预算全部就绪）。
