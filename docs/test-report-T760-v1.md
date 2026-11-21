# T760 v1 · 目标 20 G13 双 scope 隔离可见性真机验证

日期：2026-11-21，effector（tester 兼，含出包），真机（011124264R000337）。
被测：T760-split（T756 GLES DEPENDENCY_SPLIT 修复交付，inputSha ab373f28
回执一致→signed 868106e2，sign=PASS，ARM64 BuildId e5ec50ff）。证据：
`test/reports/t491/t760/`（l1–l3 布局、t760-diag-full.json 资格原字、
t756-compute-persisted.jsonl 7 条 T475 记录、v1 判定）。

## 编译域：仍全过 ✅

G13_COMPUTE（T756 修订双 scope 布局，shaderSHA e1893369）COMPILE→LINK→
REFLECT→EXPORT→**PERSISTED binaryBytes=301961**（7 条 T475 记录）；缓存
新 key bacbc911 落盘。T756 绑定/可见性修订未破坏编译域。

## 资格门：硬件 FTZ 原字复现（三族逐字一致）

配置 REST → 240s → **INIT_ERROR_27 @ BOOT_EFFECTIVE_TERMINAL:EXIT**，
`effectiveRejectStage=OP65`：
- op65 WORK `[0,1065353216,1065353216,0,1065353216]`——**与 T739 attempt
  250 逐字相同**（+53 次正规 half-ULP tie 被 FTZ）
- op66 ctrl200To215 逐字相同，mismatchMasks [12,12]

**T756 修复域是绑定/可见性，不改精度域**——FTZ 原字不变属预期。

## 隔离域：本轮不可达（如实）

跨 lane、跨 program、WORK 重用、map、顶点可见性探针**全部位于数值门
之后**——G13 页唯一入口走 BOOT 数值门即停（INIT_ERROR_27），隔离域
采证不可达。合同明确"T730 完整根保持数值门，不放宽 FTZ"，故不放宽。

## 精确缺口与前置

**缺真实探针入口**：需 architect 提供（a）数值门之前的独立隔离域入口，
或（b）FTZ 决策（FTZ 安全向量/precise/门放宽）放行后的完整根路径。
T756 绑定修复已交付且编译域验证通过，但其可见性效果在门通过前不可观测。

## 回执

- 编译域 PASS（含双 scope 修订布局）
- 资格门：硬件 FTZ 原字三度复现（250/196/180 同字）
- 隔离域：NOT_REACHABLE（入口缺失，前置=architect 决策）
- T730 完整根：数值门保持，未放宽
