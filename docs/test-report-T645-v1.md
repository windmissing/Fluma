# T645 v1 · 目标 9 新 RX 域真机首测——加载/资格 PASS，shader redeclared 拒

日期：2026-11-19，tester，真机（011124264R000337）。
被测：T645-rx-signed.hap（t645-tree=tools/g9rx/prepare_build.cjs 新鲜隔离树，
inputSha 0ef930ea→signed 0bdf3304，sign=PASS；含 T644 cced5866 全部 g9rx 域）。
证据：`test/reports/t645a/`（l1-l4 布局、t645a-state.jpeg、rx-hilog3.txt driver
原文、v1-rx-first.json）。

## 执行链与结果

| 阶段 | 实测 | 判定 |
|---|---|---|
| 部署/包 | bm clean 冷启，独立研究页直达（原生回执粒子场研究 OFF） | ✓ |
| **LOAD** | LOADED_FILE_OBSERVED：libgrs_g9rx.so sha256=b050b96e、1399592B、BuildId e0e62034…、**memoryBuildIdMatched=true** | **PASS**（包内字节与映射模块逐字节身份成立） |
| ENABLE | 两轮独立"启用研究"均进入 GL configure | ✓ |
| **SHADER** | **CONFIGURE_FAILED_8 = SHADER_FAILED**，driver 原文：`0:1728: S0022: Symbol 'slot' redeclared`（两轮稳定复现） | **FAIL**（产品生成缺陷） |

## 精确根因

g9rx 生成 shader 第 1728 行符号 `slot` 重复声明——T644 generate_assets.py /
冻结父源变体的**产品生成缺陷**，回 developer（工程域）。

## 阻断与附加发现（工具链域正面消息）

- 主树原全量构建被 cold_slots 链接行溢出挡死。本轮 tester 定位**精确根因**：
  CMakeCache CMAKE_SHARED/EXE_LINKER_FLAGS 各含 **64 次重复**的 rtlib/fuse-ld
  块（10.6KB，超 cmd 8191）——每次 CMake re-configure 累积追加。cache 去重后
  **主树 ninja 全量 rc=0**（grsm1/grsm0/cold_slots 全部链接成功）。
  该发现是 T655 architect 归因的决定性输入（工程修复=cmake 函数幂等化）
- t645-tree 构建另需 PATH 带 java（spawn java ENOENT 一次，重跑 PASS）

## NOT_REACHED 如实

READY/pulse/touch/物理/可见帧均未达（configure 未过）；T642 N01–N11 反例
不可达。设施+包就绪，T644 shader 修复后 v2 即续。
